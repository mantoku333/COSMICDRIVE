#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class ChangeMotionComponent :public sf::Component
		{
		public:
			void Begin()override;

		private:
			void Update();

		private:
			sf::command::Command<> updateCommand;

			//Motionコンポーネント保存用変数
			sf::SafePtr<sf::motion::Motion> motion;
		};
	}
}