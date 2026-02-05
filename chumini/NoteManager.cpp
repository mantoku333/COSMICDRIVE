#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include "BGMComponent.h"
#include "Config.h"
#include "DirectX11.h"
#include <cstdio>
#pragma comment(lib, "d3dcompiler.lib")
#include <fstream>
#include <sstream>
#include <algorithm>
#include "IngameScene.h"
#include "NoteData.h"
#include "SoundComponent.h"
#include "SongInfo.h"
#include <numeric>
#include <cmath>
#include <map>
#include "JudgeStatsService.h"
#include "ChedParser.h"
#include "ResultScene.h"
#include "IngameCanvas.h"
#include "LoadingScene.h"
#include <filesystem>
#include <windows.h>
#include "StringUtils.h"  // 文字コード変換ユーティリティ

// sf::util の関数を使用（ローカル関数は削除）
namespace {
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;
}

namespace app::test {

    // ============================================================
    // 効果音パス取得
    // ============================================================
    namespace {
        const char* GetHitSfxPath(NoteType t) {
            switch (t) {
            case NoteType::Tap:   return "Assets\\sound\\tap.wav";
            case NoteType::Skill: return "Assets\\sound\\tap.wav";
            default:              return "Assets\\sound\\tap.wav";
            }
        }
    }

    // ============================================================
    // 定数定義
    // ============================================================
    static constexpr int    K_BEATS_PER_MEASURE = 4;
    static constexpr double K_DEFAULT_BPM = 120.0;
    static constexpr double K_OFFSET_SEC = 0.15;
    static constexpr float  INPUT_OFFSET_SEC = 0.0f;



    // ============================================================
    // 初期化（Begin）
    // 譜面ファイルを読み込み、ノーツを生成する
    // 1. ハイスピード設定
    // 2. 譜面ファイルのパース
    // 3. テンポマップ構築
    // 4. ノーツデータのソートとペアリング
    // 5. ノーツアクターの生成
    // 6. インスタンシング初期化
    // 7. 最大コンボ数の事前計算
    // ============================================================
    void NoteManager::Begin() {
        // UI上の 7.0 が内部値 30.0 になるように変換
        float factor = 30.0f / 7.0f;
        HiSpeed = gGameConfig.hiSpeed * factor;
        noteSpeed = basenoteSpeed * HiSpeed;

        while (ShowCursor(FALSE) >= 0);

        if (resultScene.isNull()) {
            resultScene = ResultScene::StandbyScene();
        }

        if (auto actor = actorRef.Target()) {
            auto* changer = actor->GetComponent<SceneChangeComponent>();
            if (changer) {
                sceneChanger = changer;
            }
        }

        isPlaying = false;
        JudgeStatsService::Reset();

        GetCursorPos(&lastCursorPos);
        mouseSpeed = 0.0f;

        auto owner = actorRef.Target();
        if (!owner) return;
        auto* scene = &owner->GetScene();

        // 譜面パス取得
        std::string chartPath = "Save/chart/Sample.chart";
        if (auto* ingameScene = dynamic_cast<IngameScene*>(scene)) {
            const SongInfo& selectedSong = ingameScene->GetSelectedSong();
            if (!selectedSong.chartPath.empty())
                chartPath = selectedSong.chartPath;
        }

        noteSequence.clear();
        tempoMap.clear();
        bool sawAnyTempoMeta = false;
        noteOffset = 0.0;

        // ------------------------------------------
        // 譜面ファイル読み込み
        // ------------------------------------------
        {
            ChedParser parser;
            std::wstring wChartPath = Utf8ToWstring(chartPath);
            if (parser.Load(wChartPath)) {
                sf::debug::Debug::Log("Chart Loaded: " + chartPath);
            } else {
                sf::debug::Debug::Log("Chart Load Failed: " + chartPath);
            }

            noteSequence.clear();
            tempoMap.clear();
            sawAnyTempoMeta = false;
            noteOffset = 0.0;

            // テンポイベント取得
            for (const auto& te : parser.tempos) {
                TempoEvent e;
                e.atBeat = te.absBeat;
                e.bpm = te.bpm;
                e.spb = 60.0 / e.bpm;
                e.atSec = 0.0;
                tempoMap.push_back(e);
            }
            sawAnyTempoMeta = !tempoMap.empty();

            // ホールドノーツのペアリング用マップ
            std::map<int, size_t> pendingHolds;

            for (const auto& cn : parser.notes) {
                NoteData nd;
                nd.lane = std::clamp(cn.lane, 0, 5);
                nd.mbt.measure = cn.measure;

                double beatInMeasure = cn.beat;
                nd.mbt.beat = static_cast<int>(std::floor(beatInMeasure));
                nd.mbt.tick16 = static_cast<int>(std::round((beatInMeasure - nd.mbt.beat) * 16.0));
                nd.mbt.beat = std::clamp(nd.mbt.beat, 0, K_BEATS_PER_MEASURE - 1);
                nd.mbt.tick16 = std::clamp(nd.mbt.tick16, 0, 15);

                nd.type = cn.type;
                nd.absBeat = cn.absBeat;
                nd.judged = false;
                nd.result = JudgeResult::None;
                nd.pairIndex = -1;
                nd.duration = 0.0f;

                // ホールドノーツ処理
                if (cn.isHold) {
                    int key = (cn.lane << 8) | cn.holdId;
                    if (pendingHolds.count(key)) {
                        size_t startIdx = pendingHolds[key];
                        noteSequence[startIdx].type = NoteType::HoldStart;
                        noteSequence[startIdx].pairIndex = (int)noteSequence.size();
                        nd.type = NoteType::HoldEnd;
                        nd.pairIndex = (int)startIdx;
                        pendingHolds.erase(key);
                    } else {
                        nd.type = NoteType::HoldStart;
                        pendingHolds[key] = noteSequence.size();
                    }
                }

                noteSequence.push_back(nd);
            }
        }

        if (!sawAnyTempoMeta)
            tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
        tempoService.BuildTempoMap(tempoMap);

        // ヒット時間計算
        for (auto& nd : noteSequence)
            nd.hittime = static_cast<float>(tempoService.BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC + noteOffset + gGameConfig.offsetSec);

        // ------------------------------------------
        // 時間順ソート
        // ------------------------------------------
        std::stable_sort(noteSequence.begin(), noteSequence.end(),
            [](const NoteData& a, const NoteData& b) {
                return a.hittime < b.hittime;
            });

        // ソート後のペアリング再構築
        std::vector<int> pendingStartIdx(6, -1);
        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            noteSequence[i].pairIndex = -1;

            if (noteSequence[i].type == NoteType::HoldStart) {
                pendingStartIdx[noteSequence[i].lane] = i;
            }
            else if (noteSequence[i].type == NoteType::HoldEnd) {
                int startIdx = pendingStartIdx[noteSequence[i].lane];
                if (startIdx != -1) {
                    noteSequence[startIdx].pairIndex = i;
                    noteSequence[i].pairIndex = startIdx;
                    pendingStartIdx[noteSequence[i].lane] = -1;
                }
            }
        }

        // ホールド長さ計算
        for (auto& nd : noteSequence) {
            if (nd.type == NoteType::HoldStart && nd.pairIndex != -1) {
                if (nd.pairIndex < (int)noteSequence.size()) {
                    nd.duration = noteSequence[nd.pairIndex].hittime - nd.hittime;
                }
            }
        }

        // ------------------------------------------
        // レーンパラメータ
        // ------------------------------------------
        float slopeRad = rotX * 3.14159265f / 180.0f;
        float startZ = laneH * 0.5f;
        float hitZ = -laneH * 0.5f + laneH * barRatio;
        float startY = baseY - std::tan(slopeRad) * startZ;

        int lanes = std::max(1, (int)laneRefs.size());
        float totalDistance = std::abs(startZ - hitZ);

        leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
        songTime = -leadTime;

        // ------------------------------------------
        // ノーツアクター生成
        // ------------------------------------------
        noteActors.clear();
        nextIndex = 0;
        currentCombo = 0;

        for (size_t i = 0; i < noteSequence.size(); ++i) {
            if (noteSequence[i].type == NoteType::SongEnd) {
                noteActors.push_back({});
                continue;
            }

            float laneX = 0.0f;
            float laneYOffset = 0.0f;

            if (noteSequence[i].lane == 4) {
                laneX = sideLaneX_Left;
            }
            else if (noteSequence[i].lane == 5) {
                laneX = sideLaneX_Right;
            }
            else if (noteSequence[i].type == NoteType::Skill) {
                laneX = 0.0f;
            }
            else {
                laneX = (noteSequence[i].lane - 4 * 0.5f + 0.5f) * laneW;
            }

            auto noteActor = scene->Instantiate();

            // メッシュ追加（ホールド・スキルのみ）
            if (noteSequence[i].type == NoteType::HoldStart ||
                noteSequence[i].type == NoteType::HoldEnd ||
                noteSequence[i].type == NoteType::Skill) {
                auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
                mesh->SetGeometry(g_cube);
                mesh->material.SetColor({ 1, 1, 1, 1 });

                if (noteSequence[i].type == NoteType::Skill) {
                    mesh->material.SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
                    noteActor.Target()->transform.SetScale({ laneW * 4.0f, 0.5f, 0.2f });
                } else {
                    noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
                }
            } else {
                noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
            }

            noteActor.Target()->transform.SetPosition({ laneX, startY + laneYOffset, startZ });
            noteActor.Target()->transform.SetRotation({ rotX, 0, 0 });

            auto comp = noteActor.Target()->AddComponent<NoteComponent>();
            comp->info = noteSequence[i];
            comp->leadTime = leadTime;
            comp->noteSpeed = noteSpeed;

            noteActor.Target()->DeActivate();
            noteActors.push_back(noteActor);
        }

        // レーン管理初期化
        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        activeHolds.assign(lanes, -1);
        activeHoldNextBeats.assign(lanes, 0.0);

        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            if (noteSequence[i].type == NoteType::SongEnd) {
                songEndIndex = i;
            }
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }

        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));

        try {
            InitInstancing();
        }
        catch (...) {
            sf::debug::Debug::LogError("InitInstancing Failed");
        }

        // ------------------------------------------
        // 最大コンボ数計算
        // ------------------------------------------
        maxTotalCombo = 0;
        for (const auto& nd : noteSequence) {
            if (nd.type == NoteType::SongEnd) continue;

            if (nd.type == NoteType::Tap || nd.type == NoteType::Skill || nd.type == NoteType::HoldStart) {
                maxTotalCombo++;
            }

            if (nd.type == NoteType::HoldStart && nd.pairIndex != -1 && nd.pairIndex < (int)noteSequence.size()) {
                const auto& endNote = noteSequence[nd.pairIndex];
                double startBeat = nd.absBeat;
                double endBeat = endNote.absBeat;
                double nextBeat = startBeat + 0.5;

                while (nextBeat < endBeat) {
                    maxTotalCombo++;
                    nextBeat += 0.5;
                }
                maxTotalCombo++;
            }
        }
    }

    // ============================================================
    // 更新（Update）
    // 毎フレーム呼び出される
    // 1. 曲時間の更新
    // 2. ノーツのアクティベート（画面外から可視化）
    // 3. ミスノーツの検出
    // 4. マウススラッシュによるスキルノーツ判定
    // ============================================================
    void NoteManager::Update(const sf::command::ICommand&) {
        if (!isPlaying) return;

        // 曲時間を進める
        songTime += sf::Time::DeltaTime();

        // ノーツのアクティベート
        // leadTime分前倒しでノーツを画面に出現させる
        while (nextIndex < noteSequence.size()
            && songTime + leadTime >= noteSequence[nextIndex].hittime)
        {
            if (noteSequence[nextIndex].type == NoteType::SongEnd) {
                ++nextIndex;
                continue;
            }

            auto* act = noteActors[nextIndex].Target();
            if (act) act->Activate();
            ++nextIndex;
        }

        // ミスノーツの検出と処理
        CheckMissedNotes();

        // ------------------------------------------
        // マウススラッシュ処理
        // マウスの移動速度が閾値を超えたら
        // スキルノーツを判定する
        // ------------------------------------------
        POINT currentCursorPos;
        GetCursorPos(&currentCursorPos);

        float dx = (float)(currentCursorPos.x - lastCursorPos.x);
        float dy = (float)(currentCursorPos.y - lastCursorPos.y);
        float mouseSpeed = std::sqrt(dx * dx + dy * dy);

        if (mouseSpeed > 10.0f) {
            for (int l = 0; l < (int)laneOrder.size(); ++l) {
                auto& order = laneOrder[l];
                size_t& head = laneHeads[l];
                size_t maxScan = std::min(head + 3, order.size());

                for (size_t k = head; k < maxScan; ++k) {
                    int idx = order[k];
                    if (noteSequence[idx].judged) continue;

                    if (noteSequence[idx].type == NoteType::Skill) {
                        float diff = std::abs((songTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime);
                        if (diff <= J_WIN_GOOD) {
                            noteSequence[idx].judged = true;
                            noteSequence[idx].result = JudgeResult::Perfect;
                            JudgeStatsService::AddResult(JudgeResult::Perfect);

                            if (auto* scene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene())) {
                                scene->TriggerSkillEffect();
                            }

                            if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                canvas->SpawnSlashEffect(0.0f, -300.0f, 16.0f, 4.0f, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
                            }

                            if (auto* act = noteActors[idx].Target()) {
                                act->DeActivate();
                                act->Destroy();
                            }
                        }
                    }
                }
            }
        }

        lastCursorPos = currentCursorPos;
    }

    // ============================================================
    // 単一ノーツの時間判定
    // ノーツのヒット時間と入力時間の差から判定結果を決定
    // HoldStartは判定緩和（ホールドは始まりが重要）
    // ============================================================
    JudgeResult NoteManager::JudgeNote(NoteType type, float noteTime, float inputTime) {
        // 入力オフセットを適用
        inputTime += INPUT_OFFSET_SEC;
        float diff = std::abs(noteTime - inputTime);

        // HoldStartは判定ウィンドウを広げる（ホールドは開始が重要）
        if (type == NoteType::HoldStart) {
            if (diff <= J_WIN_PERFECT * 2.0f) return JudgeResult::Perfect;
            if (diff <= J_WIN_GREAT * 1.5f)   return JudgeResult::Great;
            if (diff <= J_WIN_GOOD * 1.5f)    return JudgeResult::Good;
        } else {
            // 通常ノーツの判定
            if (diff <= J_WIN_PERFECT) return JudgeResult::Perfect;
            if (diff <= J_WIN_GREAT)   return JudgeResult::Great;
            if (diff <= J_WIN_GOOD)    return JudgeResult::Good;
        }
        return JudgeResult::Miss;
    }

    // ============================================================
    // レーン単位の判定処理
    // 先読み範囲内から最適なノーツを探して判定
    // - 判定済みノーツはスキップ
    // - まだスポーンしていないノーツはスキップ
    // - HoldEndとSkillはタップ判定しない
    // - 位置許容度を考慮した判定
    // ============================================================

    JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
        if (lane < 0 || lane >= 6) {
            return JudgeResult::Skip;
        }

        auto& order = laneOrder[lane];
        size_t& head = laneHeads[lane];
        size_t currentIdx = head;
        int lookAheadCount = 0;
        const int MAX_LOOKAHEAD = 5;

        while (currentIdx < order.size() && lookAheadCount < MAX_LOOKAHEAD) {
            if (noteSequence[order[currentIdx]].judged) {
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            int idx = order[currentIdx];

            if (noteSequence[idx].type == NoteType::SongEnd) {
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            if (idx >= (int)nextIndex) {
                return JudgeResult::Skip;
            }

            auto* act = noteActors[idx].Target();
            if (!act) {
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            float noteZ = act->transform.GetPosition().z;

            if (noteSequence[idx].type == NoteType::HoldEnd ||
                noteSequence[idx].type == NoteType::Skill) {
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            if (noteSequence[idx].type == NoteType::HoldStart) {
                float slopeRad = rotX * 3.14159265f / 180.0f;
                float halfLenZ = act->transform.GetScale().z * std::cos(slopeRad) * 0.5f;
                noteZ -= halfLenZ;
            }

            JudgeResult res = JudgeNote(noteSequence[idx].type, noteSequence[idx].hittime, inputTime);

            if (res == JudgeResult::Miss) {
                float diff = inputTime - noteSequence[idx].hittime;

                if (diff > J_WIN_GOOD) {
                    ++currentIdx;
                    continue;
                }
                else if (diff < -J_WIN_GOOD) {
                    return JudgeResult::Skip;
                }

                float diffZ = std::abs(noteZ - judgeZ);
                if (diffZ > judgeRange * J_POS_TOL_MULT) {
                    if (diff < 0) return JudgeResult::Skip;
                    else {
                        ++currentIdx;
                        continue;
                    }
                }
            }

            // 判定適用
            noteSequence[idx].judged = true;
            noteSequence[idx].result = res;
            JudgeStatsService::AddResult(res);

            if (res != JudgeResult::Skip && res != JudgeResult::Miss) {
                if (noteSequence[idx].type == NoteType::Skill) {
                    if (auto* scene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene())) {
                        scene->TriggerSkillEffect();
                    }
                }

                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                    float hitY = -265.0f;
                    float uiLaneWidth = 300.0f;
                    float sideOffset = 250.0f;
                    float hitX = 0.0f;
                    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
                    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
                    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
                    float scale = (uiLaneWidth / 100.0f) * 1.5f;
                    canvas->SpawnHitEffect(hitX, hitY, scale, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
                }
            }

            // ホールド登録
            if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
                if (noteSequence[idx].type == NoteType::HoldStart && noteSequence[idx].pairIndex != -1) {
                    activeHolds[lane] = noteSequence[idx].pairIndex;
                    activeHoldNextBeats[lane] = noteSequence[idx].absBeat + 0.5;
                }
            }

            // FAST/SLOW表示
            if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
                float diff = (inputTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime;
                int type = (diff < 0) ? 1 : 2;
                if (diff < 0) JudgeStatsService::AddFast();
                else JudgeStatsService::AddSlow();

                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                    canvas->ShowFastSlow(type);
                }
            }

            // アクター破棄
            bool shouldDestroy = true;
            if (noteSequence[idx].type == NoteType::HoldStart) shouldDestroy = false;
            if (shouldDestroy) {
                act->DeActivate();
                act->Destroy();
            }

            return res;

            ++lookAheadCount;
        }

        return JudgeResult::Skip;
    }

    void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
        for (int l : pressedLanes) {
            JudgeLane(l, inputTime);
        }
    }

    // ============================================================
    // デバッグ：強制クリア
    // ============================================================
    void NoteManager::DebugForceComplete() {
        for (auto& note : noteSequence) {
            if (note.judged) continue;
            if (note.type == NoteType::SongEnd) continue;

            note.judged = true;
            note.result = JudgeResult::Perfect;
            JudgeStatsService::AddResult(JudgeResult::Perfect);
        }

        if (!sceneChanger.isNull()) {
            sf::Mesh::ClearAllRegistered();
            ShowCursor(TRUE);
            LoadingScene::SetLoadingType(LoadingType::Common);
            if (resultScene.Get() == nullptr) {
                sceneChanger->ChangeScene(ResultScene::StandbyScene());
            }
            else {
                sceneChanger->ChangeScene(resultScene);
            }
        }
    }

    // ============================================================
    // ミスチェック
    // ============================================================
    void NoteManager::CheckMissedNotes() {
        // SongEnd処理
        if (songEndIndex >= 0 && songEndIndex < (int)noteSequence.size()) {
            auto& songEnd = noteSequence[songEndIndex];
            if (!songEnd.judged && songEndIndex < (int)nextIndex) {
                if (songTime > songEnd.hittime + J_WIN_GOOD) {
                    if (!sceneChanger.isNull()) {
                        sf::Mesh::ClearAllRegistered();
                        ShowCursor(TRUE);
                        LoadingScene::SetLoadingType(LoadingType::Common);
                        sceneChanger->ChangeScene(resultScene);
                    }
                    songEnd.judged = true;
                    return;
                }
            }
        }

        // 各レーンのミスチェック
        const int totalLanes = (int)laneOrder.size();

        for (int l = 0; l < totalLanes; ++l) {
            auto& order = laneOrder[l];
            size_t& head = laneHeads[l];

            while (head < order.size()) {
                int idx = order[head];
                if (noteSequence[idx].judged) { ++head; continue; }
                if (idx >= (int)nextIndex) break;

                if (noteSequence[idx].type == NoteType::SongEnd) { ++head; continue; }

                if (songTime > noteSequence[idx].hittime + J_WIN_GOOD) {
                    noteSequence[idx].judged = true;
                    noteSequence[idx].result = JudgeResult::Miss;
                    JudgeStatsService::AddResult(JudgeResult::Miss);

                    if (noteSequence[idx].type == NoteType::HoldStart) {
                        if (noteSequence[idx].pairIndex != -1) {
                            activeHolds[l] = noteSequence[idx].pairIndex;
                            activeHoldNextBeats[l] = std::max(noteSequence[idx].absBeat + 0.5, SecondsToBeat(songTime));
                        }
                    }
                    else if (noteSequence[idx].type == NoteType::HoldEnd) {
                        int startIdx = noteSequence[idx].pairIndex;
                        if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
                            if (auto* act = noteActors[startIdx].Target()) {
                                act->DeActivate();
                                act->Destroy();
                            }
                        }
                        activeHolds[l] = -1;
                    }
                    else {
                        if (auto* act = noteActors[idx].Target()) {
                            act->DeActivate();
                            act->Destroy();
                        }
                    }

                    ++head;
                }
                else {
                    break;
                }
            }
        }
    }

    // ============================================================
    // コンボ管理
    // 判定結果に応じてコンボを更新
    // Perfect/Great/Good: +1、Miss: 0にリセット
    // ============================================================
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

    void NoteManager::AddCombo(int amount) {
        currentCombo += amount;
        JudgeStatsService::AddCombo(amount);
    }

    // ============================================================
    // レーン設定
    // ============================================================
    void NoteManager::SetLaneParams(
        const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
        float laneW_, float laneH_, float rotX_,
        float baseY_, float barRatio_,
        float sideLeftX_, float sideRightX_)
    {
        laneRefs = lanes;
        laneW = laneW_;
        laneH = laneH_;
        rotX = rotX_;
        baseY = baseY_;
        barRatio = barRatio_;
        sideLaneX_Left = sideLeftX_;
        sideLaneX_Right = sideRightX_;
        judgeZ = -laneH * 0.5f + laneH * barRatio;
    }

    void NoteManager::StartGame() {
        isPlaying = true;
    }

    void NoteManager::SetCurrentSongTime(float time) {
        this->songTime = time;
    }

    // ============================================================
    // 時間同期
    // BGMとノーツマネージャーの時間を同期
    // - 大きなズレ（50ms以上）: 即座に合わせる
    // - 小さなズレ: 補間で滑らかに近づける
    // - 音声未開始時: ゲーム時間をそのまま進める
    // ============================================================
    void NoteManager::SyncTime(float time) {
        float diff = time - this->songTime;

        // 大きなズレは即座に合わせる
        if (std::abs(diff) > 0.05f) {
            this->songTime = time;
        }
        else {
            // 音声未開始時は同期しない
            if (time <= 0.001f) {
            }
            else {
                // 小さなズレは補間で滑らかに
                this->songTime += diff * 0.2f;
            }
        }
    }

    // ============================================================
    // ホールドチェック
    // ホールドノーツの継続判定
    // - 押し続けている間: 0.5拍ごとにコンボ加算
    // - 終端到達時: Perfect判定、エフェクト再生
    // - チェーンホールド: 次のホールドを自動判定
    // - リリース時: コンボ加算をスキップ
    // ============================================================
    void NoteManager::CheckHold(int lane, bool isPressed) {
        if (lane < 0 || lane >= (int)activeHolds.size()) return;
        int idx = activeHolds[lane];
        if (idx == -1) return;

        auto& endNote = noteSequence[idx];
        if (endNote.judged) {
            activeHolds[lane] = -1;
            return;
        }

        if (isPressed) {
            // コンボ加算
            double curBeat = SecondsToBeat(songTime);
            while (curBeat >= activeHoldNextBeats[lane] && activeHoldNextBeats[lane] < endNote.absBeat) {
                JudgeResult res = JudgeResult::Perfect;
                JudgeStatsService::AddResult(res);
                UpdateCombo(res);
                activeHoldNextBeats[lane] += 0.5;
            }

            // 終端判定
            if (songTime >= endNote.hittime - J_WIN_PERFECT) {
                JudgeResult res = JudgeResult::Perfect;
                endNote.judged = true;
                endNote.result = res;
                JudgeStatsService::AddResult(res);

                if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
                    sound->Play(GetHitSfxPath(endNote.type));
                }

                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                    float uiLaneWidth = 300.0f;
                    float sideOffset = 250.0f;
                    float hitX = 0.0f;
                    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
                    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
                    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
                    canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
                }

                // 開始アクター破棄
                int startIdx = endNote.pairIndex;
                if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
                    if (auto* act = noteActors[startIdx].Target()) {
                        act->DeActivate();
                        act->Destroy();
                    }
                }

                // チェーンホールド処理
                bool foundChain = false;
                if (lane >= 0 && lane < (int)laneOrder.size()) {
                    auto& order = laneOrder[lane];
                    for (size_t k = laneHeads[lane]; k < order.size(); ++k) {
                        int nextIdx = order[k];
                        auto& nextNote = noteSequence[nextIdx];

                        if (nextNote.judged) continue;
                        if (nextNote.type == NoteType::HoldEnd) continue;

                        if (nextNote.type == NoteType::HoldStart) {
                            float diff = std::abs(nextNote.hittime - songTime);

                            if (diff <= J_WIN_GOOD) {
                                nextNote.judged = true;
                                nextNote.result = JudgeResult::Perfect;
                                JudgeStatsService::AddResult(JudgeResult::Perfect);

                                if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
                                    sound->Play(GetHitSfxPath(NoteType::Tap));
                                }
                                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                    float uiLaneWidth = 300.0f;
                                    float sideOffset = 250.0f;
                                    float hitX = 0.0f;
                                    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
                                    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
                                    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
                                    canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
                                }

                                if (nextNote.pairIndex != -1) {
                                    activeHolds[lane] = nextNote.pairIndex;
                                    activeHoldNextBeats[lane] = nextNote.absBeat + 0.5;
                                    foundChain = true;
                                }
                            }
                        }
                        break;
                    }
                }

                if (!foundChain) {
                    activeHolds[lane] = -1;
                }
            }
        } else {
            // リリース時：ビートを進めてコンボ悪用を防ぐ
            double curBeat = SecondsToBeat(songTime);
            while (curBeat >= activeHoldNextBeats[lane]) {
                activeHoldNextBeats[lane] += 0.5;
            }
        }
    }

    // ============================================================
    // 時間変換
    // TempoServiceに委譲
    // ============================================================
    double NoteManager::SecondsToBeat(double seconds) const {
        return tempoService.SecondsToBeat(seconds, tempoMap);
    }

    // ============================================================
    // インスタンシング初期化
    // タップノーツを一括描画するためのリソースを準備
    // 1. インスタンスバッファ（動的、最大2048ノーツ）
    // 2. キューブメッシュ（頂点・インデックス）
    // 3. シェーダーと入力レイアウト
    // ============================================================
    void NoteManager::InitInstancing()
    {
        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();

        // インスタンスバッファ（動的書き込み用）
        m_instanceDataCPU.reserve(2048);

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(NoteInstanceData) * 2048;  // 最大2048ノーツ
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateBuffer(&desc, nullptr, &m_instanceBuffer);
        if (FAILED(hr)) {
            sf::debug::Debug::LogError("Failed to create Instance Buffer");
        }

        // キューブメッシュの頂点データ（6面×4頂点 = 24頂点）
        struct SimpleVertex {
            float pos[3];
            float nor[3];
            float uv[2];
            float col[4];
        };

        SimpleVertex vertices[] = {
            // 前面
            { {-0.5f, -0.5f, -0.5f}, {0,0,-1}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {0,0,-1}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,0,-1}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,0,-1}, {1,1}, {1,1,1,1} },
            // 背面
            { { 0.5f, -0.5f,  0.5f}, {0,0,1}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,0,1}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,0,1}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f,  0.5f}, {0,0,1}, {1,1}, {1,1,1,1} },
            // 上面
            { {-0.5f,  0.5f, -0.5f}, {0,1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,1,0}, {1,1}, {1,1,1,1} },
            // 下面
            { {-0.5f, -0.5f,  0.5f}, {0,-1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {0,-1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,-1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {0,-1,0}, {1,1}, {1,1,1,1} },
            // 左面
            { {-0.5f, -0.5f,  0.5f}, {-1,0,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {-1,0,0}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {-1,0,0}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {-1,0,0}, {1,1}, {1,1,1,1} },
            // 右面
            { { 0.5f, -0.5f, -0.5f}, {1,0,0}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {1,0,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {1,0,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {1,0,0}, {1,1}, {1,1,1,1} },
        };

        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = sizeof(SimpleVertex) * 24;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vInitData{};
        vInitData.pSysMem = vertices;
        hr = device->CreateBuffer(&vbd, &vInitData, &m_cubeVB);

        // インデックス
        unsigned int indices[] = {
            0,1,2, 0,2,3,
            4,5,6, 4,6,7,
            8,9,10, 8,10,11,
            12,13,14, 12,14,15,
            16,17,18, 16,18,19,
            20,21,22, 20,22,23
        };
        m_cubeIndexCount = 36;

        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = sizeof(unsigned int) * 36;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iInitData{};
        iInitData.pSysMem = indices;
        hr = device->CreateBuffer(&ibd, &iInitData, &m_cubeIB);

        // プリコンパイル済みシェーダー（CSO）をロード
        // Shader::LoadCSOと同じ方式でファイルを読み込む
        FILE* fp = nullptr;
        fopen_s(&fp, "Assets\\Shader\\vsNoteInstancing.cso", "rb");
        if (!fp) {
            sf::debug::Debug::LogError("Failed to open vsNoteInstancing.cso");
            return;
        }

        // ファイルサイズを取得
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // メモリに読み込み
        char* pShaderData = new char[fileSize];
        fread(pShaderData, fileSize, 1, fp);
        fclose(fp);

        hr = device->CreateVertexShader(pShaderData, fileSize, nullptr, &m_vs);
        if (FAILED(hr)) {
            delete[] pShaderData;
            sf::debug::Debug::LogError("Failed to create vertex shader from CSO");
            return;
        }

        // 入力レイアウト
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        hr = device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), pShaderData, fileSize, &m_layout);
        delete[] pShaderData;
    }

    // ============================================================
    // インスタンスバッファ更新
    // 可視なタップノーツのワールド行列を収集し、GPUに転送
    // - 判定済みノーツはスキップ
    // - Hold/Skill/SongEndは個別描画なのでスキップ
    // ============================================================
    void NoteManager::UpdateInstanceBuffer()
    {
        m_instanceDataCPU.clear();

        // スポーン済みノーツのみチェック
        size_t maxIdx = std::min(nextIndex, noteActors.size());
        for (size_t i = 0; i < maxIdx; ++i) {
            if (i >= noteSequence.size()) continue;
            
            // 判定済みはスキップ（破棄済み）
            if (noteSequence[i].judged) continue;

            // ホールド・スキル・終了ノーツは個別描画
            if (noteSequence[i].type == NoteType::HoldStart ||
                noteSequence[i].type == NoteType::HoldEnd ||
                noteSequence[i].type == NoteType::Skill ||
                noteSequence[i].type == NoteType::SongEnd) continue;

            // 無効なアクターはスキップ
            if (noteActors[i].IsNull()) continue;
            if (!noteActors[i].IsValid()) continue;

            auto act = noteActors[i].Target();
            if (!act) continue;

            // ワールド行列を収集
            NoteInstanceData data;
            DirectX::XMMATRIX m = act->transform.Matrix();
            DirectX::XMStoreFloat4x4(&data.world, m);
            data.color = { 1, 1, 1, 1 };
            m_instanceDataCPU.push_back(data);
        }

        if (m_instanceDataCPU.empty()) return;

        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetContext();

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            size_t sizeInBytes = sizeof(NoteInstanceData) * std::min(m_instanceDataCPU.size(), (size_t)2048);
            memcpy(mapped.pData, m_instanceDataCPU.data(), sizeInBytes);
            context->Unmap(m_instanceBuffer, 0);
        }
    }

    // ============================================================
    // インスタンス描画
    // 収集したノーツデータを一括描画
    // DrawIndexedInstancedで全ノーツを1回のDrawCallで描画
    // ============================================================
    void NoteManager::DrawInstanced()
    {
        try {
            // バッファ更新
            UpdateInstanceBuffer();
            if (m_instanceDataCPU.empty()) return;
            if (!m_vs || !m_layout || !m_cubeVB || !m_cubeIB) return;

            sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
            if (!dx11) return;

            auto context = dx11->GetMainDevice().GetContext();

            // ★デバイス/コンテキストの有効性チェック（Intel GPUクラッシュ防止）
            if (!context) {
                sf::debug::Debug::LogError("DrawInstanced: Context is null, skipping draw.");
                return;
            }

            // シェーダー設定
            context->VSSetShader(m_vs, nullptr, 0);
            context->IASetInputLayout(m_layout);
            dx11->ps3d.SetGPU(dx11->GetMainDevice());
            dx11->gsNone.SetGPU(dx11->GetMainDevice());

            // 頂点バッファ設定（キューブ + インスタンス）
            UINT strides[2] = { sizeof(float) * 12, sizeof(NoteInstanceData) };
            UINT offsets[2] = { 0, 0 };
            ID3D11Buffer* buffers[2] = { m_cubeVB, m_instanceBuffer };

            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetIndexBuffer(m_cubeIB, DXGI_FORMAT_R32_UINT, 0);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // マテリアル設定
            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            material.textureEnable = { 0.0f, 0.0f, 0.0f, 0.0f };
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            // インスタンス描画（1 DrawCallで全ノーツ）
            context->DrawIndexedInstanced(m_cubeIndexCount, (UINT)m_instanceDataCPU.size(), 0, 0, 0);
        } catch (...) {
            sf::debug::Debug::LogError("DrawInstanced Exception");
        }
    }

} // namespace app::test
