// PlayerComponent.h
#pragma once
#include "Component.h"

namespace app {
    namespace test {

        class PlayerComponent : public sf::Component {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand& command);

        private:
            sf::command::Command<> updateCommand;
        };

    } // namespace test
} // namespace app