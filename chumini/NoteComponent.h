#pragma once
#include "Component.h"
#include "NoteData.h"

namespace app::test { class NoteManager; }

namespace app::test {

    class NoteComponent : public sf::Component {
    public:
        NoteComponent();

        void Begin() override;
        void Activate() override;
        void DeActivate() override;
        void Update(const sf::command::ICommand& command);

        NoteData info;
        float leadTime = 0.f;
        float noteSpeed = 10.f;
        float spawnTime = 0.f;
        float elapsed = 0.f;

    private:
        NoteManager* noteManager = nullptr;

        sf::command::Command<> updateCommand;

        int lanes = 0;
        float laneW = 0.f;
        float laneH = 0.f;
        float rotX = 0.f;
        float baseY = 0.f;
        float barRatio = 0.f;

        float slopeRad = 0.f;
        float startZ = 0.f;
        float endZ = 0.f;
        float startY = 0.f;
        float endY = 0.f;

        bool isActive = false;
        bool skipFirstActivate = true;
    };
}
