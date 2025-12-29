#include "TitleScene.h"
#include "TitleCanvas.h"
#include "SceneChangeComponent.h"
#include "DirectWrite.h"
#include "EffectManager.h"
#include "Live2DComponent.h"

// 追加インクルード
#include "DirectX11.h"   // デバイス取得用
#include "SInput.h"      // 入力取得用
#include <string>        // ログ出力用
#include <DirectXMath.h> // 行列計算用

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

namespace app::test {

// =================================================================
    // 初期化 (Init)
    // =================================================================
    void TitleScene::Init()
    {
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
        
        // 2. Live2D表示用アクター
        {
            auto live2dActor = Instantiate();
            l2dComp = live2dActor.Target()->AddComponent<Live2DComponent>();
            if (l2dComp.Get()) {
                l2dComp->LoadModel("Assets/Live2D/Hiyori", "Hiyori.model3.json");
            }
        }

        DirectWrite();

        bgmPlayer->SetPath("Assets/Sound/BGM.wav");
        bgmPlayer->Play();


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

                effectManager->LoadEffekseer("Test", u"Assets/Effects/meteo.efk");

                auto handle = effectManager->PlayEffekseer("Icon", -1.0f, 1.0f, 0.0f);

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
    }

    // =================================================================
    // 更新 (Update)
    // =================================================================
    void TitleScene::Update(const sf::command::ICommand& command)
    {
        if (l2dComp.Get()) {
            l2dComp->Update();
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

        // 2. Live2Dの描画
        if (l2dComp.Get()) {
            l2dComp->Draw();
        }

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
}