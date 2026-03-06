#include "ChartLoader.h"
#include "ChedParser.h"
#include "StringUtils.h"
#include "Debug.h"
#include <algorithm>
#include <map>

namespace app::test {

    // 定数
    static constexpr int K_BEATS_PER_MEASURE = 4;
    static constexpr double K_DEFAULT_BPM = 120.0;

    // ============================================================
    // 譜面読み込み
    // ============================================================
    ChartLoadResult ChartLoader::Load(const std::string& chartPath,
                                       double offsetSec,
                                       double gameOffsetSec) {
        ChartLoadResult result;
        result.noteOffset = 0.0;

        // 譜面ファイル読み込み
        ChedParser parser;
        std::wstring wChartPath = sf::util::Utf8ToWstring(chartPath);
        if (parser.Load(wChartPath)) {
            sf::debug::Debug::Log("Chart Loaded: " + chartPath);
        } else {
            sf::debug::Debug::Log("Chart Load Failed: " + chartPath);
            return result;
        }

        // テンポイベント取得
        for (const auto& te : parser.tempos) {
            TempoEvent e;
            e.atBeat = te.absBeat;
            e.bpm = te.bpm;
            e.spb = 60.0 / e.bpm;
            e.atSec = 0.0;
            result.tempoMap.push_back(e);
        }

        // デフォルトテンポ
        if (result.tempoMap.empty()) {
            result.tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
        }
        tempoService.BuildTempoMap(result.tempoMap);

        // ホールドノーツのペアリング用マップ
        std::map<int, size_t> pendingHolds;

        // ノーツ変換
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
                    result.notes[startIdx].type = NoteType::HoldStart;
                    result.notes[startIdx].pairIndex = (int)result.notes.size();
                    nd.type = NoteType::HoldEnd;
                    nd.pairIndex = (int)startIdx;
                    pendingHolds.erase(key);
                } else {
                    nd.type = NoteType::HoldStart;
                    pendingHolds[key] = result.notes.size();
                }
            }

            result.notes.push_back(nd);
        }

        // ヒット時間計算
        CalculateHitTimes(result, offsetSec, gameOffsetSec);

        // 時間順ソート
        std::stable_sort(result.notes.begin(), result.notes.end(),
            [](const NoteData& a, const NoteData& b) {
                return a.hittime < b.hittime;
            });

        // ペアリング再構築
        RebuildPairIndices(result);

        // ホールド長さ計算
        CalculateHoldDurations(result);

        // SongEndインデックス検索
        for (int i = 0; i < (int)result.notes.size(); ++i) {
            if (result.notes[i].type == NoteType::SongEnd) {
                result.songEndIndex = i;
                break;
            }
        }

        // 最大コンボ計算
        CalculateMaxCombo(result);

        return result;
    }

    // ============================================================
    // ヒット時間計算
    // ============================================================
    void ChartLoader::CalculateHitTimes(ChartLoadResult& result,
                                         double offsetSec,
                                         double gameOffsetSec) {
        for (auto& nd : result.notes) {
            nd.hittime = static_cast<float>(
                tempoService.BeatToSeconds(nd.absBeat, result.tempoMap) 
                + offsetSec 
                + result.noteOffset 
                + gameOffsetSec
            );
        }
    }

    // ============================================================
    // ペアインデックス再構築（ソート後）
    // ============================================================
    void ChartLoader::RebuildPairIndices(ChartLoadResult& result) {
        std::vector<int> pendingStartIdx(6, -1);
        for (int i = 0; i < (int)result.notes.size(); ++i) {
            result.notes[i].pairIndex = -1;

            if (result.notes[i].type == NoteType::HoldStart) {
                pendingStartIdx[result.notes[i].lane] = i;
            }
            else if (result.notes[i].type == NoteType::HoldEnd) {
                int startIdx = pendingStartIdx[result.notes[i].lane];
                if (startIdx != -1) {
                    result.notes[startIdx].pairIndex = i;
                    result.notes[i].pairIndex = startIdx;
                    pendingStartIdx[result.notes[i].lane] = -1;
                }
            }
        }
    }

    // ============================================================
    // ホールド長さ計算
    // ============================================================
    void ChartLoader::CalculateHoldDurations(ChartLoadResult& result) {
        for (auto& nd : result.notes) {
            if (nd.type == NoteType::HoldStart && nd.pairIndex != -1) {
                if (nd.pairIndex < (int)result.notes.size()) {
                    nd.duration = result.notes[nd.pairIndex].hittime - nd.hittime;
                }
            }
        }
    }

    // ============================================================
    // 最大コンボ計算
    // ============================================================
    void ChartLoader::CalculateMaxCombo(ChartLoadResult& result) {
        result.maxTotalCombo = 0;
        for (const auto& nd : result.notes) {
            if (nd.type == NoteType::SongEnd) continue;

            if (nd.type == NoteType::Tap || 
                nd.type == NoteType::Skill || 
                nd.type == NoteType::HoldStart) {
                result.maxTotalCombo++;
            }

            if (nd.type == NoteType::HoldStart && 
                nd.pairIndex != -1 && 
                nd.pairIndex < (int)result.notes.size()) {
                const auto& endNote = result.notes[nd.pairIndex];
                double startBeat = nd.absBeat;
                double endBeat = endNote.absBeat;
                double nextBeat = startBeat + 0.5;

                while (nextBeat < endBeat) {
                    result.maxTotalCombo++;
                    nextBeat += 0.5;
                }
                result.maxTotalCombo++;
            }
        }
    }

} // namespace app::test
