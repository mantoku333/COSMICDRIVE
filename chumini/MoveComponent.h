#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class MoveComponent :public sf::Component
		{
		public:
			void Begin()override;

		private:
			//毎フレーム処理
			void Update(const sf::command::ICommand& command);

		private:
			//更新コマンド
			sf::command::Command<> updateCommand;
		};
	}
}