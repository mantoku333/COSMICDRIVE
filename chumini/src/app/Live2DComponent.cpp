#include "Live2DComponent.h"
#include "Live2DManager.h"
#include "DirectX11.h"
#include "Actor.h"
#include "Scene.h" // IScene螳夂ｾｩ逕ｨ

using namespace app::test;
using namespace sf;

namespace {
    std::mutex s_live2dGlobalMutex;
    std::mutex s_live2dSdkInitMutex;
}

void Live2DComponent::Begin() {
    // Live2DManager縺ｮ蛻晄悄蛹厄ｼ域里縺ｫ縺輔ｌ縺ｦ縺・ｋ蝣ｴ蜷医・蜀・Κ縺ｧ蠑ｾ縺九ｌ繧区Φ螳壹□縺悟ｿｵ縺ｮ縺溘ａ・・
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
    // 笘・せ繝ｬ繝・ラ繧ｻ繝ｼ繝・ Draw荳ｭ縺ｯ繝・せ繝医Λ繧ｯ繧ｿ繧貞ｾ・ｩ溘＆縺帙ｋ
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    // 笘・す繝ｼ繝ｳ驕ｷ遘ｻ繧ｯ繝ｩ繝・す繝･髦ｲ豁｢: 遐ｴ譽・ｸ医∩縺ｪ繧画緒逕ｻ繧ｹ繧ｭ繝・・
    if (m_isDestroyed) {
        OutputDebugStringA("Live2DComponent::Draw - SKIPPED (destroyed)\n");
        return;
    }

    // 笘・す繝ｼ繝ｳ驕ｷ遘ｻ繧ｯ繝ｩ繝・す繝･髦ｲ豁｢: 繧ｷ繝ｼ繝ｳ縺轡eActivate迥ｶ諷九↑繧画緒逕ｻ繧ｹ繧ｭ繝・・
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

    // 笘・ョ繝舌う繧ｹ/繧ｳ繝ｳ繝・く繧ｹ繝医・譛牙柑諤ｧ繝√ぉ繝・け・・ntel GPU繧ｯ繝ｩ繝・す繝･髦ｲ豁｢・・
    if (!device || !context) {
        OutputDebugStringA("Live2DComponent::Draw - Device or Context is null, skipping draw.\n");
        return;
    }

    const HRESULT removedReason = device->GetDeviceRemovedReason();
    if (FAILED(removedReason)) {
        OutputDebugStringA("Live2DComponent::Draw - Device removed, skipping draw.\n");
        return;
    }

    // Live2D縺ｮ謠冗判

    // =========================================================
    // 謠冗判螳溯｡・
    // =========================================================
    UINT numViewports = 1;
    D3D11_VIEWPORT vp = {};
    context->RSGetViewports(&numViewports, &vp);

    // 笘・ン繝･繝ｼ繝昴・繝医・譛牙柑諤ｧ繝√ぉ繝・け・医ぞ繝ｭ髯､邂鈴亟豁｢・・
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
            renderer->SetDefaultRenderState(); // Live2D蛛ｴ縺ｮ繝・ヵ繧ｩ繝ｫ繝郁ｨｭ螳壹ｂ驕ｩ逕ｨ

            // =========================================================
            // 繝ｬ繝ｳ繝繝ｪ繝ｳ繧ｰ繧ｹ繝・・繝医・騾驕ｿ縺ｨ險ｭ螳・(SetDefaultRenderState蠕後↓蠑ｷ蛻ｶ驕ｩ逕ｨ)
            // =========================================================

            // 繝医・繝ｭ繧ｸ繝ｼ繧担trip縺九ｉList縺ｸ螟画峩
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 笘・く繝｣繝・す繝･縺輔ｌ縺溘せ繝・・繝医ｒ菴ｿ逕ｨ・域ｯ弱ヵ繝ｬ繝ｼ繝逕滓・繧貞ｻ・ｭ｢・・
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

            // 繧ｷ繧ｧ繝ｼ繝繝ｼ縺ｮ辟｡蜉ｹ蛹厄ｼ亥ｹｲ貂蛾亟豁｢・・
            context->GSSetShader(nullptr, nullptr, 0);
            context->HSSetShader(nullptr, nullptr, 0);
            context->DSSetShader(nullptr, nullptr, 0);

            // 陦悟・險育ｮ・
            Live2D::Cubism::Framework::CubismMatrix44 matrix;
            matrix.LoadIdentity();

            float modelCanvasH = _model->GetModel()->GetCanvasHeight();
            if (modelCanvasH == 0) modelCanvasH = 2000.0f;

            float aspect = vp.Width / vp.Height;
            
            // 逕ｻ髱｢縺ｫ蜿弱∪繧九ｈ縺・↓繧ｹ繧ｱ繝ｼ繝ｫ隱ｿ謨ｴ (Base Fit)
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
    // 縲宣㍾隕√代Ξ繝ｳ繝繝ｪ繝ｳ繧ｰ繧ｹ繝・・繝医・繝ｪ繧ｻ繝・ヨ (蠕檎援莉倥￠)
    // =========================================================

    // 笘・esource Unbind (Driver Clean-up)
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRVs);
    
    // 1. 繧ｷ繧ｶ繝ｼ遏ｩ蠖｢縺ｮ辟｡蜉ｹ蛹・(縺薙ｌ縺梧ｮ九▲縺ｦ縺・ｋ縺ｨUI縺ｪ縺ｩ縺梧ｶ域ｻ・☆繧・
    context->RSSetScissorRects(0, nullptr);

    // 2. 繧ｷ繧ｧ繝ｼ繝繝ｼ縺ｮ隗｣髯､ (谺｡縺ｮ謠冗判縺ｸ縺ｮ蟷ｲ貂蛾亟豁｢)
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    // 3. 繧､繝ｳ繝励ャ繝医Ξ繧､繧｢繧ｦ繝医・隗｣髯､
    context->IASetInputLayout(nullptr);

    // 4. 繝医・繝ｭ繧ｸ繝ｼ繧貞渕譛ｬ縺ｮTriangleList縺ｫ謌ｻ縺・
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 笘・. 繝悶Ξ繝ｳ繝峨せ繝・・繝医・豺ｱ蠎ｦ繧ｹ繝・Φ繧ｷ繝ｫ繝ｻ繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｶ繧偵ョ繝輔か繝ｫ繝医↓謌ｻ縺呻ｼ・extImage蟷ｲ貂蛾亟豁｢・・
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    // 笘・. GPU蜷梧悄: Intel GPU繝峨Λ繧､繝舌・繧ｯ繝ｩ繝・す繝･髦ｲ豁｢
    // GPU繧ｳ繝槭Φ繝峨ｒ螳御ｺ・＆縺帙※縺九ｉ谺｡縺ｮ蜃ｦ逅・↓騾ｲ繧
}

Live2DComponent::~Live2DComponent() {
    // 笘・せ繝ｬ繝・ラ繧ｻ繝ｼ繝・ Draw螳御ｺ・ｒ蠕・▲縺ｦ縺九ｉ繝ｪ繧ｽ繝ｼ繧ｹ隗｣謾ｾ
    std::scoped_lock lock(s_live2dGlobalMutex, m_drawMutex);
    
    // 笘・ｴ譽・ヵ繝ｩ繧ｰ繧呈怙蛻昴↓繧ｻ繝・ヨ・・raw蜻ｼ縺ｳ蜃ｺ縺励ｒ髦ｲ豁｢・・
    m_isDestroyed = true;
    OutputDebugStringA("Live2DComponent::~Live2DComponent - DESTRUCTOR CALLED\n");

    // 笘・PU繧ｳ繝槭Φ繝峨ヵ繝ｩ繝・す繝･: Intel GPU繝峨Λ繧､繝舌・繧ｯ繝ｩ繝・す繝･髦ｲ豁｢
    // 繝ｪ繧ｽ繝ｼ繧ｹ隗｣謾ｾ蜑阪↓GPU縺梧緒逕ｻ繧ｳ繝槭Φ繝峨ｒ螳御ｺ・☆繧九・繧貞ｾ・▽
    ReleaseCachedStates();
    if (_model) {
        delete _model;
        _model = nullptr;
    }
}

// 笘・せ繝・・繝医・蛻晄悄蛹厄ｼ井ｸ蠎ｦ縺縺大ｮ溯｡鯉ｼ・
void Live2DComponent::InitCachedStates(ID3D11Device* device) {
    if (m_statesInitialized || !device) return;

    // 繝悶Ξ繝ｳ繝峨せ繝・・繝郁ｨｭ螳・
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

    // 繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｶ繧ｹ繝・・繝郁ｨｭ螳・
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    device->CreateRasterizerState(&rasterDesc, &m_cachedRasterState);

    // 豺ｱ蠎ｦ繧ｹ繝・Φ繧ｷ繝ｫ繧ｹ繝・・繝郁ｨｭ螳・
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
