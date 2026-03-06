#pragma once
#include "ScoreManager.h"

namespace app::test {

    class RatingManager {
    public:
        static RatingManager& Instance();

        // プレイヤーレートを取得（上位10曲の平均）
        float GetPlayerRating() const;

        // 上位チャートの詳細を取得（title, rating, chartPath のタプル）
        std::vector<std::tuple<std::string, float, std::string>> GetTopCharts(int count = 10) const;

    private:
        RatingManager() = default;
        ~RatingManager() = default;

        // コピー禁止
        RatingManager(const RatingManager&) = delete;
        RatingManager& operator=(const RatingManager&) = delete;
    };

}
