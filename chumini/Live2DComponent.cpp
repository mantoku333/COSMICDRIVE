#include "Live2DComponent.h"
#include "Live2DManager.h"
#include "DirectX11.h"

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

void Live2DComponent::Draw() {
    if (!_model) return;

    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();
    auto context = dx11->GetMainDevice().GetContext();

    // Live2Dの描画


    // =========================================================
    // 描画実行
    // =========================================================
    UINT numViewports = 1;
    D3D11_VIEWPORT vp;
    context->RSGetViewports(&numViewports, &vp);

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

    auto renderer = _model->GetMyRenderer();
    if (renderer) {
        renderer->SetDefaultRenderState(); // Live2D側のデフォルト設定も適用

        // =========================================================
        // レンダリングステートの退避と設定 (SetDefaultRenderState後に強制適用)
        // =========================================================

        // トポロジーをStripからListへ変更
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

        ID3D11BlendState* blendState = nullptr;
        if (SUCCEEDED(device->CreateBlendState(&blendDesc, &blendState))) {
            float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            context->OMSetBlendState(blendState, blendFactor, 0xffffffff);
            blendState->Release();
        }

        // ラスタライザステート設定
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE; // 両面描画
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = FALSE; // シザー無効化

        ID3D11RasterizerState* rasterState = nullptr;
        if (SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &rasterState))) {
            context->RSSetState(rasterState);
            rasterState->Release();
        }

        // 深度ステンシルステート設定（深度テスト無効、上書き）
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthDesc.StencilEnable = FALSE;

        ID3D11DepthStencilState* depthState = nullptr;
        if (SUCCEEDED(device->CreateDepthStencilState(&depthDesc, &depthState))) {
            context->OMSetDepthStencilState(depthState, 0);
            depthState->Release();
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
        
        // 画面に収まるようにスケール調整
        float scale = 2.0f / modelCanvasH; 
        matrix.Scale(scale / aspect, scale);
        // 位置調整（必要に応じて）
        // matrix.Translate(0.0f, -0.5f);

        _model->Draw(device, context, matrix);
    }

    Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);
}

Live2DComponent::~Live2DComponent() {
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}
