#pragma once
#include <string>

namespace app::test {

    struct SongInfo {
		std::string title;       //曲名
        std::string artist;      //アーティスト名
		std::string jacketPath;  //ジャケ絵パス
        std::string chartPath;   //譜面パス 後でかえる
        std::string musicPath;	 //曲パス
		std::string bpm;         //BPM
    };

} 
