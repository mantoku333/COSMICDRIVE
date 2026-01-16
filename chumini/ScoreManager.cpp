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

            // Store
            ScoreRecord record;
            record.highScore = score;
            record.rank = rank;
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
        }

        sf::debug::Debug::Log("Score saved. Entries: " + std::to_string(entryCount));
    }

    bool ScoreManager::UpdateScore(const std::string& chartPath, int score, const std::string& rank) {
        bool updated = false;
        auto it = scores.find(chartPath);
        if (it != scores.end()) {
            if (score > it->second.highScore) {
                it->second.highScore = score;
                it->second.rank = rank;
                updated = true;
            }
        } else {
            ScoreRecord newRecord;
            newRecord.highScore = score;
            newRecord.rank = rank;
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

}
