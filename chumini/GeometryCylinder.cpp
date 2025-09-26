#include "GeometryCylinder.h"

sf::geometry::GeometryCylinder::GeometryCylinder()
{
    std::vector<dx::Vertex3D> vertices;

    float radius = 1.0f;
    float halfHeight = 0.5f;
    float angleStep = DirectX::XM_2PI / segments;

    // 上面の中心点
    dx::Vertex3D topCenter;
    topCenter.pos = DirectX::XMFLOAT3(0.0f, halfHeight, 0.0f);
    topCenter.nor = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
    topCenter.uv = DirectX::XMFLOAT2(0.5f, 0.5f);
    vertices.push_back(topCenter);

    // 上面の頂点
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        dx::Vertex3D topVertex;
        topVertex.pos = DirectX::XMFLOAT3(x, halfHeight, z);
        topVertex.nor = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
        topVertex.uv = DirectX::XMFLOAT2(0.5f + 0.5f * cosf(angle), 0.5f - 0.5f * sinf(angle));
        vertices.push_back(topVertex);
    }

    // 底面の中心点
    dx::Vertex3D bottomCenter;
    bottomCenter.pos = DirectX::XMFLOAT3(0.0f, -halfHeight, 0.0f);
    bottomCenter.nor = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
    bottomCenter.uv = DirectX::XMFLOAT2(0.5f, 0.5f);
    vertices.push_back(bottomCenter);

    // 底面の頂点
    int bottomCenterIndex = static_cast<int>(vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        dx::Vertex3D bottomVertex;
        bottomVertex.pos = DirectX::XMFLOAT3(x, -halfHeight, z);
        bottomVertex.nor = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
        bottomVertex.uv = DirectX::XMFLOAT2(0.5f + 0.5f * cosf(angle), 0.5f - 0.5f * sinf(angle));
        vertices.push_back(bottomVertex);
    }

    // 側面の頂点
    int sideStartIndex = static_cast<int>(vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        float u = (float)i / segments;

        dx::Vertex3D topSideVertex, bottomSideVertex;
        topSideVertex.pos = DirectX::XMFLOAT3(x, halfHeight, z);
        topSideVertex.nor = DirectX::XMFLOAT3(x, 0.0f, z);
        topSideVertex.uv = DirectX::XMFLOAT2(u, 0.0f);

        bottomSideVertex.pos = DirectX::XMFLOAT3(x, -halfHeight, z);
        bottomSideVertex.nor = DirectX::XMFLOAT3(x, 0.0f, z);
        bottomSideVertex.uv = DirectX::XMFLOAT2(u, 1.0f);

        vertices.push_back(topSideVertex);
        vertices.push_back(bottomSideVertex);
    }

    vertexBuffer.Create(vertices);

    std::vector<WORD> indices;

    // 上面のインデックス
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i);
    }

    // 底面のインデックス
    int bottomStartIndex = bottomCenterIndex;
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(bottomStartIndex);
        indices.push_back(bottomStartIndex + i);
        indices.push_back(bottomStartIndex + i + 1);
    }

    // 側面のインデックス
    for (int i = 0; i < segments; ++i) {
        int topIndex = sideStartIndex + i * 2;
        int bottomIndex = topIndex + 1;

        indices.push_back(topIndex);
        indices.push_back(topIndex + 2);
        indices.push_back(bottomIndex);

        indices.push_back(bottomIndex);
        indices.push_back(topIndex + 2);
        indices.push_back(bottomIndex + 2);
    }

    indexBuffer.Create(indices);
}

void sf::geometry::GeometryCylinder::Draw(const Material& mtl)
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
