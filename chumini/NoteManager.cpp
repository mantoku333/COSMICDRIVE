#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include "TestScene.h"
#include "NoteData.h"
#include "SoundComponent.h"
#include "SongInfo.h"
#include <numeric>
#include <vector>
#include <cmath>

namespace app::test {

    namespace {
        // ノーツタイプごとの効果音パス（必要に応じて実ファイルに合わせて編集）
        const char* GetHitSfxPath(app::test::NoteType t) {
            using app::test::NoteType;
            switch (t) {
            case NoteType::Tap:       return "Assets\\sound\\tap.wav";
            //case NoteType::HoldStart: return "Assets\\sound\\hold_start.wav";
            //case NoteType::HoldEnd:   return "Assets\\sound\\hold_end.wav";
            //case NoteType::SongEnd:   return "Assets\\sound\\clear.wav";
            default:                  return "Assets\\sound\\tap.wav";
            }
        }
    }

    // ====== 曲・判定の基本設定（必要に応じて外出し可） ======
    static constexpr int    K_BEATS_PER_MEASURE = 4;      // 4/4
    static constexpr double K_DEFAULT_BPM = 120.0;  // メタ行が無い場合のデフォルト
    static constexpr double K_OFFSET_SEC = 0.0;    // 曲頭オフセット（必要なら±）
    static constexpr float  INPUT_OFFSET_SEC = 0.0f;   // 入力遅延補正（必要ならUIで）

    // 判定窓（秒）
    static constexpr float J_WIN_PERFECT = 0.050f;
    static constexpr float J_WIN_GREAT = J_WIN_PERFECT + 0.030f;
    static constexpr float J_WIN_GOOD = J_WIN_GREAT + 0.030f;

    // 判定チューニング（同時押し・高速トリル耐性）
    static constexpr int   J_LOOKAHEAD_NOTES = 5;      // 先読み件数（2〜6で調整）
    static constexpr float J_ASSIGN_EARLY_BIAS = 0.005f; // 未来ノーツに軽いペナルティ（“早い側優先”）
    static constexpr float J_POS_TOL_MULT = 2.0f;   // 位置許容の緩和倍率（×2〜3推奨）

    // ---- テンポマップ（可変BPM対応） ----
    struct TempoEvent {
        double atBeat = 0.0;     // この拍からテンポ適用（絶対拍）
        double bpm = K_DEFAULT_BPM;
        double spb = 60.0 / K_DEFAULT_BPM; // seconds per beat
        double atSec = 0.0;     // この拍の絶対秒（累積）
    };

    // atBeat 昇順に整列した tempoMap を前計算して、各区間の atSec を埋める
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

    // 絶対拍 absBeat をテンポマップで秒へ変換
    static double BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) {
        // upper_bound 的な2分探索
        size_t lo = 0, hi = tm.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tm[mid].atBeat <= absBeat) lo = mid; else hi = mid;
        }
        const TempoEvent& seg = tm[lo];
        const double dBeat = absBeat - seg.atBeat;
        return seg.atSec + dBeat * seg.spb;
    }

    // ---- 内部ヘルパ（レーンごとの先頭未判定ノーツを管理） ----
    // Note: laneHeads[l] は laneOrder[l] の “現在先頭” を指すインデックス
    // laneOrder[l] は noteSequence の“インデックス”列（時間順）
    // nextIndex は「アクティベート済みの全体先頭+α」を指す（既存のまま）

    void NoteManager::Begin() {
        auto owner = actorRef.Target(); if (!owner) return;
        auto* scene = &owner->GetScene();

        // 譜面パス取得
        std::string chartPath = "Save/chart/Sample.chart";
        if (auto* testScene = dynamic_cast<TestScene*>(scene)) {
            const SongInfo& selectedSong = testScene->GetSelectedSong();
            if (!selectedSong.chartPath.empty()) chartPath = selectedSong.chartPath;
        }

        // 読み込み
        // 判定処理を演奏開始からの秒数で統一したいので、MBTからhittimeを算出する
        noteSequence.clear();

        // ★ テンポマップ（可変BPM）読み取りバッファ
        std::vector<TempoEvent> tempoMap;
        bool sawAnyTempoMeta = false;

        {
            std::ifstream file(chartPath);
            if (!file) {
                sf::debug::Debug::Log("ふぁいるがみつかりません: " + chartPath);
                return;
            }
            std::string line;
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                if (!s.empty()) s.erase(s.find_last_not_of(" \t\r\n") + 1);
                };

            while (std::getline(file, line)) {
                trim(line);
                if (line.empty() || line.rfind("//", 0) == 0) continue;

                // ---- メタ行（BPM関連）を先に処理 ----
                if (line[0] == '#') {
                    // 例) #BPM, 128
                    //     #BPMCHANGE, atBeat, bpm
                    std::istringstream ms(line);
                    std::string tok; std::vector<std::string> m;
                    while (std::getline(ms, tok, ',')) { trim(tok); if (!tok.empty()) m.push_back(tok); }
                    if (!m.empty()) {
                        const std::string& tag = m[0];
                        if (tag == "#BPM" && m.size() >= 2) {
                            double bpm = std::stod(m[1]);
                            tempoMap.push_back({ 0.0, bpm, 60.0 / bpm, 0.0 });
                            sawAnyTempoMeta = true;
                            continue;
                        }
                        if (tag == "#BPMCHANGE" && m.size() >= 3) {
                            double atBeat = std::stod(m[1]);
                            double bpm = std::stod(m[2]);
                            tempoMap.push_back({ atBeat, bpm, 60.0 / bpm, 0.0 });
                            sawAnyTempoMeta = true;
                            continue;
                        }
                    }
                    // 他メタはスルー
                    continue;
                }

                // ---- ノーツ行： lane, measure, beat, tick16, type ----
                std::istringstream ss(line);
                std::string tok; std::vector<std::string> t;
                while (std::getline(ss, tok, ',')) { trim(tok); if (!tok.empty()) t.push_back(tok); }
                if (t.size() != 5) continue;

                try {
                    NoteData nd;
                    nd.lane = std::stoi(t[0]);
                    nd.mbt.measure = std::stoi(t[1]);
                    nd.mbt.beat = std::stoi(t[2]);      // 0..(K_BEATS_PER_MEASURE-1)
                    nd.mbt.tick16 = std::stoi(t[3]);      // 0..15 を想定
                    nd.type = static_cast<NoteType>(std::stoi(t[4]));

                    // クランプ
                    nd.mbt.beat = std::clamp(nd.mbt.beat, 0, K_BEATS_PER_MEASURE - 1);
                    nd.mbt.tick16 = std::clamp(nd.mbt.tick16, 0, 15);

                    // 通し拍（4/4 固定前提。拍子変更が必要になったらここを TimeSig イベントで拡張）
                    const double absBeat =
                        static_cast<double>(nd.mbt.measure) * K_BEATS_PER_MEASURE
                        + static_cast<double>(nd.mbt.beat)
                        + static_cast<double>(nd.mbt.tick16) / 16.0;

                    nd.absBeat = absBeat;

                    // hittime は後段で TempoMap から設定
                    noteSequence.push_back(nd);
                }
                catch (const std::exception&) {
                    sf::debug::Debug::Log("パースに失敗しました: " + chartPath);
                }
            }
        }

        // テンポマップ最終化（無ければデフォルトBPM）
        if (!sawAnyTempoMeta) {
            tempoMap.clear();
            tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
        }
        BuildTempoMap(tempoMap);

        // absBeat → 秒 に置換（K_OFFSET_SEC は最後に足す）
        for (auto& nd : noteSequence) {
            nd.hittime = static_cast<float>(BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC);
        }

        // ③ Yジオメトリとリードタイム
        float startY = 0.0f, hitY = 0.0f;
        int lanes = 1;
        if (auto* ts = dynamic_cast<TestScene*>(scene)) {
            startY = ts->panelPos.y + ts->panelH / 2.f;
            hitY = ts->barY;
            lanes = std::max(1, ts->lanes);
        }

        // 時間順にソート
        std::vector<int> idxs(noteSequence.size());
        std::iota(idxs.begin(), idxs.end(), 0);
        std::sort(idxs.begin(), idxs.end(), [&](int a, int b) {
            return noteSequence[a].hittime < noteSequence[b].hittime;
            });

        // ソートに合わせてnoteSequenceを並べ替える　
        std::vector<NoteData> sorted;
        sorted.reserve(noteSequence.size());
        for (int i : idxs) sorted.push_back(noteSequence[i]);
        noteSequence.swap(sorted);

        float totalDistance = std::abs(startY - hitY);
        leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
        songTime = -leadTime;

        // 先に全ノーツを生成し非アクティブで保持
        noteActors.clear();
        nextIndex = 0;
        currentCombo = 0;

        for (size_t i = 0; i < noteSequence.size(); ++i) {
            auto noteActor = scene->Instantiate();
            auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
            mesh->SetGeometry(g_cube);
            mesh->material.SetColor({ 1,1,1,1 });
            noteActor.Target()->transform.SetScale({ 3.0f,0.5f,0.1f });

            auto comp = noteActor.Target()->AddComponent<NoteComponent>();
            comp->info = noteSequence[i];
            comp->leadTime = leadTime;
            comp->noteSpeed = noteSpeed;

            noteActor.Target()->DeActivate();
            noteActors.push_back(noteActor);
        }

        // ⑤ レーンごとの順序テーブル（判定をO(1)化）
        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }

        // Update登録
        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));
    }

    void NoteManager::Update(const sf::command::ICommand&) {
        songTime += sf::Time::DeltaTime();

        // 出番が来たノーツだけアクティブ化（i < nextIndex が “Active” の代替指標）
        while (nextIndex < noteSequence.size()
            && songTime + leadTime >= noteSequence[nextIndex].hittime)
        {
            auto* act = noteActors[nextIndex].Target();
            if (act) {
                act->Activate();
                if (auto* comp = act->GetComponent<NoteComponent>()) comp->Activate();
            }
            ++nextIndex;
        }

        CheckMissedNotes();
    }

    // 秒差ベースのシンプル判定
    JudgeResult NoteManager::JudgeNote(NoteType, float noteTime, float inputTime) {
        inputTime += INPUT_OFFSET_SEC;
        float diff = std::abs(noteTime - inputTime);
        if (diff <= J_WIN_PERFECT) return JudgeResult::Perfect;
        if (diff <= J_WIN_GREAT)   return JudgeResult::Great;
        if (diff <= J_WIN_GOOD)    return JudgeResult::Good;
        return JudgeResult::Miss;
    }

    // ★レーンキュー方式：同時押し・16分トリル耐性版
    JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
        auto owner = actorRef.Target(); if (!owner) return JudgeResult::Skip;
        auto* ts = dynamic_cast<TestScene*>(&owner->GetScene()); if (!ts) return JudgeResult::Skip;

        // レーン範囲
        if (lane < 0 || lane >= (int)laneOrder.size()) return JudgeResult::Skip;

        auto& order = laneOrder[lane];
        size_t& head = laneHeads[lane];

        // (A) 入力時点で Good 窓を既に超えている先頭は即時 Miss にしてどかす（フレーム待ちしない）
        while (head < order.size()) {
            int i = order[head];
            if (noteSequence[i].judged) { ++head; continue; }
            if (i >= (int)nextIndex) break; // まだ画面外（未Activate）

            const float hit = noteSequence[i].hittime;
            if (inputTime > hit + J_WIN_GOOD) {
                noteSequence[i].judged = true;
                noteSequence[i].result = JudgeResult::Miss;
                UpdateCombo(JudgeResult::Miss);
                ++head;
                continue;
            }
            break;
        }
        if (head >= order.size()) return JudgeResult::Skip;

        // (B) 先頭から数件先まで見る：時間優先＋“早いノーツ”をわずかに優遇
        int   bestIdx = -1;
        float bestScore = 1e9f;

        for (int k = 0; k < J_LOOKAHEAD_NOTES && head + k < order.size(); ++k) {
            int ii = order[head + k];
            if (noteSequence[ii].judged) continue;
            if (ii >= (int)nextIndex) break; // これ以降は未Activate

            const float hit = noteSequence[ii].hittime;
            const float tdiff = std::abs(hit - inputTime);
            if (tdiff > J_WIN_GOOD) continue; // 時間的に窓外

            // 位置はソフト制約：厳しすぎるとフレーム抜けを起こすので少し緩和
            if (auto* act = noteActors[ii].Target()) {
                const float dy = std::abs(act->transform.GetPosition().y - ts->barY);
                if (dy > judgeRange * J_POS_TOL_MULT) continue;
            }

            // “早いノーツ優遇”のスコア：未来ノーツには小さなペナルティ
            const bool  isFuture = (hit > inputTime);
            const float score = tdiff + (isFuture ? J_ASSIGN_EARLY_BIAS : 0.0f);

            if (score < bestScore) {
                bestScore = score;
                bestIdx = ii;
            }
        }

        if (bestIdx < 0) {
            // 候補なし → この入力では判定しない
            return JudgeResult::Skip;
        }

        // (C) 選ばれたノーツを時間判定
        JudgeResult res = JudgeNote(noteSequence[bestIdx].type, noteSequence[bestIdx].hittime, inputTime);
        noteSequence[bestIdx].judged = true;
        noteSequence[bestIdx].result = res;
        UpdateCombo(res);

        /*if (auto* sound = owner->GetComponent<SoundComponent>()) {
            sound->Play(GetHitSfxPath(noteSequence[bestIdx].type));
        }*/

        // (D) 先頭ポインタを前進
        while (head < order.size() && noteSequence[order[head]].judged) ++head;

        // ログ
        {
            std::ostringstream oss;
            oss << "[JudgeLane] lane=" << lane
                << " idx=" << bestIdx
                << " input=" << inputTime
                << " hit=" << noteSequence[bestIdx].hittime
                << " res=" << (int)res;
            sf::debug::Debug::Log(oss.str());
        }

        return res;
    }

    // ★同時押し一括判定（押下レーン配列を渡す）
    void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
        // まず全レーンで“時間早送り”を適用（Good窓を過ぎた先頭は即 Miss）
        for (int l : pressedLanes) {
            if (l < 0 || l >= (int)laneOrder.size()) continue;
            auto& order = laneOrder[l];
            size_t& head = laneHeads[l];
            while (head < order.size()) {
                int i = order[head];
                if (noteSequence[i].judged) { ++head; continue; }
                if (i >= (int)nextIndex) break; // 未Activate

                if (inputTime > noteSequence[i].hittime + J_WIN_GOOD) {
                    noteSequence[i].judged = true;
                    noteSequence[i].result = JudgeResult::Miss;
                    UpdateCombo(JudgeResult::Miss);
                    ++head;
                    continue;
                }
                break;
            }
        }
        // その上で各レーン独立に判定（ブロックが外れているので取りこぼしにくい）
        for (int l : pressedLanes) {
            JudgeLane(l, inputTime);
        }
    }

    // ★見逃しは “時間基準”：Good窓を超えたらMiss
    void NoteManager::CheckMissedNotes() {
        // 各レーンの先頭未判定だけを見る（O(レーン数)）
        for (int l = 0; l < (int)laneOrder.size(); ++l) {
            auto& order = laneOrder[l];
            size_t& head = laneHeads[l];

            while (head < order.size()) {
                int i = order[head];
                if (noteSequence[i].judged) { ++head; continue; }

                // 未Activate（画面外）は見逃し対象にしない
                if (i >= (int)nextIndex) break;

                if (songTime > noteSequence[i].hittime + J_WIN_GOOD) {
                    noteSequence[i].judged = true;
                    noteSequence[i].result = JudgeResult::Miss;
                    UpdateCombo(JudgeResult::Miss);

                    std::ostringstream oss;
                    oss << "Missed: lane=" << l
                        << " idx=" << i
                        << " t=" << songTime
                        << " hit=" << noteSequence[i].hittime;
                    sf::debug::Debug::Log(oss.str());

                    ++head;
                }
                else {
                    // まだ判定窓内
                    break;
                }
            }
        }
    }

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
        std::ostringstream oss; oss << "[Combo] " << currentCombo;
        sf::debug::Debug::Log(oss.str());
    }

    int NoteManager::GetCurrentCombo() const { return currentCombo; }

} // namespace app::test
