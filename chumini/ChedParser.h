#pragma once
#include "Component.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "NoteData.h"

namespace app::test {

    struct ChedNote {
        NoteType type;
        int lane = 0;
        int measure = 0;
        int tick = 0;
        int ticksPerBeat = 480;
        double beat = 0.0;
        double absBeat = 0.0;

        double time = 0.0;

        // Hold Note Support
        bool isHold = false;
        int holdId = -1; 
    };

    struct ChedTempoEvent {
        double absBeat = 0.0;
        double bpm = 120.0;
        double secAtStart = 0.0;
    };

    class ChedParser : public sf::Component {
    public:
        void Begin() override {} 
        // void Update(const sf::command::ICommand&) override {}
        
        bool Load(const std::wstring& path, bool headerOnly = false);

        // 読み取ったヘッダ情報を外部から参照できるようにする
        std::string title;
        std::string artist;
        std::string jacketFile;
        std::string waveFile;
        int bpm = 0;

        int level = 0;
        int difficulty = 0;

        int ticksPerBeat = 480;

        std::unordered_map<int, double> bpmTable;
        std::vector<ChedTempoEvent> tempos;
        std::vector<ChedNote> notes;
	};

}
