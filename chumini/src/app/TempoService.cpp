#include "TempoService.h"

namespace app::test {

    // ============================================================
    // テンポマップ構築
    // BPM変化イベントを時間順にソートし、
    // 各イベントの開始秒数（atSec）を計算する
    // ============================================================
    void TempoService::BuildTempoMap(std::vector<TempoEvent>& tm) const {
        // テンポイベントがない場合はデフォルトBPMを設定
        if (tm.empty()) {
            tm.push_back({ 0.0, DEFAULT_BPM, 60.0 / DEFAULT_BPM, 0.0 });
            return;
        }

        // 拍数順にソート
        std::sort(tm.begin(), tm.end(), [](const TempoEvent& a, const TempoEvent& b) {
            return a.atBeat < b.atBeat;
        });

        // 最初のイベントは0秒から開始
        tm[0].spb = 60.0 / tm[0].bpm;
        tm[0].atSec = 0.0;

        // 各イベントの開始秒数を累積計算
        for (size_t i = 1; i < tm.size(); ++i) {
            tm[i].spb = 60.0 / tm[i].bpm;
            const double beatsSpan = tm[i].atBeat - tm[i - 1].atBeat;
            tm[i].atSec = tm[i - 1].atSec + beatsSpan * tm[i - 1].spb;
        }
    }

    // ============================================================
    // 拍数を秒数に変換
    // テンポマップを二分探索して該当セグメントを特定
    // ============================================================
    double TempoService::BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) const {
        if (tm.empty()) return 0.0;

        size_t lo = 0, hi = tm.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tm[mid].atBeat <= absBeat) lo = mid; else hi = mid;
        }
        const TempoEvent& seg = tm[lo];
        const double dBeat = absBeat - seg.atBeat;
        return seg.atSec + dBeat * seg.spb;
    }

    // ============================================================
    // 秒数を拍数に変換
    // テンポマップを二分探索して該当セグメントを特定
    // ============================================================
    double TempoService::SecondsToBeat(double seconds, const std::vector<TempoEvent>& tm) const {
        if (tm.empty()) return 0.0;

        size_t lo = 0, hi = tm.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tm[mid].atSec <= seconds) lo = mid; else hi = mid;
        }

        if (lo >= tm.size()) return 0.0;

        const auto& seg = tm[lo];
        if (seg.spb <= 0.000001) return seg.atBeat;

        return seg.atBeat + (seconds - seg.atSec) / seg.spb;
    }

} // namespace app::test
