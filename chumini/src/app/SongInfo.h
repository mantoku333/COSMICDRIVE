#pragma once
#include <string>

namespace app::test {

    struct SongInfo {
        std::string title;       // 曲名 (#TITLE)
        std::string artist;      // アーティスト名 (#ARTIST)
        std::string jacketPath;  // ジャケット画像のパス (#JACKET)
        std::string chartPath;   // 譜面ファイル(.ched/.sus)のパス
        std::string musicPath;   // 音声ファイルのパス (#WAVE)
        std::string bpm;         // BPM (文字列で保持、初期表示用)
        float previewStartTime = 0.0f; // プレビュー再生開始時間(秒)
        int level = 0;           // レベル (#PLAYLEVEL)
        int difficulty = 0;      // 難易度ID (#DIFFICULTY)
    };

} 
