#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "TestScene.h"
#include "NoteData.h"
#include "SoundComponent.h"
#include "SongInfo.h"
#include <numeric>
#include <cmath>
#include "JudgeStatsService.h"
#include "ChedParser.h"



namespace app::test {

    // ノーツタイプごとの効果音パス
    namespace {
        const char* GetHitSfxPath(NoteType t) {
            switch (t) {
            case NoteType::Tap: return "Assets\\sound\\tap.wav";
            default:            return "Assets\\sound\\tap.wav";
            }
        }
    }

    // -----------------------------
    // 定数定義
    // -----------------------------
    static constexpr int    K_BEATS_PER_MEASURE = 4;       // 小節あたり拍数
    static constexpr double K_DEFAULT_BPM = 120.0;         // デフォルトBPM
    static constexpr double K_OFFSET_SEC = 0.0;            // 曲全体の開始オフセット
    static constexpr float  INPUT_OFFSET_SEC = 0.0f;       // 入力遅延補正

    // 判定ウィンドウ（秒）
    static constexpr float J_WIN_PERFECT = 0.050f;
    static constexpr float J_WIN_GREAT = J_WIN_PERFECT + 0.030f;
    static constexpr float J_WIN_GOOD = J_WIN_GREAT + 0.030f;

    // 判定挙動補助
    static constexpr int   J_LOOKAHEAD_NOTES = 5;          // 同時押し先読み数
    static constexpr float J_ASSIGN_EARLY_BIAS = 0.005f;   // 早押し優遇時間補正
    static constexpr float J_POS_TOL_MULT = 2.0f;          // 判定許容範囲倍率

    // -----------------------------
    // BPM変化対応用の構造体
    // -----------------------------
    struct TempoEvent {
        double atBeat = 0.0;       // この拍から有効
        double bpm = K_DEFAULT_BPM;
        double spb = 60.0 / K_DEFAULT_BPM; // 秒/拍
        double atSec = 0.0;        // この拍が始まる時刻（秒）
    };

    // テンポイベントを拍順に並び替え、拍→秒の累積時間を構築
    static void BuildTempoMap(std::vector<TempoEvent>& tm) {
        if (tm.empty()) {
            tm.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
            return;
        }

        std::sort(tm.begin(), tm.end(), [](const TempoEvent& a, const TempoEvent& b) {
            return a.atBeat < b.atBeat;
            });

        tm[0].spb = 60.0 / tm[0].bpm;
        tm[0].atSec = 0.0;

        for (size_t i = 1; i < tm.size(); ++i) {
            tm[i].spb = 60.0 / tm[i].bpm;
            const double beatsSpan = tm[i].atBeat - tm[i - 1].atBeat;
            tm[i].atSec = tm[i - 1].atSec + beatsSpan * tm[i - 1].spb;
        }
    }

    // 絶対拍(absBeat)から秒に変換
    static double BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) {
        size_t lo = 0, hi = tm.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tm[mid].atBeat <= absBeat) lo = mid; else hi = mid;
        }
        const TempoEvent& seg = tm[lo];
        const double dBeat = absBeat - seg.atBeat;
        return seg.atSec + dBeat * seg.spb;
    }

    // ==================================================
    // 初期化：譜面読み込み・ノーツ生成
    // ==================================================
    void NoteManager::Begin() {
        auto owner = actorRef.Target(); if (!owner) return;
        auto* scene = &owner->GetScene();

        // 譜面パス取得
        std::string chartPath = "Save/chart/Sample.chart";
        if (auto* testScene = dynamic_cast<TestScene*>(scene)) {
            const SongInfo& selectedSong = testScene->GetSelectedSong();
            if (!selectedSong.chartPath.empty())
                chartPath = selectedSong.chartPath;
        }

        noteSequence.clear();
        std::vector<TempoEvent> tempoMap;
        bool sawAnyTempoMeta = false;
        noteOffset = 0.0;

        // ----------------------------------
        // 譜面ファイル読み込み
        // ----------------------------------
        {
            ChedParser parser;
            parser.Load(chartPath);



            // いったんクリア
            noteSequence.clear();
            tempoMap.clear();
            sawAnyTempoMeta = false;
            noteOffset = 0.0; // SUS側でWAVEOFFSET対応するならここで加味する

            // --------- テンポイベントをChedParserからコピー ---------
            // ChedTempoEvent: { double absBeat; double bpm; }
            for (const auto& te : parser.tempos) {
                TempoEvent e;
                e.atBeat = te.absBeat;          // この拍から有効
                e.bpm = te.bpm;
                e.spb = 60.0 / e.bpm;
                e.atSec = 0.0;                // 後で BuildTempoMap() が埋める
                tempoMap.push_back(e);
            }
            sawAnyTempoMeta = !tempoMap.empty();

            // --------- ノーツをChedParserからコピー ---------
            // ChedNote: { int lane; int measure; double beat; double absBeat; ... }
            for (const auto& cn : parser.notes) {
                NoteData nd;

                // SUSレーン(0,4,8,12) → ゲームレーン(0,1,2,3)
                nd.lane = std::clamp(cn.lane / 4, 0, 3);

                nd.mbt.measure = cn.measure;

                double beatInMeasure = cn.beat;
                nd.mbt.beat = static_cast<int>(std::floor(beatInMeasure));
                nd.mbt.tick16 = static_cast<int>(
                    std::round((beatInMeasure - nd.mbt.beat) * 16.0)
                    );
                nd.mbt.beat = std::clamp(nd.mbt.beat, 0, K_BEATS_PER_MEASURE - 1);
                nd.mbt.tick16 = std::clamp(nd.mbt.tick16, 0, 15);

                nd.type = NoteType::Tap;
                nd.absBeat = cn.absBeat;
                nd.judged = false;
                nd.result = JudgeResult::None;

                noteSequence.push_back(nd);
            }

            sf::debug::Debug::Log("===== AFTER lane mapping =====");
            for (const auto& nd : noteSequence) {
                sf::debug::Debug::Log("mapped lane=" + std::to_string(nd.lane)
                    + " absBeat=" + std::to_string(nd.absBeat));
            }
        }

        // BPMが未定義ならデフォルトを使用 （ よくない ）
        if (!sawAnyTempoMeta)
            tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
        BuildTempoMap(tempoMap);

        // 各ノーツのヒット時刻を計算
        for (auto& nd : noteSequence)
            nd.hittime = static_cast<float>(BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC + noteOffset);

        // ----------------------------------
        // レーンパラメータ・傾斜算出
        // ----------------------------------
        float slopeRad = rotX * 3.14159265f / 180.0f;
        float startZ = laneH * 0.5f;
        float hitZ = -laneH * 0.5f + laneH * barRatio;
        float startY = baseY - std::tan(slopeRad) * startZ;
        float hitY = baseY - std::tan(slopeRad) * hitZ;

        int lanes = std::max(1, (int)laneRefs.size());
        float totalDistance = std::abs(startZ - hitZ);

        leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
        songTime = -leadTime;

        // ----------------------------------
        // ノーツ生成
        // ----------------------------------
        noteActors.clear();
        nextIndex = 0;
        currentCombo = 0;

        for (size_t i = 0; i < noteSequence.size(); ++i) {
            const auto& nd = noteSequence[i];
            float laneX = (nd.lane - lanes * 0.5f + 0.5f) * laneW;

            auto noteActor = scene->Instantiate();
            auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
            mesh->SetGeometry(g_cube);
            mesh->material.SetColor({ 1, 1, 1, 1 });
            noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
            noteActor.Target()->transform.SetPosition({ laneX, startY, startZ });
            noteActor.Target()->transform.SetRotation({ rotX, 0, 0 });

            auto comp = noteActor.Target()->AddComponent<NoteComponent>();
            comp->info = nd;
            comp->leadTime = leadTime;
            comp->noteSpeed = noteSpeed;

            noteActor.Target()->DeActivate();
            noteActors.push_back(noteActor);
        }

        // レーンごとにノーツを分類して順序管理
        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }

        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));
    }

    // ==================================================
    // フレーム更新：ノーツ出現制御＋ミスチェック
    // ==================================================
    void NoteManager::Update(const sf::command::ICommand&) {
        songTime += sf::Time::DeltaTime();

        // 出現タイミングに達したノーツをActivate
        while (nextIndex < noteSequence.size()
            && songTime + leadTime >= noteSequence[nextIndex].hittime)
        {
            auto* act = noteActors[nextIndex].Target();
            if (act) act->Activate();
            ++nextIndex;
        }

        CheckMissedNotes();
    }

    // ==================================================
    // 単一ノーツの時間判定
    // ==================================================
    JudgeResult NoteManager::JudgeNote(NoteType, float noteTime, float inputTime) {
        inputTime += INPUT_OFFSET_SEC;
        float diff = std::abs(noteTime - inputTime);
        if (diff <= J_WIN_PERFECT) return JudgeResult::Perfect;
        if (diff <= J_WIN_GREAT)   return JudgeResult::Great;
        if (diff <= J_WIN_GOOD)    return JudgeResult::Good;
        return JudgeResult::Miss;
    }

    // ==================================================
    // レーン単体との判定（Z座標＋時間）
    // ==================================================
    JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
        if (lane < 0 || lane >= (int)laneOrder.size()) return JudgeResult::Skip;

        auto& order = laneOrder[lane];
        size_t& head = laneHeads[lane];
        while (head < order.size() && noteSequence[order[head]].judged) ++head;
        if (head >= order.size()) return JudgeResult::Skip;

        int idx = order[head];
        if (idx >= (int)nextIndex) return JudgeResult::Skip;

        auto* act = noteActors[idx].Target();
        if (!act) return JudgeResult::Skip;

        float noteZ = act->transform.GetPosition().z;
        float diffZ = std::abs(noteZ - judgeZ);
        if (diffZ > judgeRange * J_POS_TOL_MULT) return JudgeResult::Skip;

        JudgeResult res = JudgeNote(noteSequence[idx].type, noteSequence[idx].hittime, inputTime);
        noteSequence[idx].judged = true;
        noteSequence[idx].result = res;
        //UpdateCombo(res);

        JudgeStatsService::AddResult(res);

        act->DeActivate();
        act->Destroy();      // 次のDestroyActors()でdeleteされる

        if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
            sound->Play(GetHitSfxPath(noteSequence[idx].type));
        }

        ++head;
        return res;
    }

    // ==================================================
    // 同時押し判定：複数レーンをまとめて処理
    // ==================================================
    void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
        for (int l : pressedLanes) {
            JudgeLane(l, inputTime);
        }
    }

    // ==================================================
    // ミス判定：一定時間経過した未処理ノーツをMiss扱い
    // ==================================================
    void NoteManager::CheckMissedNotes() {
        for (int l = 0; l < (int)laneOrder.size(); ++l) {
            auto& order = laneOrder[l];
            size_t& head = laneHeads[l];

            while (head < order.size()) {
                int i = order[head];
                if (noteSequence[i].judged) { ++head; continue; }

                // 未Activateは対象外
                if (i >= (int)nextIndex) break;

                // 判定窓超過 → Miss
                if (songTime > noteSequence[i].hittime + J_WIN_GOOD) {
                    noteSequence[i].judged = true;
                    noteSequence[i].result = JudgeResult::Miss;

                    //コンボリセット
                    JudgeStatsService::AddResult(JudgeResult::Miss);

                    //ミスしたノーツを削除予約
                    if (auto* act = noteActors[i].Target()) {
                        act->DeActivate();  
                        act->Destroy();     // 削除予約（呼ぶだけ）
                    }
                    ++head;
                }
                else {
                    break;
                }
            }
        }
    }

    // ==================================================
    // コンボ更新処理
    // ==================================================
    void NoteManager::UpdateCombo(JudgeResult result) {
        switch (result) {
        case JudgeResult::Perfect:
        case JudgeResult::Great:
        case JudgeResult::Good:
            ++currentCombo; break;
        case JudgeResult::Miss:
            currentCombo = 0; break;
        default: break;
        }
    }

    int NoteManager::GetCurrentCombo() const { return currentCombo; }

// ==================================================
// レーン設定：TestScene 側から呼ばれて情報を渡す
// ==================================================
    void NoteManager::SetLaneParams(
        const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
        float laneW_, float laneH_, float rotX_,
        float baseY_, float barRatio_)
    {
        laneRefs = lanes;
        laneW = laneW_;
        laneH = laneH_;
        rotX = rotX_;
        baseY = baseY_;
        barRatio = barRatio_;

        // 判定ラインZ座標を計算（バーの位置）
        judgeZ = -laneH * 0.5f + laneH * barRatio;
    }
} // namespace app::test
