#include "EffectManager.h"
#include "sf/Time.h"
#include "Debug.h"

namespace app::test {

    EffectManager::EffectManager() {}

    EffectManager::~EffectManager() {
        spritePool.clear();
        efkResourceMap.clear();
        efkManager = nullptr;
        efkRenderer = nullptr;
    }

    // ===== Effekseer初期化 =====

    /// Effekseerのレンダラーとマネージャーを生成し、各種レンダラーを登録する
    void EffectManager::InitializeEffekseer(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (device != nullptr && context != nullptr) {
            efkRenderer = EffekseerRendererDX11::Renderer::Create(device, context, 2000);
            efkManager = Effekseer::Manager::Create(2000);

            m_context = context;

            if (efkRenderer != nullptr && efkManager != nullptr) {
                // 左手座標系に設定（DirectXに合わせる）
                efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

                // 描画ステートの自動復元を有効化
                efkRenderer->SetRestorationOfStatesFlag(true);

                // 各種レンダラー・ローダーを登録
                efkManager->SetSpriteRenderer(efkRenderer->CreateSpriteRenderer());
                efkManager->SetRibbonRenderer(efkRenderer->CreateRibbonRenderer());
                efkManager->SetRingRenderer(efkRenderer->CreateRingRenderer());
                efkManager->SetTrackRenderer(efkRenderer->CreateTrackRenderer());
                efkManager->SetModelRenderer(efkRenderer->CreateModelRenderer());
                efkManager->SetTextureLoader(efkRenderer->CreateTextureLoader());
                efkManager->SetModelLoader(efkRenderer->CreateModelLoader());
                efkManager->SetMaterialLoader(efkRenderer->CreateMaterialLoader());
            }
        }
        updateCommand.Bind(std::bind(&EffectManager::Update, this, std::placeholders::_1));
    }

    // ===== スプライト初期化 =====

    /// スプライトエフェクト用のオブジェクトプールを生成する
    void EffectManager::InitializeSprite(ImageFactory factory, sf::Texture* texture, int poolSize) {
        if (factory && texture) {
            spritePool.clear();
            spritePool.reserve(poolSize);

            for (int i = 0; i < poolSize; ++i) {
                sf::SafePtr<sf::ui::Image> img = factory();

                if (!img.isNull()) {
                    img->material.texture = texture;
                    img->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
                    img->SetVisible(false);

                    EffectInstance instance;
                    instance.image = img;
                    instance.active = false;
                    spritePool.push_back(instance);
                }
            }
        }
        updateCommand.Bind(std::bind(&EffectManager::Update, this, std::placeholders::_1));
    }

    // ===== Effekseerエフェクト操作 =====

    /// エフェクトファイルを読み込み、キー名でリソースマップに登録する
    void EffectManager::LoadEffekseer(const std::string& key, const std::u16string& path) {
        if (efkManager == nullptr) return;
        auto effect = Effekseer::Effect::Create(efkManager, path.c_str());
        if (effect != nullptr) {
            efkResourceMap[key] = effect;
        }
    }

    /// 登録済みエフェクトをキー名で再生し、ハンドルを返す
    Effekseer::Handle EffectManager::PlayEffekseer(const std::string& key, float x, float y, float z) {
        if (efkManager == nullptr) return -1;
        if (efkResourceMap.find(key) == efkResourceMap.end()) return -1;
        return efkManager->Play(efkResourceMap[key], x, y, z);
    }

    /// Effekseerエフェクトを描画する
    void EffectManager::DrawEffekseer() {
        if (efkManager != nullptr && efkRenderer != nullptr) {
            // 不要なシェーダーステージを無効化
            m_context->GSSetShader(nullptr, nullptr, 0);
            m_context->HSSetShader(nullptr, nullptr, 0);
            m_context->DSSetShader(nullptr, nullptr, 0);

            // プリミティブトポロジーを設定
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Effekseer描画
            efkRenderer->BeginRendering();
            efkManager->Draw();
            efkRenderer->EndRendering();
        }
    }

    // ===== スプライトエフェクト操作 =====

    /// スプライトエフェクトを再生する（均一スケール版）
    void EffectManager::SpawnSprite(float x, float y, float scale, float duration, const Color& color) {
        SpawnSprite(x, y, scale, scale, duration, color);
    }

    /// プールから未使用のスプライトを取得し、エフェクトを再生する
    void EffectManager::SpawnSprite(float x, float y, float scaleX, float scaleY, float duration, const Color& color) {
        for (auto& effect : spritePool) {
            if (!effect.active) {
                if (effect.image.isNull()) continue;
                effect.active = true;
                effect.timer = 0.0f;
                effect.duration = duration;
                effect.image->transform.SetPosition(Vector3(x, y, 0));
                effect.image->transform.SetScale(Vector3(scaleX, scaleY, 0));
                effect.image->material.SetColor({ color.r, color.g, color.b, color.a });
                effect.image->SetVisible(true);
                return;
            }
        }
    }

    // ===== 共通更新処理 =====

    /// スプライトとEffekseerの毎フレーム更新
    void EffectManager::Update(const sf::command::ICommand&) {

        float dt = sf::Time::DeltaTime();

        // --- スプライトエフェクトの更新 ---
        for (auto& effect : spritePool) {
            if (!effect.active) continue;
            if (effect.image.isNull()) { effect.active = false; continue; }

            effect.timer += dt;

            // 再生時間を超えたら非表示にして返却
            if (effect.timer >= effect.duration) {
                effect.active = false;
                effect.image->SetVisible(false);
                continue;
            }

            // スプライトシートのフレームを進行率に応じて更新
            if (effect.duration > 0.0f) {
                float t = effect.timer / effect.duration;
                int totalFrames = gridCols * gridRows;
                int currentFrame = static_cast<int>(t * totalFrames);
                if (currentFrame >= totalFrames) currentFrame = totalFrames - 1;

                int col = currentFrame % gridCols;
                int row = currentFrame / gridCols;
                float uSize = 1.0f / gridCols;
                float vSize = 1.0f / gridRows;
                effect.image->SetUV(col * uSize, row * vSize, (col + 1) * uSize, (row + 1) * vSize);
            }
        }

        // --- Effekseerの更新（60FPS基準に変換） ---
        if (efkManager != nullptr) {
            efkManager->Update(dt * 60.0f);
        }
    }

    /// 全スプライトを非表示にし、全Effekseerエフェクトを停止する
    void EffectManager::ClearAll() {
        for (auto& effect : spritePool) {
            effect.active = false;
            if (!effect.image.isNull()) effect.image->SetVisible(false);
        }
        if (efkManager != nullptr) efkManager->StopAllEffects();
    }

    /// Effekseerの射影行列を設定する
    void EffectManager::SetProjectionMatrix(const Effekseer::Matrix44& proj) {
        if (efkRenderer != nullptr) efkRenderer->SetProjectionMatrix(proj);
    }

    /// Effekseerのカメラ行列を設定する
    void EffectManager::SetCameraMatrix(const Effekseer::Matrix44& camera) {
        if (efkRenderer != nullptr) efkRenderer->SetCameraMatrix(camera);
    }

    /// Effekseerエフェクトのスケールを設定する
    void EffectManager::SetScale(Effekseer::Handle handle, float x, float y, float z) {
        if (efkManager != nullptr) {
            efkManager->SetScale(handle, x, y, z);
        }
    }
}
