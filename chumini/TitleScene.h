#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class TitleScene :public sf::Scene<TitleScene>
		{
		public:

			void Init()override;
			//void Update(const sf::command::ICommand& command) override;

		private:
			//更新コマンド
			sf::command::Command<> updateCommand;


			sf::geometry::GeometryCube g_cube;


			sf::ref::Ref<sf::Actor> uiManagerActor;
			sf::ref::Ref<sf::Actor> editButtonActor;
			sf::ref::Ref<sf::Actor> playButtonActor;
			int selectedButton;  // 0: エディット, 1: プレイ
			bool isButtonPressed;

		};
	}
}