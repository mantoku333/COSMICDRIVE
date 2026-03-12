#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class EditorComponent :public sf::Component
		{
		public:
			void Begin()override;

		private:
			void Update();

		private:
			//更新コマンド
			sf::command::Command<> updateCommand;
			sf::geometry::GeometryCube g_cube;
		};
	}
}