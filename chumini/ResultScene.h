#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class ResultScene :public sf::Scene<ResultScene>
		{
		public:
			sf::ref::Ref<sf::Actor> uiManagerActor;
			void Init()override;
			void Update(const sf::command::ICommand& command) override;

		private:
			sf::command::Command<> updateCommand;
		};
	}
}