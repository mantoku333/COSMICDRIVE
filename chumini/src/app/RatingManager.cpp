#include "RatingManager.h"
#include "Debug.h"
#include <vector>
#include <algorithm>

namespace app::test {

    RatingManager& RatingManager::Instance() {
        static RatingManager instance;
        return instance;
    }

    float RatingManager::GetPlayerRating() const {
        // ScoreManagerから全スコアを取得
        const auto& allScores = ScoreManager::Instance().GetAllScores();
        
        if (allScores.empty()) {
            return 0.0f;
        }

        // レート値のリストを作成
        std::vector<float> ratings;
        ratings.reserve(allScores.size());
        
        for (const auto& pair : allScores) {
            const ScoreRecord& record = pair.second;
            // スコアが記録されており、レート値が妥当な範囲の場合のみ使用
            // ratingは通常0~50程度の範囲（難易度0~40 + ボーナス0~1程度）
            if (record.highScore > 0 && record.rating >= 0.0f && record.rating < 100.0f) {
                ratings.push_back(record.rating);
            }
        }

        if (ratings.empty()) {
            return 0.0f;
        }

        // レート値を降順にソート
        std::sort(ratings.begin(), ratings.end(), std::greater<float>());

        // 上位10曲（または全曲数が10未満ならその分）を取得
        int topCount = std::min(10, static_cast<int>(ratings.size()));
        
        float sum = 0.0f;
        for (int i = 0; i < topCount; ++i) {
            sum += ratings[i];
        }

        float playerRating = sum / static_cast<float>(topCount);
        
        sf::debug::Debug::Log("Player Rating: " + std::to_string(playerRating) + 
                              " (Top " + std::to_string(topCount) + " charts)");
        
        return playerRating;
    }

    std::vector<std::tuple<std::string, float, std::string>> RatingManager::GetTopCharts(int count) const {
        const auto& allScores = ScoreManager::Instance().GetAllScores();
        
        if (allScores.empty()) {
            return {};
        }

        // title, rating, chartPathのタプルのリストを作成
        std::vector<std::tuple<std::string, float, std::string>> chartRatings;
        chartRatings.reserve(allScores.size());
        
        for (const auto& pair : allScores) {
            const ScoreRecord& record = pair.second;
            if (record.highScore > 0 && record.rating >= 0.0f && record.rating < 100.0f) {
                std::string displayTitle = record.title.empty() ? "Unknown" : record.title;
                chartRatings.push_back(std::make_tuple(displayTitle, record.rating, pair.first));
            }
        }

        if (chartRatings.empty()) {
            return {};
        }

        // レート値を降順にソート
        std::sort(chartRatings.begin(), chartRatings.end(), 
                  [](const auto& a, const auto& b) { return std::get<1>(a) > std::get<1>(b); });

        // 上位count曲を返す
        int topCount = std::min(count, static_cast<int>(chartRatings.size()));
        chartRatings.resize(topCount);
        
        return chartRatings;
    }

}
