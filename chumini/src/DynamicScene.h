#pragma once
#include "App.h"
#include "GeometrySwat.h"
#include "SwatMotionArray.h"
namespace app
{
	namespace test
	{
		class DynamicScene :public sf::Scene<DynamicScene>
		{
		public:
			void Init()override;

		private:
			sf::geometry::GeometrySphere g_sphere;

			//人型モデル
			GeometrySwat g_human;

			//人型モーションアレイ
			SwatMotionArray ma_human;
		};
	}
}