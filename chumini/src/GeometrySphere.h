#pragma once
#include "Geometry.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
namespace sf
{
	namespace geometry
	{
		//Œ`óƒNƒ‰ƒX-‹…
		class GeometrySphere :public Geometry
		{
		public:
			GeometrySphere();

			void Draw(const Material& mtl)override;

		private:
			dx::VertexBuffer<dx::Vertex3D> vertexBuffer;
			dx::IndexBuffer indexBuffer;

			const static int latitudeSegments = 16;
			const static int longitudeSegments = 16;
		};
	}
}