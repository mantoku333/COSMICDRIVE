#pragma once
#include "Geometry.h"

namespace sf
{
	namespace geometry
	{
		//形状クラス-立方体
		class GeometryCube :public Geometry
		{
		public:
			void Draw(const Material& mtl)override;

		};
	}
}