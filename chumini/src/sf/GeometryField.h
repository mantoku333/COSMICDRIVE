#pragma once
#include "Geometry.h"
namespace sf
{
	namespace geometry
	{
		class GeometryField :public Geometry
		{
		public:
			void Load();

			void Draw(const Material& mtl)override;
		};
	}
}