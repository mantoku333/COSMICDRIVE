#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class ResultCanvas :public sf::ui::Canvas
		{
		public:
			void Begin()override;
			void Update(const sf::command::ICommand&);

		private:
			sf::command::Command<> updateCommand;
		};
	}
}