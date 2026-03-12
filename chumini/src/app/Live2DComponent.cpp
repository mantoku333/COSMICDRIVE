#include "Live2DComponent.h"
#include "Live2DManager.h"
#include "DirectX11.h"
#include "Actor.h"
#include "Scene.h" // IScene定義用

using namespace app::test;
using namespace sf;

namespace {
    std::mutex s_live2dGlobalMutex;
    std::mutex s_live2dSdkInitMutex;
}

void Live2DComponent::Begin() {
    // Live2D関連処理
    Live2DManager::GetInstance()->Initialize();
}

void Live2DComponent::LoadModel(const std::string& dir, const std::string& fileName) {
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    OutputDebugStringA("Live2DComponent::LoadModel - Start\n");

    Live2DManager::GetInstance()->Initialize();

    auto* dx11 = sf::dx::DirectX11::Instance();
    if (!dx11) {
        OutputDebugStringA("Live2DComponent::LoadModel - [ERROR] DirectX11 instance is null!\n");
        return;
    }

    auto device = dx11->GetMainDevice().GetDevice();
    if (!device) {
        OutputDebugStringA("Live2DComponent::LoadModel - [ERROR] Device is null!\n");
        return;
    }

    const HRESULT removedReason = device->GetDeviceRemovedReason();
    if (FAILED(removedReason)) {
        OutputDebugStringA("Live2DComponent::LoadModel - [ERROR] Device removed before model load.\n");
        return;
    }

    if (_model) {
        delete _model;
        _model = nullptr;
    }

    _model = new AppModel();

    {
        std::lock_guard<std::mutex> sdkLock(s_live2dSdkInitMutex);
        static ID3D11Device* s_initializedDevice = nullptr;
        if (s_initializedDevice != device) {
            OutputDebugStringA("Live2DComponent::LoadModel - Initializing Static SDK Resources...\n");
            Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
            Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device);
            s_initializedDevice = device;
            OutputDebugStringA("Live2DComponent::LoadModel - Static SDK Resources Initialized.\n");
        }
        else {
            OutputDebugStringA("Live2DComponent::LoadModel - Static SDK Resources already initialized for this device.\n");
        }
    }

    OutputDebugStringA("Live2DComponent::LoadModel - Calling _model->LoadAssets...\n");
    _model->LoadAssets(device, dir, fileName);
    OutputDebugStringA("Live2DComponent::LoadModel - _model->LoadAssets finished.\n");

    auto renderer = _model->GetMyRenderer();
    if (renderer) {
        OutputDebugStringA("Live2DComponent::LoadModel - Renderer Ready.\n");
    }
    else {
        OutputDebugStringA("Live2DComponent::LoadModel - [WARNING] Renderer is null after LoadAssets.\n");
    }
}

void Live2DComponent::Update() {
    if (_model) {
        _model->Update();
    }
}

void Live2DComponent::PlayMotion(const char* group, int no, int priority) {
    if (_model) {
        _model->StartMotion(group, no, priority);
    }
}

void Live2DComponent::StartGlitchMotion(const char* group, int no) {
    if (_model) {
        _model->StartGlitchMotion(group, no);
    }
}

void Live2DComponent::SetDragging(float x, float y) {
    if (_model) {
        _model->SetDragging(x, y);
    }
}

void Live2DComponent::Draw() {
    // Live2D関連処理
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    // 条件分岐
    if (m_isDestroyed) {
        OutputDebugStringA("Live2DComponent::Draw - SKIPPED (destroyed)\n");
        return;
    }

    // 処理本体
    auto owner = actorRef.Target();
    if (!owner || !owner->GetScene().IsActivate()) {
        OutputDebugStringA("Live2DComponent::Draw - SKIPPED (scene deactivated)\n");
        return;
    }

    if (!_model) return;

    auto* dx11 = sf::dx::DirectX11::Instance();
    if (!dx11) return;

    auto device = dx11->GetMainDevice().GetDevice();
    auto context = dx11->GetMainDevice().GetContext();

    // 条件分岐
    if (!device || !context) {
        OutputDebugStringA("Live2DComponent::Draw - Device or Context is null, skipping draw.\n");
        return;
    }

    const HRESULT removedReason = device->GetDeviceRemovedReason();
    if (FAILED(removedReason)) {
        OutputDebugStringA("Live2DComponent::Draw - Device removed, skipping draw.\n");
        return;
    }

    // Live2Dの描画

    // =========================================================
    // 処理本体
    // =========================================================
    UINT numViewports = 1;
    D3D11_VIEWPORT vp = {};
    context->RSGetViewports(&numViewports, &vp);

    // 条件分岐
    if (numViewports == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f) {
        OutputDebugStringA("Live2DComponent::Draw - Invalid viewport, skipping draw.\n");
        return;
    }

    try {
        OutputDebugStringA("Live2DComponent::Draw - Before StartFrame\n");
        Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);
        OutputDebugStringA("Live2DComponent::Draw - After StartFrame\n");

        auto renderer = _model->GetMyRenderer();
        if (renderer) {
            renderer->SetDefaultRenderState();

            // =========================================================
            // デバッグログを出力
            // =========================================================

            // デバッグログを出力
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 処理本体
            InitCachedStates(device);
            
            if (m_cachedBlendState) {
                float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                context->OMSetBlendState(m_cachedBlendState, blendFactor, 0xffffffff);
            }

            if (m_cachedRasterState) {
                context->RSSetState(m_cachedRasterState);
            }

            if (m_cachedDepthState) {
                context->OMSetDepthStencilState(m_cachedDepthState, 0);
            }

            // 処理本体
            context->GSSetShader(nullptr, nullptr, 0);
            context->HSSetShader(nullptr, nullptr, 0);
            context->DSSetShader(nullptr, nullptr, 0);

            // 陦悟・險育ｮ・
            Live2D::Cubism::Framework::CubismMatrix44 matrix;
            matrix.LoadIdentity();

            float modelCanvasH = _model->GetModel()->GetCanvasHeight();
            if (modelCanvasH == 0) modelCanvasH = 2000.0f;

            float aspect = vp.Width / vp.Height;
            
            // 処理本体
            float scale = 2.0f / modelCanvasH; 
            matrix.Scale(scale / aspect, scale);

            // Apply Actor Transform (Position & Scale)
            if (auto owner = actorRef.Target()) {
                Vector3 pos = owner->transform.GetPosition();
                Vector3 scl = owner->transform.GetScale();
                
                // Translate first or after? 
                // In CubismMatrix, operations are multiplied.
                // We want to move the fitted model.
                matrix.Translate(pos.x, pos.y);
                matrix.Scale(scl.x, scl.y);
            }

            OutputDebugStringA("Live2DComponent::Draw - Before Model Draw\n");
            _model->Draw(device, context, matrix);
            OutputDebugStringA("Live2DComponent::Draw - After Model Draw\n");
        }

        Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);
        OutputDebugStringA("Live2DComponent::Draw - After EndFrame\n");

    } catch (const std::exception& e) {
        OutputDebugStringA("Live2DComponent::Draw - Exception caught: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
    } catch (...) {
        OutputDebugStringA("Live2DComponent::Draw - Unknown Exception caught!\n");
    }

    // =========================================================
    // 処理本体
    // =========================================================

    // 笘・esource Unbind (Driver Clean-up)
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRVs);
    
    // 処理本体
    context->RSSetScissorRects(0, nullptr);

    // 2. シェーダーの解除 (次の描画への干渉防止)
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    // 処理本体
    context->IASetInputLayout(nullptr);

    // デバッグログを出力
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 処理本体
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    // Live2D関連処理
    // Live2D関連処理
}

Live2DComponent::~Live2DComponent() {
    // Live2D関連処理
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    // 処理本体
    m_isDestroyed = true;
    OutputDebugStringA("Live2DComponent::~Live2DComponent - DESTRUCTOR CALLED\n");

    // 処理本体
    // 処理本体
    ReleaseCachedStates();
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}

// Live2D関連処理
void Live2DComponent::InitCachedStates(ID3D11Device* device) {
    if (m_statesInitialized || !device) return;

    // 処理本体
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, &m_cachedBlendState);

    // 処理本体
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    device->CreateRasterizerState(&rasterDesc, &m_cachedRasterState);

    // 処理本体
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.StencilEnable = FALSE;
    device->CreateDepthStencilState(&depthDesc, &m_cachedDepthState);

    m_statesInitialized = true;
}

void Live2DComponent::ReleaseCachedStates() {
    if (m_cachedBlendState) { m_cachedBlendState->Release(); m_cachedBlendState = nullptr; }
    if (m_cachedRasterState) { m_cachedRasterState->Release(); m_cachedRasterState = nullptr; }
    if (m_cachedDepthState) { m_cachedDepthState->Release(); m_cachedDepthState = nullptr; }
    m_statesInitialized = false;
}
