#include "ScoreManager.h"
#include "Debug.h" // sf::debug::Debug::Log
#include <fstream>
#include <filesystem>

namespace app::test {

    ScoreManager& ScoreManager::Instance() {
        static ScoreManager instance;
        return instance;
    }

    void ScoreManager::Load() {
        scores.clear();

        if (!std::filesystem::exists(SAVE_FILE_PATH)) {
            sf::debug::Debug::Log("Score file not found, creating new one on save.");
            return;
        }

        std::ifstream ifs(SAVE_FILE_PATH, std::ios::binary);
        if (!ifs) {
            sf::debug::Debug::Log("Failed to open score file for reading.");
            return;
        }

        int entryCount = 0;
        ifs.read(reinterpret_cast<char*>(&entryCount), sizeof(int));

        for (int i = 0; i < entryCount; ++i) {
            // Path
            int pathLen = 0;
            ifs.read(reinterpret_cast<char*>(&pathLen), sizeof(int));
            if (pathLen < 0 || pathLen > 10000) break; // Safety check

            std::string path(pathLen, '\0');
            ifs.read(&path[0], pathLen);

            // Score
            int score = 0;
            ifs.read(reinterpret_cast<char*>(&score), sizeof(int));

            // Rank
            int rankLen = 0;
            ifs.read(reinterpret_cast<char*>(&rankLen), sizeof(int));
            if (rankLen < 0 || rankLen > 100) break; // Safety check

            std::string rank(rankLen, '\0');
            ifs.read(&rank[0], rankLen);

            // Difficulty (新規追加、後方互換性のためチェック)
            int difficulty = 0;
            if (ifs.good() && !ifs.eof()) {
                // 読み込める状態かチェックしてから読む
                std::streampos before = ifs.tellg();
                ifs.read(reinterpret_cast<char*>(&difficulty), sizeof(int));
                if (ifs.fail()) {
                    // 読み込み失敗したら0にリセット
                    ifs.clear();
                    ifs.seekg(before);
                    difficulty = 0;
                }
            }

            // Rating (新規追加、後方互換性のためチェック)
            float rating = 0.0f;
            if (ifs.good() && !ifs.eof()) {
                std::streampos before = ifs.tellg();
                ifs.read(reinterpret_cast<char*>(&rating), sizeof(float));
                if (ifs.fail()) {
                    // 読み込み失敗したら0にリセット
                    ifs.clear();
                    ifs.seekg(before);
                    rating = 0.0f;
                }
            }

            // Title (新規追加、後方互換性のためチェック)
            std::string title = "";
            if (ifs.good() && !ifs.eof()) {
                std::streampos before = ifs.tellg();
                int titleLen = 0;
                ifs.read(reinterpret_cast<char*>(&titleLen), sizeof(int));
                if (!ifs.fail() && titleLen > 0 && titleLen < 1000) {
                    title.resize(titleLen);
                    ifs.read(&title[0], titleLen);
                    if (ifs.fail()) {
                        title = "";
                    }
                } else {
                    if (ifs.fail()) ifs.clear();
                    ifs.seekg(before);
                }
            }

            // Store
            ScoreRecord record;
            record.highScore = score;
            record.rank = rank;
            record.difficulty = difficulty;
            record.rating = rating;
            record.title = title;
            scores[path] = record;
        }

        sf::debug::Debug::Log("Score loaded. Entries: " + std::to_string(scores.size()));
    }

    void ScoreManager::Save() {
        // Ensure directory exists
        std::filesystem::path p(SAVE_FILE_PATH);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream ofs(SAVE_FILE_PATH, std::ios::binary);
        if (!ofs) {
            sf::debug::Debug::Log("Failed to open score file for writing.");
            return;
        }

        int entryCount = static_cast<int>(scores.size());
        ofs.write(reinterpret_cast<const char*>(&entryCount), sizeof(int));

        for (const auto& pair : scores) {
            const std::string& path = pair.first;
            const ScoreRecord& record = pair.second;

            // Path
            int pathLen = static_cast<int>(path.size());
            ofs.write(reinterpret_cast<const char*>(&pathLen), sizeof(int));
            ofs.write(path.c_str(), pathLen);

            // Score
            ofs.write(reinterpret_cast<const char*>(&record.highScore), sizeof(int));

            // Rank
            int rankLen = static_cast<int>(record.rank.size());
            ofs.write(reinterpret_cast<const char*>(&rankLen), sizeof(int));
            ofs.write(record.rank.c_str(), rankLen);

            // Difficulty
            ofs.write(reinterpret_cast<const char*>(&record.difficulty), sizeof(int));

            // Rating
            ofs.write(reinterpret_cast<const char*>(&record.rating), sizeof(float));

            // Title
            int titleLen = static_cast<int>(record.title.size());
            ofs.write(reinterpret_cast<const char*>(&titleLen), sizeof(int));
            ofs.write(record.title.c_str(), titleLen);
        }

        sf::debug::Debug::Log("Score saved. Entries: " + std::to_string(entryCount));
    }

    // レート計算ヘルパー関数
    static float CalculateRating(int difficulty, int score) {
        float base = static_cast<float>(difficulty);
        
        // スコアに応じたボーナスを計算
        if (score >= 1000000) {
            return base + 1.0f;  // SSS
        } else if (score >= 900000) {
            // SS: +0.8 ~ +1.0 (線形補間)
            float ratio = (score - 900000) / 100000.0f;
            return base + 0.8f + 0.2f * ratio;
        } else if (score >= 800000) {
            // S: +0.6 ~ +0.8
            float ratio = (score - 800000) / 100000.0f;
            return base + 0.6f + 0.2f * ratio;
        } else if (score >= 600000) {
            // A: +0.3 ~ +0.6
            float ratio = (score - 600000) / 200000.0f;
            return base + 0.3f + 0.3f * ratio;
        } else if (score >= 400000) {
            // B: +0.0 ~ +0.3
            float ratio = (score - 400000) / 200000.0f;
            return base + 0.0f + 0.3f * ratio;
        } else {
            // C: 0.0
            return base;
        }
    }

    bool ScoreManager::UpdateScore(const std::string& chartPath, int score, const std::string& rank, int difficulty, const std::string& title) {
        bool updated = false;
        float rating = CalculateRating(difficulty, score);
        
        auto it = scores.find(chartPath);
        if (it != scores.end()) {
            if (score > it->second.highScore) {
                it->second.highScore = score;
                it->second.rank = rank;
                it->second.difficulty = difficulty;
                it->second.rating = rating;
                it->second.title = title;
                updated = true;
            }
        } else {
            ScoreRecord newRecord;
            newRecord.highScore = score;
            newRecord.rank = rank;
            newRecord.difficulty = difficulty;
            newRecord.rating = rating;
            newRecord.title = title;
            scores[chartPath] = newRecord;
            updated = true;
        }
        return updated;
    }

    ScoreRecord ScoreManager::GetScore(const std::string& chartPath) const {
        auto it = scores.find(chartPath);
        if (it != scores.end()) {
            return it->second;
        }
        return ScoreRecord{};
    }

    const std::map<std::string, ScoreRecord>& ScoreManager::GetAllScores() const {
        return scores;
    }

}
