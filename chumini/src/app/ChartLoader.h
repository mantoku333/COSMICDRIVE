#pragma once
#include <vector>
#include <string>
#include "NoteData.h"
#include "TempoService.h"

namespace app::test {

    // ============================================================
    // 譜面読み込み結果
    // ============================================================
    struct ChartLoadResult {
        std::vector<NoteData> notes;       ///< ノーツデータ（時間順ソート済み）
        std::vector<TempoEvent> tempoMap;  ///< テンポマップ
        double noteOffset = 0.0;           ///< 譜面オフセット
        int maxTotalCombo = 0;             ///< 最大コンボ数
        int songEndIndex = -1;             ///< SongEndノーツのインデックス
    };

    // ============================================================
    // ChartLoader - 譜面読み込みサービス
    // 
    // 責務:
    // - 譜面ファイル（.ched）の読み込みとパース
    // - ノーツデータの変換とホールドペアリング
    // - ヒット時間の計算
    // - 最大コンボ数の計算
    // ============================================================
    class ChartLoader {
    public:
        /// 譜面を読み込む
        /// @param chartPath 譜面ファイルパス
        /// @param offsetSec グローバルオフセット（秒）
        /// @param gameOffsetSec ゲーム設定オフセット（秒）
        /// @return 読み込み結果
        ChartLoadResult Load(const std::string& chartPath, 
                             double offsetSec = 0.15,
                             double gameOffsetSec = 0.0);

    private:
        TempoService tempoService;  ///< テンポ変換サービス

        /// ヒット時間を計算
        void CalculateHitTimes(ChartLoadResult& result, 
                               double offsetSec,
                               double gameOffsetSec);

        /// ソート後のペアインデックスを再構築
        void RebuildPairIndices(ChartLoadResult& result);

        /// ホールド長さを計算
        void CalculateHoldDurations(ChartLoadResult& result);

        /// 最大コンボ数を計算
        void CalculateMaxCombo(ChartLoadResult& result);
    };

} // namespace app::test
