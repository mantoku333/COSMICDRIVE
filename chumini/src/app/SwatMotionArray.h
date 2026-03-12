#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class SwatMotionArray :public sf::motion::MotionArray
		{
		public:
			void LoadMotions()override;

		public:
			static const int ID_Idle = 0;	//アイドル
			static const int ID_Walk = 1;	//歩き
		};
	}
}