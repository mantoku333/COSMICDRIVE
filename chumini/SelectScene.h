#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class SelectScene :public sf::Scene<SelectScene>
        {
        public:
            sf::ref::Ref<sf::Actor> uiManagerActor;
            void Init() override;
            void Update(const sf::command::ICommand& command);

        private:
            sf::command::Command<> updateCommand;

        };
	}
}