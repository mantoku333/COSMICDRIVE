#include "GeometrySphere.h"

sf::geometry::GeometrySphere::GeometrySphere()
{
	std::vector<dx::Vertex3D> vertices;

	const float radius = 1.0f;

	// í∏ì_Çê∂ê¨
	for (int lat = 0; lat <= latitudeSegments; ++lat) {
		float theta = lat * DirectX::XM_PI / latitudeSegments;
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);

		for (int lon = 0; lon <= longitudeSegments; ++lon) {
			float phi = lon * 2.0f * DirectX::XM_PI / longitudeSegments;
			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			dx::Vertex3D vertex;
			vertex.pos = DirectX::XMFLOAT3(radius * cosPhi * sinTheta, radius * cosTheta, radius * sinPhi * sinTheta);
			vertex.nor = DirectX::XMFLOAT3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
			vertex.uv = DirectX::XMFLOAT2((float)lon / longitudeSegments, (float)lat / latitudeSegments);

			vertices.push_back(vertex);
		}
	}

	vertexBuffer.Create(vertices);

	std::vector<WORD> indices;

	for (int lat = 0; lat < latitudeSegments; ++lat) {
		for (int lon = 0; lon < longitudeSegments; ++lon) {
			int first = (lat * (longitudeSegments + 1)) + lon;
			int second = first + longitudeSegments + 1;

			indices.push_back(first);
			indices.push_back(first + 1);
			indices.push_back(second);

			indices.push_back(second);
			indices.push_back(first + 1);
			indices.push_back(second + 1);
		}
	}

	indexBuffer.Create(indices);
}

void sf::geometry::GeometrySphere::Draw(const Material& mtl)
{
	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	mtl.SetGPU(dx11->GetMainDevice());
	vertexBuffer.SetGPU(dx11->GetMainDevice());
	indexBuffer.SetGPU(dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dx11->vs3d.SetGPU(dx11->GetMainDevice());
	dx11->gsNone.SetGPU(dx11->GetMainDevice());
	dx11->ps3d.SetGPU(dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->DrawIndexed(indexBuffer.GetIndexSize(), 0, 0);
}
