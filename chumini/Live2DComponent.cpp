#include "Live2DComponent.h"
#include "Live2DManager.h"
#include "DirectX11.h"
#include "Actor.h"

using namespace app::test;
using namespace sf;

void Live2DComponent::Begin() {
    // Live2DManagerの初期化（既にされている場合は内部で弾かれる想定だが念のため）
    Live2DManager::GetInstance()->Initialize();
}

void Live2DComponent::LoadModel(const std::string& dir, const std::string& fileName) {
    // 確実に初期化しておく
    Live2DManager::GetInstance()->Initialize();

    if (_model) {
        delete _model;
        _model = nullptr;
    }

    _model = new AppModel();
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    // ★重要: レンダラーの静的初期化をモデルロードの前に行う (s_deviceの設定)
    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device);

    _model->LoadAssets(device, dir, fileName);

    auto renderer = _model->GetMyRenderer();
    if (renderer) {
        renderer->Initialize(_model->GetModel());
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
    if (!_model) return;

    auto* dx11 = sf::dx::DirectX11::Instance();
    if (!dx11) return;

    auto device = dx11->GetMainDevice().GetDevice();
    auto context = dx11->GetMainDevice().GetContext();

    // ★デバイス/コンテキストの有効性チェック（Intel GPUクラッシュ防止）
    if (!device || !context) {
        OutputDebugStringA("Live2DComponent::Draw - Device or Context is null, skipping draw.\n");
        return;
    }

    // Live2Dの描画

    // =========================================================
    // 描画実行
    // =========================================================
    UINT numViewports = 1;
    D3D11_VIEWPORT vp = {};
    context->RSGetViewports(&numViewports, &vp);

    // ★ビューポートの有効性チェック（ゼロ除算防止）
    if (numViewports == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f) {
        OutputDebugStringA("Live2DComponent::Draw - Invalid viewport, skipping draw.\n");
        return;
    }

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

    auto renderer = _model->GetMyRenderer();
    if (renderer) {
        renderer->SetDefaultRenderState(); // Live2D側のデフォルト設定も適用

        // =========================================================
        // レンダリングステートの退避と設定 (SetDefaultRenderState後に強制適用)
        // =========================================================

        // トポロジーをStripからListへ変更
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ★キャッシュされたステートを使用（毎フレーム生成を廃止）
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

        // シェーダーの無効化（干渉防止）
        context->GSSetShader(nullptr, nullptr, 0);
        context->HSSetShader(nullptr, nullptr, 0);
        context->DSSetShader(nullptr, nullptr, 0);

        // 行列計算
        Live2D::Cubism::Framework::CubismMatrix44 matrix;
        matrix.LoadIdentity();

        float modelCanvasH = _model->GetModel()->GetCanvasHeight();
        if (modelCanvasH == 0) modelCanvasH = 2000.0f;

        float aspect = vp.Width / vp.Height;
        
        // 画面に収まるようにスケール調整 (Base Fit)
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

        _model->Draw(device, context, matrix);
    }

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);

    // =========================================================
    // 【重要】レンダリングステートのリセット (後片付け)
    // =========================================================

    // ★Resource Unbind (Driver Clean-up)
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRVs);
    
    // 1. シザー矩形の無効化 (これが残っているとUIなどが消滅する)
    context->RSSetScissorRects(0, nullptr);

    // 2. シェーダーの解除 (次の描画への干渉防止)
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    // 3. インプットレイアウトの解除
    context->IASetInputLayout(nullptr);

    // 4. トポロジーを基本のTriangleListに戻す
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Live2DComponent::~Live2DComponent() {
    ReleaseCachedStates();
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}

// ★ステートの初期化（一度だけ実行）
void Live2DComponent::InitCachedStates(ID3D11Device* device) {
    if (m_statesInitialized || !device) return;

    // ブレンドステート設定
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

    // ラスタライザステート設定
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    device->CreateRasterizerState(&rasterDesc, &m_cachedRasterState);

    // 深度ステンシルステート設定
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

