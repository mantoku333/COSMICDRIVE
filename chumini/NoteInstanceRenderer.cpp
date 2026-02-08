#include "NoteInstanceRenderer.h"
#include "DirectX11.h"
#include "Debug.h"
#include <cstdio>
#include <algorithm>

namespace app::test {
    
    // ============================================================
    // Pimpl 実装クラス
    // DirectXリソース管理の詳細を隠蔽
    // ============================================================
    class NoteInstanceRenderer::Impl {
    public:
        Impl() = default;
        ~Impl() { Release(); }

        void Init();
        void UpdateBuffer(const std::vector<NoteInstanceData>& instances);
        void Draw(size_t instanceCount);
        void Release();

    private:
        static constexpr size_t MAX_INSTANCES = 2048;

        ID3D11Buffer* m_instanceBuffer = nullptr;
        ID3D11VertexShader* m_vs = nullptr;
        ID3D11InputLayout* m_layout = nullptr;
        ID3D11Buffer* m_cubeVB = nullptr;
        ID3D11Buffer* m_cubeIB = nullptr;
        int m_cubeIndexCount = 0;
    };

    // ============================================================
    // 外向インターフェース実装 (Pimplへの委譲)
    // ============================================================
    
    NoteInstanceRenderer::NoteInstanceRenderer() : m_impl(std::make_unique<Impl>()) {}
    NoteInstanceRenderer::~NoteInstanceRenderer() = default;

    NoteInstanceRenderer::NoteInstanceRenderer(NoteInstanceRenderer&&) noexcept = default;
    NoteInstanceRenderer& NoteInstanceRenderer::operator=(NoteInstanceRenderer&&) noexcept = default;

    void NoteInstanceRenderer::Init() { m_impl->Init(); }
    void NoteInstanceRenderer::UpdateBuffer(const std::vector<NoteInstanceData>& instances) { m_impl->UpdateBuffer(instances); }
    void NoteInstanceRenderer::Draw(size_t instanceCount) { m_impl->Draw(instanceCount); }
    void NoteInstanceRenderer::UpdateAndDraw(const std::vector<NoteInstanceData>& instances) {
        if(instances.empty()) return;
        m_impl->UpdateBuffer(instances);
        m_impl->Draw(instances.size());
    }
    void NoteInstanceRenderer::Release() { m_impl->Release(); }

    // ============================================================
    // Impl 実装（DirectX詳細）
    // ============================================================

    void NoteInstanceRenderer::Impl::Init()
    {
        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();

        // インスタンスバッファ（動的書き込み用）
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(NoteInstanceData) * MAX_INSTANCES;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateBuffer(&desc, nullptr, &m_instanceBuffer);
        if (FAILED(hr)) {
            sf::debug::Debug::LogError("Failed to create Instance Buffer");
        }

        // キューブメッシュの頂点データ（6面×4頂点 = 24頂点）
        struct SimpleVertex {
            float pos[3];
            float nor[3];
            float uv[2];
            float col[4];
        };

        SimpleVertex vertices[] = {
            // 前面
            { {-0.5f, -0.5f, -0.5f}, {0,0,-1}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {0,0,-1}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,0,-1}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,0,-1}, {1,1}, {1,1,1,1} },
            // 背面
            { { 0.5f, -0.5f,  0.5f}, {0,0,1}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,0,1}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,0,1}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f,  0.5f}, {0,0,1}, {1,1}, {1,1,1,1} },
            // 上面
            { {-0.5f,  0.5f, -0.5f}, {0,1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,1,0}, {1,1}, {1,1,1,1} },
            // 下面
            { {-0.5f, -0.5f,  0.5f}, {0,-1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {0,-1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,-1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {0,-1,0}, {1,1}, {1,1,1,1} },
            // 左面
            { {-0.5f, -0.5f,  0.5f}, {-1,0,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {-1,0,0}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {-1,0,0}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {-1,0,0}, {1,1}, {1,1,1,1} },
            // 右面
            { { 0.5f, -0.5f, -0.5f}, {1,0,0}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {1,0,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {1,0,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {1,0,0}, {1,1}, {1,1,1,1} },
        };

        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = sizeof(SimpleVertex) * 24;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vInitData{};
        vInitData.pSysMem = vertices;
        hr = device->CreateBuffer(&vbd, &vInitData, &m_cubeVB);

        // インデックス
        unsigned int indices[] = {
            0,1,2, 0,2,3,
            4,5,6, 4,6,7,
            8,9,10, 8,10,11,
            12,13,14, 12,14,15,
            16,17,18, 16,18,19,
            20,21,22, 20,22,23
        };
        m_cubeIndexCount = 36;

        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = sizeof(unsigned int) * 36;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iInitData{};
        iInitData.pSysMem = indices;
        hr = device->CreateBuffer(&ibd, &iInitData, &m_cubeIB);

        // プリコンパイル済みシェーダー（CSO）をロード
        FILE* fp = nullptr;
        fopen_s(&fp, "Assets\\Shader\\vsNoteInstancing.cso", "rb");
        if (!fp) {
            sf::debug::Debug::LogError("Failed to open vsNoteInstancing.cso");
            return;
        }

        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char* pShaderData = new char[fileSize];
        fread(pShaderData, fileSize, 1, fp);
        fclose(fp);

        hr = device->CreateVertexShader(pShaderData, fileSize, nullptr, &m_vs);
        if (FAILED(hr)) {
            delete[] pShaderData;
            sf::debug::Debug::LogError("Failed to create vertex shader from CSO");
            return;
        }

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        hr = device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), pShaderData, fileSize, &m_layout);
        delete[] pShaderData;
    }

    void NoteInstanceRenderer::Impl::UpdateBuffer(const std::vector<NoteInstanceData>& instances)
    {
        if (instances.empty()) return;
        if (!m_instanceBuffer) return;

        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetContext();

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            size_t count = std::min(instances.size(), MAX_INSTANCES);
            size_t sizeInBytes = sizeof(NoteInstanceData) * count;
            memcpy(mapped.pData, instances.data(), sizeInBytes);
            context->Unmap(m_instanceBuffer, 0);
        }
    }

    void NoteInstanceRenderer::Impl::Draw(size_t instanceCount)
    {
        try {
            if (instanceCount == 0) return;
            if (!m_vs || !m_layout || !m_cubeVB || !m_cubeIB) return;

            sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
            if (!dx11) return;

            auto context = dx11->GetMainDevice().GetContext();

            if (!context) {
                sf::debug::Debug::LogError("DrawInstanced: Context is null, skipping draw.");
                return;
            }

            context->VSSetShader(m_vs, nullptr, 0);
            context->IASetInputLayout(m_layout);
            dx11->ps3d.SetGPU(dx11->GetMainDevice());
            dx11->gsNone.SetGPU(dx11->GetMainDevice());

            UINT strides[2] = { sizeof(float) * 12, sizeof(NoteInstanceData) };
            UINT offsets[2] = { 0, 0 };
            ID3D11Buffer* buffers[2] = { m_cubeVB, m_instanceBuffer };

            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetIndexBuffer(m_cubeIB, DXGI_FORMAT_R32_UINT, 0);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            material.textureEnable = { 0.0f, 0.0f, 0.0f, 0.0f };
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            context->DrawIndexedInstanced(m_cubeIndexCount, (UINT)instanceCount, 0, 0, 0);
        } catch (...) {
            sf::debug::Debug::LogError("DrawInstanced Exception");
        }
    }

    void NoteInstanceRenderer::Impl::Release()
    {
        if (m_instanceBuffer) { m_instanceBuffer->Release(); m_instanceBuffer = nullptr; }
        if (m_vs)             { m_vs->Release();             m_vs = nullptr; }
        if (m_layout)         { m_layout->Release();         m_layout = nullptr; }
        if (m_cubeVB)         { m_cubeVB->Release();         m_cubeVB = nullptr; }
        if (m_cubeIB)         { m_cubeIB->Release();         m_cubeIB = nullptr; }
    }

} // namespace app::test
