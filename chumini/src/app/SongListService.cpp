#include "SongListService.h"
#include "ChedParser.h"
#include "Debug.h"
#include "StringUtils.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace app::test {

    // sf::util の関数を使用
    using sf::util::WstringToUtf8;
    using sf::util::Utf8ToWstring;

    void SongListService::ScanSongs(const std::string& rootPath)
    {
        allGenres.clear();
        namespace fs = std::filesystem;

        if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) {
            sf::debug::Debug::Log("Songs directory not found: " + rootPath);
            // ダミージャンルを追加
            Genre dummy;
            dummy.name = "No Genre";
            dummy.songs.push_back({ "No Music Found", "System", "", "", "", "0" });
            allGenres.push_back(dummy);
            currentGenreIndex = 0;
            return;
        }

        // 1. rootPath直下のフォルダを「ジャンル」として扱う
        for (const auto& genreEntry : fs::directory_iterator(rootPath)) {
            if (!genreEntry.is_directory()) continue;

            Genre newGenre;
            // フォルダ名をジャンル名とする
            newGenre.name = genreEntry.path().filename().string();

            // 2. そのジャンルフォルダ内を再帰的に走査して曲を探す
            for (const auto& fileEntry : fs::recursive_directory_iterator(genreEntry.path())) {
                if (!fileEntry.is_regular_file()) continue;

                std::wstring ext = fileEntry.path().extension().wstring();
                if (ext == L".sus" || ext == L".ched") {
                    ChedParser parser;
                    if (parser.Load(fileEntry.path().wstring(), true)) {
                        SongInfo info;
                        info.title = parser.title.empty() ? "No Title" : parser.title;
                        info.artist = parser.artist.empty() ? "No Artist" : parser.artist;
                        info.chartPath = WstringToUtf8(fileEntry.path().wstring());

                        fs::path parentDir = fileEntry.path().parent_path();

                        // ジャケット
                        if (!parser.jacketFile.empty()) {
                            std::wstring jacketW = Utf8ToWstring(parser.jacketFile);
                            fs::path fullJacketPath = parentDir / jacketW;
                            info.jacketPath = WstringToUtf8(fullJacketPath.wstring());
                        }
                        else {
                            info.jacketPath = "";
                        }

                        // 音声
                        if (!parser.waveFile.empty()) {
                            std::wstring waveW = Utf8ToWstring(parser.waveFile);
                            fs::path fullWavePath = parentDir / waveW;
                            info.musicPath = WstringToUtf8(fullWavePath.wstring());
                        }

                        // プレビュー開始位置 (.pv check)
                        fs::path pvPath = fileEntry.path();
                        pvPath.replace_extension(".pv");
                        if (fs::exists(pvPath)) {
                            std::ifstream ifs(pvPath);
                            if (ifs) {
                                float startTime = 0.0f;
                                ifs >> startTime;
                                info.previewStartTime = startTime;
                            }
                        }

                        info.bpm = std::to_string(parser.bpm);
                        info.level = parser.level;
                        info.difficulty = parser.difficulty;
                        newGenre.songs.push_back(info);
                    }
                }
            }

            // 曲が1つでもあればジャンルとして登録
            if (!newGenre.songs.empty()) {
                allGenres.push_back(newGenre);
                sf::debug::Debug::Log("Genre Loaded: " + newGenre.name + " (" + std::to_string(newGenre.songs.size()) + " songs)");
            }
        }

        // 3. データがなければダミー
        if (allGenres.empty()) {
            Genre dummy;
            dummy.name = "No Genre";
            dummy.songs.push_back({ "No Music Found", "System", "", "", "", "0" });
            allGenres.push_back(dummy);
        }

        currentGenreIndex = 0;
    }

    const Genre& SongListService::GetCurrentGenre() const
    {
        static Genre empty{ "Empty", {} };
        if (allGenres.empty()) return empty;
        int idx = std::clamp(currentGenreIndex, 0, (int)allGenres.size() - 1);
        return allGenres[idx];
    }

    const std::vector<SongInfo>& SongListService::ChangeGenre(int index)
    {
        if (allGenres.empty()) {
            static std::vector<SongInfo> empty;
            return empty;
        }
        // 循環インデックス
        int n = (int)allGenres.size();
        currentGenreIndex = ((index % n) + n) % n;
        return allGenres[currentGenreIndex].songs;
    }

    const std::vector<SongInfo>& SongListService::NextGenre()
    {
        return ChangeGenre(currentGenreIndex + 1);
    }

    const std::vector<SongInfo>& SongListService::PreviousGenre()
    {
        return ChangeGenre(currentGenreIndex - 1);
    }

    const std::vector<SongInfo>& SongListService::GetCurrentSongs() const
    {
        return GetCurrentGenre().songs;
    }

}
