#include "TitleScene.h"
#include "TitleCanvas.h"
#include "SceneChangeComponent.h"
#include "DirectWrite.h"
#include "EffectManager.h"
#include "Live2DComponent.h"
#include "BGMComponent.h"

// 追加インクルード
#include "DirectX11.h"   // デバイス取得用
#include "SInput.h"      // 入力取得用
#include <string>        // ログ出力用
#include <DirectXMath.h> // 行列計算用
#include <CubismFramework.hpp>
#include <Id/CubismIdManager.hpp>
#include <CubismFramework.hpp>
#include <Id/CubismIdManager.hpp>
#include <Windows.h> 
#include <algorithm>
#include "Mesh.h"
#include "Geometry.h"
//#include "GeometryCube.h" // Reverted

using namespace DirectX; // 行列系を短く書くため

// =================================================================
// ヘルパー関数: DirectXの行列をEffekseer形式に変換
// =================================================================
static Effekseer::Matrix44 ToEffekseerMatrix(const DirectX::XMMATRIX& mat) {
    Effekseer::Matrix44 ret;
    DirectX::XMFLOAT4X4 f;
    DirectX::XMStoreFloat4x4(&f, mat);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ret.Values[i][j] = f.m[i][j];
        }
    }
    return ret;
}

// =================================================================
// Model Configuration (Change these paths to switch models)
// =================================================================
namespace Config {
    // Relative path to the model directory
    const std::string MODEL_DIR = "Assets/Live2D/CyberCat";
    
    // Model .model3.json filename
    const std::string MODEL_FILE = "CyberCat.model3.json";
    
    // Glitch Motion Name (File name without extension or Group name)
    const std::string GLITCH_MOTION_NAME = "GlitchNoise"; 
}

namespace app::test {

// =================================================================
    // 初期化 (Init)
    // =================================================================
    void TitleScene::Init()
    {
        loadProgress = 0.0f;
        sf::debug::Debug::Log("TitleScene: Init開始");

        // 1. UI管理用アクターの生成とコンポーネント追加
        {
            uiManagerActor = Instantiate();
            uiManagerActor.Target()->AddComponent<TitleCanvas>();
            uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
            uiManagerActor.Target()->AddComponent<EffectManager>(); // EffectManager追加
            bgmPlayer = uiManagerActor.Target()->AddComponent<app::test::BGMComponent>();

            uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });
        }
        
        loadProgress = 0.2f;

        // 2. Live2D表示用アクター
        {
            auto live2dActor = Instantiate();
            l2dComp = live2dActor.Target()->AddComponent<Live2DComponent>();
            if (l2dComp.Get()) {
                // Load Model using Config
                l2dComp->LoadModel(Config::MODEL_DIR, Config::MODEL_FILE);

                loadProgress = 0.5f;

                // Set Half Size
                live2dActor.Target()->transform.SetScale({ 0.7f, 1.f, 1.0f });

                // Start Idle animation
                l2dComp->PlayMotion("Idle", 0, 3);
                
                // Start Loop Glitch Animation (Parallel)
                sf::debug::Debug::Log("TitleScene: Calling StartGlitchMotion...");
                l2dComp->StartGlitchMotion(Config::GLITCH_MOTION_NAME.c_str(), 0);
            }

            // [Hack] Reverted g_cube approach
        }

        loadProgress = 0.6f;

        DirectWrite();

        bgmPlayer->SetPath("Assets/Sound/BGM.wav");
        bgmPlayer->Play();

        loadProgress = 0.7f;

        // 2. Effekseerの初期化
        auto* dx11 = sf::dx::DirectX11::Instance();
        ID3D11Device* device = dx11->GetMainDevice().GetDevice();
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context); // コンテキスト取得

        if (auto actor = uiManagerActor.Target()) {
            if (auto effectManager = actor->GetComponent<EffectManager>()) {

                // 初期化実行
                effectManager->InitializeEffekseer(device, context);

                // ファイル読み込み (icon.efk)
                // キー名は "Icon" とします
                effectManager->LoadEffekseer("Icon", u"Assets/Effects/TTT.efk");

                loadProgress = 0.8f;

                effectManager->LoadEffekseer("Test", u"Assets/Effects/meteo.efk");

                auto handle = effectManager->PlayEffekseer("Icon", -3.0f, 1.0f, 0.0f);

                auto handle2 = effectManager->PlayEffekseer("Test", 0.0f,-5.0f, 0.0f);

                effectManager->SetScale(handle, 4.5f, 3.5f, 1.0f);
                effectManager->SetScale(handle2, 3.5f, 3.5f, 1.0f);
            }
        }
        if (context) context->Release();

        // 初期パラメーター設定
        selectedButton = 0;
        isButtonPressed = false;

        updateCommand.Bind(std::bind(&TitleScene::Update, this, std::placeholders::_1));

        loadProgress = 1.0f;
    }

    // =================================================================
    // 更新 (Update)
    // =================================================================
    void TitleScene::Update(const sf::command::ICommand& command)
    {
        if (l2dComp.Get()) {
            l2dComp->Update();

            // Eye Tracking (Mouse Follow)
            POINT p;
            if (GetCursorPos(&p)) {
                auto* dx11 = sf::dx::DirectX11::Instance();
                if (HWND hwnd = dx11->GetHWND()) {
                    ScreenToClient(hwnd, &p);

                    // Get Window Size (Using Client Rect for accuracy)
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    float w = static_cast<float>(rc.right - rc.left);
                    float h = static_cast<float>(rc.bottom - rc.top);

                    if (w > 0 && h > 0) {
                        // Normalize to -1.0 ~ 1.0 (Center is 0,0)
                        float nx = (p.x / w) * 2.0f - 1.0f;
                        float ny = -((p.y / h) * 2.0f - 1.0f); // Invert Y

                        // Clamp (Keep within screen + margin)
                        nx = std::max(-1.5f, std::min(1.5f, nx));
                        ny = std::max(-1.5f, std::min(1.5f, ny));

                        l2dComp->SetDragging(nx, ny);
                    }
                }
            }
        }

        // Check for M key to play TapBody motion
        if (SInput::Instance().GetKeyDown(Key::KEY_M)) {
            if (l2dComp.Get()) {
                l2dComp->PlayMotion("TapBody", 0, 3);
            }
        }

        // Fキーが押されたらエフェクト再生
        if (SInput::Instance().GetKeyDown(Key::KEY_F)) {

            if (auto actor = uiManagerActor.Target()) {
                if (auto effectManager = actor->GetComponent<EffectManager>()) {

                    auto handle = effectManager->PlayEffekseer("Test", 0.0f, 0.0f, 1.0f);

                    // 必要なら大きさも変える
                    effectManager->SetScale(handle, 2.0f, 2.0f, 2.0f);
                }
            }
        }
    }

    void TitleScene::Draw()
    {
        // 1. 基底クラスの描画 (他のコンポーネント)
        sf::Scene<TitleScene>::Draw();

        // 3. Effekseer (既存処理)
        auto actor = uiManagerActor.Target();
        if (!actor) return;

        auto effectManager = actor->GetComponent<EffectManager>();
        if (!effectManager) return;

        XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -20.0f, 0.0f);
        XMVECTOR focus = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX viewMat = XMMatrixLookAtLH(eye, focus, up);
        float aspect = 1920.0f / 1080.0f;
        XMMATRIX projMat = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspect, 0.1f, 1000.0f);

        effectManager->SetCameraMatrix(ToEffekseerMatrix(viewMat));
        effectManager->SetProjectionMatrix(ToEffekseerMatrix(projMat));
        effectManager->DrawEffekseer();
    }

    void TitleScene::DrawOverlay()
    {
        // 2. Live2Dの描画 (最前面)
        if (l2dComp.Get()) {
            l2dComp->Draw();
        }
    }

    void TitleScene::OnGUI()
    {
        ImGui::Begin("Live2D Control");
        
        if (ImGui::Button("Play Idle")) {
            if (l2dComp.Get()) {
                l2dComp->PlayMotion("Idle", 0, 3);
            }
        }

        if (ImGui::Button("Play TapBody")) {
            if (l2dComp.Get()) {
                l2dComp->PlayMotion("TapBody", 0, 3);
            }
        }

        ImGui::Separator();
        ImGui::Text("Expression Test");

        static bool manualOverride = false;
        ImGui::Checkbox("Enable Manual Override", &manualOverride);

        if (l2dComp.Get() && l2dComp->GetAppModel() && l2dComp->GetAppModel()->GetModel()) {
             auto* model = l2dComp->GetAppModel()->GetModel();
             auto* idManager = Live2D::Cubism::Framework::CubismFramework::GetIdManager();

             // Static helpers to hold values (Not ideal for multiple instances, but fine for singleton TitleScene)
             static float v_mouth = 0.0f;
             static float v_eyeL = 1.0f;
             static float v_eyeR = 1.0f;
             static float v_browL = 0.0f;
             static float v_browR = 0.0f;
             static float v_angX = 0.0f;
             static float v_angY = 0.0f;
             static float v_angZ = 0.0f;
             static float v_bodyX = 0.0f;

             if (!manualOverride) {
                 l2dComp->GetAppModel()->ClearParamOverrides();
             }

             auto ProcessParam = [&](const char* label, const char* idStr, float* valPtr, float min, float max) {
                 Live2D::Cubism::Framework::CubismIdHandle pid = idManager->GetId(idStr);
                 int idx = model->GetParameterIndex(pid);
                 if (idx >= 0) {
                     // If NOT manual, sync slider to model
                     if (!manualOverride) {
                         *valPtr = model->GetParameterValue(idx);
                     }
                     
                     // Display Slider
                     ImGui::SliderFloat(label, valPtr, min, max);

                     // If manual, force value using override system
                     if (manualOverride) {
                         l2dComp->GetAppModel()->SetParamOverride(idStr, *valPtr);
                     }
                 }
             };

             ProcessParam("Mouth Open", "ParamMouthOpenY", &v_mouth, 0.0f, 1.0f);
             ProcessParam("EyeL Open", "ParamEyeLOpen", &v_eyeL, 0.0f, 1.0f);
             ProcessParam("EyeR Open", "ParamEyeROpen", &v_eyeR, 0.0f, 1.0f);
             ProcessParam("BrowL Y", "ParamBrowLY", &v_browL, -1.0f, 1.0f);
             ProcessParam("BrowR Y", "ParamBrowRY", &v_browR, -1.0f, 1.0f);
             
             ImGui::Separator();
             ProcessParam("Angle X", "ParamAngleX", &v_angX, -30.0f, 30.0f);
             ProcessParam("Angle Y", "ParamAngleY", &v_angY, -30.0f, 30.0f);
             ProcessParam("Angle Z", "ParamAngleZ", &v_angZ, -30.0f, 30.0f);
             ProcessParam("Body X", "ParamBodyAngleX", &v_bodyX, -10.0f, 10.0f);

             ImGui::Separator();
             ImGui::Text("Glitch Parameters");
             static float v_noise = 0.0f;
             static float v_chroma = 0.0f;
             static float v_pink = 0.0f;
             static float v_cyan = 0.0f;

             ProcessParam("Noise", "ParamNoise", &v_noise, -30.0f, 30.0f);
             ProcessParam("Chromatic Aberration", "ParamChromaticAberration", &v_chroma, 0.0f, 1.0f);
             ProcessParam("Noise Pink", "ParamNoisePink", &v_pink, 0.0f, 1.0f);
             ProcessParam("Noise Cyan", "ParamNoiseCyan", &v_cyan, 0.0f, 1.0f);
        }

        ImGui::Separator();
        if (ImGui::Button(("Play " + Config::GLITCH_MOTION_NAME).c_str())) {
            if (l2dComp.Get()) {
                l2dComp->PlayMotion(Config::GLITCH_MOTION_NAME.c_str(), 0, 3);
            }
        }

        ImGui::End();
    }
}