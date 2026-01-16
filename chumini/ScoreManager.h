#pragma once
#include <string>
#include <map>
#include <vector>

namespace app::test {

    struct ScoreRecord {
        int highScore = 0;
        std::string rank = "-";
    };

    class ScoreManager {
    public:
        static ScoreManager& Instance();

        // データの読み込み・保存
        void Load();
        void Save();

        // スコア更新 (ハイスコアなら更新してtrueを返す)
        bool UpdateScore(const std::string& chartPath, int score, const std::string& rank);

        // スコア取得
        ScoreRecord GetScore(const std::string& chartPath) const;

    private:
        ScoreManager() = default;
        ~ScoreManager() = default;

        // コピー禁止
        ScoreManager(const ScoreManager&) = delete;
        ScoreManager& operator=(const ScoreManager&) = delete;

        std::map<std::string, ScoreRecord> scores;
        const std::string SAVE_FILE_PATH = "Save/Scores.bin";
    };

}
