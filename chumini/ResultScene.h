#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class ResultScene :public sf::Scene<ResultScene>
		{
		public:

			void Init()override;
			//void Update(const sf::command::ICommand& command) override;

		private:
			//更新コマンド
			sf::command::Command<> updateCommand;
		};
	}
}