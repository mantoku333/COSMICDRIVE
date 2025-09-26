#pragma once
#include "Geometry.h"

namespace sf
{
	namespace geometry
	{
		//Œ`óƒNƒ‰ƒX-—§•û‘Ì
		class GeometryCube :public Geometry
		{
		public:
			void Draw(const Material& mtl)override;

		};
	}
}