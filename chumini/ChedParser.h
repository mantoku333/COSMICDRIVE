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
    };

    struct ChedTempoEvent {
        double absBeat = 0.0;
        double bpm = 120.0;
        double secAtStart = 0.0;
    };

    class ChedParser : public sf::Component {
    public:
        void Begin() override {} // 今は空
        // void Update(const sf::command::ICommand&) override {}
        bool Load(const std::string& path);

        int ticksPerBeat = 480;

        std::unordered_map<int, double> bpmTable;
        std::vector<ChedTempoEvent> tempos;
        std::vector<ChedNote> notes;
	};

}
