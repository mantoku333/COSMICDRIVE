#pragma once
#include "Component.h"
//#include <Windows.h>

namespace app {
    namespace test {

        class PlayerComponent : public sf::Component {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand& command);

        private:
            sf::command::Command<> updateCommand;

            long prevMouseX = 0;
            long prevMouseY = 0;
            bool mouseInitialized = false;
            float leverX = 0.0f; // -1(左) ～ +1(右)

            // サイドレーンの前フレーム状態
            bool wasInLeftEdge = false;
            bool wasInRightEdge = false;
        };

    } // namespace test
} // namespace app
