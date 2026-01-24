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

}
