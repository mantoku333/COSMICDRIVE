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
#include <vector>
#include <cmath>

namespace app::test {

    namespace {
        const char* GetHitSfxPath(app::test::NoteType t) {
            using app::test::NoteType;
            switch (t) {
            case NoteType::Tap:       return "Assets\\sound\\tap.wav";
            default:                  return "Assets\\sound\\tap.wav";
            }
        }
    }

    // ====== 基本設定 ======
    static constexpr int    K_BEATS_PER_MEASURE = 4;
    static constexpr double K_DEFAULT_BPM = 120.0;
    static constexpr double K_OFFSET_SEC = 0.0;
    static constexpr float  INPUT_OFFSET_SEC = 0.0f;

    static constexpr float J_WIN_PERFECT = 0.050f;
    static constexpr float J_WIN_GREAT = J_WIN_PERFECT + 0.030f;
    static constexpr float J_WIN_GOOD = J_WIN_GREAT + 0.030f;

    static constexpr int   J_LOOKAHEAD_NOTES = 5;
    static constexpr float J_ASSIGN_EARLY_BIAS = 0.005f;
    static constexpr float J_POS_TOL_MULT = 2.0f;

    struct TempoEvent {
        double atBeat = 0.0;
        double bpm = K_DEFAULT_BPM;
        double spb = 60.0 / K_DEFAULT_BPM;
        double atSec = 0.0;
    };

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
            double beatsSpan = tm[i].atBeat - tm[i - 1].atBeat;
            tm[i].atSec = tm[i - 1].atSec + beatsSpan * tm[i - 1].spb;
        }
    }

    static double BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) {
        size_t lo = 0, hi = tm.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tm[mid].atBeat <= absBeat) lo = mid; else hi = mid;
        }
        const TempoEvent& seg = tm[lo];
        double dBeat = absBeat - seg.atBeat;
        return seg.atSec + dBeat * seg.spb;
    }

    // ==================================================
    void NoteManager::Begin() {
        auto owner = actorRef.Target(); if (!owner) return;
        auto* scene = &owner->GetScene();

        // --- 譜面パス ---
        std::string chartPath = "Save/chart/Sample.chart";
        if (auto* testScene = dynamic_cast<TestScene*>(scene)) {
            const SongInfo& selectedSong = testScene->GetSelectedSong();
            if (!selectedSong.chartPath.empty())
                chartPath = selectedSong.chartPath;
        }

        // --- 譜面読み込み ---
        noteSequence.clear();
        std::vector<TempoEvent> tempoMap;
        bool sawAnyTempoMeta = false;

        {
            std::ifstream file(chartPath);
            if (!file) {
                sf::debug::Debug::Log("譜面ファイルが見つかりません: " + chartPath);
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

                // BPM メタ処理
                if (line[0] == '#') {
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
                    continue;
                }

                // ノーツ行
                std::istringstream ss(line);
                std::string tok; std::vector<std::string> t;
                while (std::getline(ss, tok, ',')) { trim(tok); if (!tok.empty()) t.push_back(tok); }
                if (t.size() != 5) continue;

                try {
                    NoteData nd;
                    nd.lane = std::stoi(t[0]);
                    nd.mbt.measure = std::stoi(t[1]);
                    nd.mbt.beat = std::stoi(t[2]);
                    nd.mbt.tick16 = std::stoi(t[3]);
                    nd.type = static_cast<NoteType>(std::stoi(t[4]));

                    nd.mbt.beat = std::clamp(nd.mbt.beat, 0, K_BEATS_PER_MEASURE - 1);
                    nd.mbt.tick16 = std::clamp(nd.mbt.tick16, 0, 15);

                    double absBeat =
                        static_cast<double>(nd.mbt.measure) * K_BEATS_PER_MEASURE
                        + static_cast<double>(nd.mbt.beat)
                        + static_cast<double>(nd.mbt.tick16) / 16.0;

                    nd.absBeat = absBeat;
                    noteSequence.push_back(nd);
                }
                catch (...) {
                    sf::debug::Debug::Log("譜面行パース失敗: " + chartPath);
                }
            }
        }

        // --- テンポ確定 ---
        if (!sawAnyTempoMeta) tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
        BuildTempoMap(tempoMap);

        for (auto& nd : noteSequence)
            nd.hittime = static_cast<float>(BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC);

        // --- 傾斜レーン情報 ---
        float slopeRad = rotX * 3.14159265f / 180.0f;
        float startZ = laneH * 0.5f;                     // 奥から出る
        float hitZ = -laneH * 0.5f + laneH * barRatio; // バー位置
        float startY = baseY - std::tan(slopeRad) * startZ;
        float hitY = baseY - std::tan(slopeRad) * hitZ;

        int lanes = std::max(1, (int)laneRefs.size());
        float totalDistance = std::abs(startZ - hitZ);
        leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
        songTime = -leadTime;

        // --- ノーツ生成 ---
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

        // --- レーン順序 ---
        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }

        // --- 確認ログ ---
        sf::debug::Debug::Log("[NoteManager] init done: "
            + std::to_string(noteActors.size()) + " notes, "
            + std::to_string(laneRefs.size()) + " lanes, "
            + "leadTime=" + std::to_string(leadTime));

        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));
    }

    void NoteManager::Update(const sf::command::ICommand&) {
        songTime += sf::Time::DeltaTime();

        while (nextIndex < noteSequence.size()
            && songTime + leadTime >= noteSequence[nextIndex].hittime)
        {
            auto* act = noteActors[nextIndex].Target();
            if (act) act->Activate();
            sf::debug::Debug::Log("[Note] Activate index=" + std::to_string(nextIndex));
            ++nextIndex;
        }
        CheckMissedNotes();
    }

    void NoteManager::CheckMissedNotes() {
        // 仮実装（後で判定処理と結合）
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
    }

    int NoteManager::GetCurrentCombo() const { return currentCombo; }

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
        judgeZ = -laneH * 0.5f + laneH * barRatio;
    }

    app::test::JudgeResult app::test::NoteManager::JudgeLane(int lane, float inputTime)
    {
        if (lane < 0 || lane >= (int)laneOrder.size()) return JudgeResult::Skip;
        auto& order = laneOrder[lane];
        size_t& head = laneHeads[lane];

        // 判定済みをスキップ
        while (head < order.size() && noteSequence[order[head]].judged)
            ++head;
        if (head >= order.size()) return JudgeResult::Skip;

        int idx = order[head];
        const float hitTime = noteSequence[idx].hittime;
        const float diff = inputTime - hitTime;

        JudgeResult res = JudgeResult::Skip;
        if (std::abs(diff) <= J_WIN_PERFECT) res = JudgeResult::Perfect;
        else if (std::abs(diff) <= J_WIN_GREAT) res = JudgeResult::Great;
        else if (std::abs(diff) <= J_WIN_GOOD) res = JudgeResult::Good;
        else if (diff > J_WIN_GOOD) res = JudgeResult::Miss;

        if (res != JudgeResult::Skip)
        {
            noteSequence[idx].judged = true;
            noteSequence[idx].result = res;
            UpdateCombo(res);

            sf::debug::Debug::Log(
                "[Judge] lane=" + std::to_string(lane) +
                " result=" + std::to_string((int)res) +
                " diff=" + std::to_string(diff)
            );

            ++head;
        }

        return res;
    }
} // namespace app::test
