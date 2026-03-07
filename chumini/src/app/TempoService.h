#pragma once
#include <vector>
#include <algorithm>

namespace app::test {

    // ============================================================
    // BPM変化対応用の構造体
    // 曲中でBPMが変化する場合、各変化点の情報を保持
    // ============================================================
    struct TempoEvent {
        double atBeat = 0.0;  ///< この変化が起きる拍数
        double bpm = 120.0;   ///< 新しいテンポ（BPM）
        double spb = 0.5;     ///< Seconds Per Beat（1拍あたりの秒数）
        double atSec = 0.0;   ///< この変化が起きる秒数（計算で求める）
    };

    // ============================================================
    // TempoService - テンポ管理サービス
    // 
    // 責務:
    // - テンポマップの構築（BPM変化イベントのソート・累積計算）
    // - 拍数⇔秒数の相互変換
    // ============================================================
    class TempoService {
    public:
        /// デフォルトBPM
        static constexpr double DEFAULT_BPM = 120.0;

        /// テンポマップを構築
        /// BPM変化イベントを時間順にソートし、各イベントの開始秒数を計算
        /// @param tm テンポイベントリスト（入出力）
        void BuildTempoMap(std::vector<TempoEvent>& tm) const;

        /// 拍数を秒数に変換
        /// @param absBeat 絶対拍数
        /// @param tm テンポマップ
        /// @return 秒数
        double BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) const;

        /// 秒数を拍数に変換
        /// @param seconds 秒数
        /// @param tm テンポマップ
        /// @return 拍数
        double SecondsToBeat(double seconds, const std::vector<TempoEvent>& tm) const;
    };

} // namespace app::test
