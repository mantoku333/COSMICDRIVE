// PlayerComponent.h
#pragma once
#include "Component.h"
#include <Windows.h>  

namespace app {
    namespace test {

        class PlayerComponent : public sf::Component {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand& command);

        private:
            sf::command::Command<> updateCommand;

            POINT prevMouse = { 0, 0 };
            bool mouseInitialized = false;
            float leverX = 0.0f; // -1(ç∂) Å` +1(âE)
        };

    } // namespace test
} // namespace app