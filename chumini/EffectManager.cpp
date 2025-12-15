#include "EffectManager.h"
#include "Time.h"
#include "Debug.h"

namespace app::test {

    EffectManager::EffectManager() {}

    EffectManager::~EffectManager() {
        spritePool.clear();
        efkResourceMap.clear();
        efkManager = nullptr;
        efkRenderer = nullptr;
    }

    // ----------------------------------------------------------------
    // Effekseerの初期化
    // ----------------------------------------------------------------
    void EffectManager::InitializeEffekseer(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (device != nullptr && context != nullptr) {
            efkRenderer = EffekseerRendererDX11::Renderer::Create(device, context, 2000);
            efkManager = Effekseer::Manager::Create(2000);

            if (efkRenderer != nullptr && efkManager != nullptr) {
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
        // Update登録 (重複登録しても問題ないよう設計するか、確認する)
        updateCommand.Bind(std::bind(&EffectManager::Update, this, std::placeholders::_1));
    }

    // ----------------------------------------------------------------
    // 2Dスプライトの初期化
    // ----------------------------------------------------------------
    void EffectManager::InitializeSprite(ImageFactory factory, sf::Texture* texture, int poolSize) {
        if (factory && texture) {
            spritePool.clear();
            spritePool.reserve(poolSize);

            for (int i = 0; i < poolSize; ++i) {
                // 渡された工場関数を使ってUIを作る (Canvasへの依存なし)
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

    // ----------------------------------------------------------------
    // Effekseer 機能
    // ----------------------------------------------------------------
    void EffectManager::LoadEffekseer(const std::string& key, const std::u16string& path) {
        if (efkManager == nullptr) return;
        auto effect = Effekseer::Effect::Create(efkManager, path.c_str());
        if (effect != nullptr) {
            efkResourceMap[key] = effect;
        }
    }

    Effekseer::Handle EffectManager::PlayEffekseer(const std::string& key, float x, float y, float z) {
        if (efkManager == nullptr) return -1;
        if (efkResourceMap.find(key) == efkResourceMap.end()) return -1;
        return efkManager->Play(efkResourceMap[key], x, y, z);
    }

    void EffectManager::DrawEffekseer() {
        if (efkManager != nullptr && efkRenderer != nullptr) {
            efkRenderer->BeginRendering();
            efkManager->Draw();
            efkRenderer->EndRendering();
        }
    }

    // ----------------------------------------------------------------
    // スプライト 機能
    // ----------------------------------------------------------------
    void EffectManager::SpawnSprite(float x, float y, float scale, float duration, const Color& color) {
        for (auto& effect : spritePool) {
            if (!effect.active) {
                if (effect.image.isNull()) continue;
                effect.active = true;
                effect.timer = 0.0f;
                effect.duration = duration;
                effect.image->transform.SetPosition(Vector3(x, y, 0));
                effect.image->transform.SetScale(Vector3(scale, scale, 0));
                effect.image->material.SetColor({ color.r, color.g, color.b, color.a });
                effect.image->SetVisible(true);
                return;
            }
        }
    }

    // ----------------------------------------------------------------
    // 共通・更新
    // ----------------------------------------------------------------
    void EffectManager::Update(const sf::command::ICommand&) {
        float dt = sf::Time::DeltaTime();

        // スプライト更新
        for (auto& effect : spritePool) {
            if (!effect.active) continue;
            if (effect.image.isNull()) { effect.active = false; continue; }

            effect.timer += dt;
            if (effect.timer >= effect.duration) {
                effect.active = false;
                effect.image->SetVisible(false);
                continue;
            }
            if (effect.duration > 0.0f) {
                // UVアニメーション計算
                float t = effect.timer / effect.duration;
                int totalFrames = COLUMNS * ROWS;
                int currentFrame = static_cast<int>(t * totalFrames);
                if (currentFrame >= totalFrames) currentFrame = totalFrames - 1;
                int col = currentFrame % COLUMNS;
                int row = currentFrame / COLUMNS;
                float uSize = 1.0f / COLUMNS;
                float vSize = 1.0f / ROWS;
                effect.image->SetUV(col * uSize, row * vSize, (col + 1) * uSize, (row + 1) * vSize);
            }
        }

        // Effekseer更新
        if (efkManager != nullptr) {
            efkManager->Update(dt * 60.0f);
        }
    }

    void EffectManager::ClearAll() {
        for (auto& effect : spritePool) {
            effect.active = false;
            if (!effect.image.isNull()) effect.image->SetVisible(false);
        }
        if (efkManager != nullptr) efkManager->StopAllEffects();
    }

    void EffectManager::SetProjectionMatrix(const Effekseer::Matrix44& proj) {
        if (efkRenderer != nullptr) efkRenderer->SetProjectionMatrix(proj);
    }
    void EffectManager::SetCameraMatrix(const Effekseer::Matrix44& camera) {
        if (efkRenderer != nullptr) efkRenderer->SetCameraMatrix(camera);
    }
}