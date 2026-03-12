#include "Live2DComponent.h"
#include "Live2DManager.h"
#include "DirectX11.h"
#include "Actor.h"
#include "Scene.h"

using namespace app::test;
using namespace sf;

namespace {
    std::mutex s_live2dGlobalMutex;    // Live2D SDK全体のグローバルミューテックス
    std::mutex s_live2dSdkInitMutex;   // SDK静的リソース初期化用ミューテックス
}

/// 初期化処理 - Live2Dマネージャーを初期化する
void Live2DComponent::Begin() {
    Live2DManager::GetInstance()->Initialize();
}

/// モデルファイルを読み込み、レンダラーを準備する
void Live2DComponent::LoadModel(const std::string& dir, const std::string& fileName) {
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);

    Live2DManager::GetInstance()->Initialize();

    // DirectX11デバイスの取得と検証
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

    // デバイスロスト状態のチェック
    const HRESULT removedReason = device->GetDeviceRemovedReason();
    if (FAILED(removedReason)) {
        OutputDebugStringA("Live2DComponent::LoadModel - [ERROR] Device removed before model load.\n");
        return;
    }

    // 既存モデルがあれば解放
    if (_model) {
        delete _model;
        _model = nullptr;
    }

    _model = new AppModel();

    // SDK静的リソースの初期化（デバイスごとに1回のみ）
    {
        std::lock_guard<std::mutex> sdkLock(s_live2dSdkInitMutex);
        static ID3D11Device* s_initializedDevice = nullptr;
        if (s_initializedDevice != device) {
            Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
            Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device);
            s_initializedDevice = device;
        }
    }

    // モデルアセットの読み込み
    _model->LoadAssets(device, dir, fileName);
}

/// モデルの更新処理（モーション・物理演算の進行）
void Live2DComponent::Update() {
    if (_model) {
        _model->Update();
    }
}

/// モーションを再生する
void Live2DComponent::PlayMotion(const char* group, int no, int priority) {
    if (_model) {
        _model->StartMotion(group, no, priority);
    }
}

/// グリッチモーションを再生する
void Live2DComponent::StartGlitchMotion(const char* group, int no) {
    if (_model) {
        _model->StartGlitchMotion(group, no);
    }
}

/// 視線追従の目標座標を設定する
void Live2DComponent::SetDragging(float x, float y) {
    if (_model) {
        _model->SetDragging(x, y);
    }
}

/// Live2Dモデルの描画処理
void Live2DComponent::Draw() {
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    // 破棄済みなら描画をスキップ
    if (m_isDestroyed) return;

    // シーンが非アクティブなら描画をスキップ
    auto owner = actorRef.Target();
    if (!owner || !owner->GetScene().IsActivate()) return;

    if (!_model) return;

    // DirectX11デバイス・コンテキストの取得
    auto* dx11 = sf::dx::DirectX11::Instance();
    if (!dx11) return;

    auto device = dx11->GetMainDevice().GetDevice();
    auto context = dx11->GetMainDevice().GetContext();

    if (!device || !context) return;

    // デバイスロスト状態のチェック
    const HRESULT removedReason = device->GetDeviceRemovedReason();
    if (FAILED(removedReason)) return;

    // ビューポートの取得と検証
    UINT numViewports = 1;
    D3D11_VIEWPORT vp = {};
    context->RSGetViewports(&numViewports, &vp);

    if (numViewports == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f) return;

    // ===== Live2D描画メインブロック =====
    try {
        // フレーム開始
        Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

        auto renderer = _model->GetMyRenderer();
        if (renderer) {
            renderer->SetDefaultRenderState();

            // プリミティブトポロジーを設定
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // キャッシュ済みパイプラインステートを適用
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

            // 不要なシェーダーステージを無効化
            context->GSSetShader(nullptr, nullptr, 0);
            context->HSSetShader(nullptr, nullptr, 0);
            context->DSSetShader(nullptr, nullptr, 0);

            // 投影行列の設定
            Live2D::Cubism::Framework::CubismMatrix44 matrix;
            matrix.LoadIdentity();

            float modelCanvasH = _model->GetModel()->GetCanvasHeight();
            if (modelCanvasH == 0) modelCanvasH = 2000.0f;

            float aspect = vp.Width / vp.Height;
            
            // キャンバス高さに基づくスケーリング（NDCに正規化）
            float scale = 2.0f / modelCanvasH; 
            matrix.Scale(scale / aspect, scale);

            // アクターのトランスフォーム（位置・スケール）を行列に適用
            if (auto owner = actorRef.Target()) {
                Vector3 pos = owner->transform.GetPosition();
                Vector3 scl = owner->transform.GetScale();
                matrix.Translate(pos.x, pos.y);
                matrix.Scale(scl.x, scl.y);
            }

            // モデル描画
            _model->Draw(device, context, matrix);
        }

        // フレーム終了
        Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);

    } catch (const std::exception& e) {
        OutputDebugStringA("Live2DComponent::Draw - Exception: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
    } catch (...) {
        OutputDebugStringA("Live2DComponent::Draw - Unknown Exception!\n");
    }

    // ===== パイプラインステートのクリーンアップ =====
    // （後続の描画パスへの干渉を防止）

    // シェーダーリソースの解除
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRVs);
    
    // シザー矩形のリセット
    context->RSSetScissorRects(0, nullptr);

    // 全シェーダーステージの解除
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    // 入力レイアウトのリセット
    context->IASetInputLayout(nullptr);

    // プリミティブトポロジーをデフォルトに復元
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ブレンド・深度・ラスタライザステートをデフォルトに復元
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

/// デストラクタ - モデルとキャッシュ済みステートを解放する
Live2DComponent::~Live2DComponent() {
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    m_isDestroyed = true;

    // キャッシュ済みステートとモデルを解放
    ReleaseCachedStates();
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}

/// パイプラインステートを初期化する（初回のDraw時に1回だけ実行）
void Live2DComponent::InitCachedStates(ID3D11Device* device) {
    if (m_statesInitialized || !device) return;

    // ブレンドステート（アルファブレンド有効）
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

    // ラスタライザステート（カリングなし・ソリッド描画）
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    device->CreateRasterizerState(&rasterDesc, &m_cachedRasterState);

    // 深度ステンシルステート（深度テスト無効）
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDesc.StencilEnable = FALSE;
    device->CreateDepthStencilState(&depthDesc, &m_cachedDepthState);

    m_statesInitialized = true;
}

/// キャッシュ済みパイプラインステートを解放する
void Live2DComponent::ReleaseCachedStates() {
    if (m_cachedBlendState) { m_cachedBlendState->Release(); m_cachedBlendState = nullptr; }
    if (m_cachedRasterState) { m_cachedRasterState->Release(); m_cachedRasterState = nullptr; }
    if (m_cachedDepthState) { m_cachedDepthState->Release(); m_cachedDepthState = nullptr; }
    m_statesInitialized = false;
}
