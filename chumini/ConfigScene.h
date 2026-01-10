#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class ConfigScene : public sf::Scene<ConfigScene>
		{
		public:
			void Init() override;
			void Update(const sf::command::ICommand& command);

		private:
			sf::ref::Ref<sf::Actor> uiManagerActor;
			sf::command::Command<> updateCommand;
		};
	}
}
