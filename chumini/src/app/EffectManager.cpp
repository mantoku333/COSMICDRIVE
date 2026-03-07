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

    // ----------------------------------------------------------------
    // Effekseer縺ｮ蛻晄悄蛹・
    // ----------------------------------------------------------------
    void EffectManager::InitializeEffekseer(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (device != nullptr && context != nullptr) {
            efkRenderer = EffekseerRendererDX11::Renderer::Create(device, context, 2000);
            efkManager = Effekseer::Manager::Create(2000);

            m_context = context;

            if (efkRenderer != nullptr && efkManager != nullptr) {

                // 蟾ｦ謇句ｺｧ讓咏ｳｻ・・irectX讓呎ｺ厄ｼ峨↓險ｭ螳壹☆繧・
                efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

                // Effekseer縺梧緒逕ｻ縺励◆蠕後↓縲∝・縺ｮDirectX縺ｮ險ｭ螳壹↓謌ｻ縺・
                efkRenderer->SetRestorationOfStatesFlag(true);

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

    // ----------------------------------------------------------------
    // 2D繧ｹ繝励Λ繧､繝医・蛻晄悄蛹・
    // ----------------------------------------------------------------
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

    // ----------------------------------------------------------------
    // Effekseer 讖溯・
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

            // 繧ｷ繧ｧ繝ｼ繝繝ｼ隗｣髯､
            m_context->GSSetShader(nullptr, nullptr, 0);
            m_context->HSSetShader(nullptr, nullptr, 0);
            m_context->DSSetShader(nullptr, nullptr, 0);

            // 繝医・繝ｭ繧ｸ繝ｼ繧ょｿｵ縺ｮ縺溘ａ繝ｪ繧ｻ繝・ヨ・・ffekseer縺ｯTriangleList繧剃ｽｿ縺・∪縺吶′縲∝ｿｵ縺ｮ縺溘ａ・・
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            efkRenderer->BeginRendering();
            efkManager->Draw();
            efkRenderer->EndRendering();
        }
    }

    // ----------------------------------------------------------------
    // 繧ｹ繝励Λ繧､繝・讖溯・
    // ----------------------------------------------------------------
    void EffectManager::SpawnSprite(float x, float y, float scale, float duration, const Color& color) {
        SpawnSprite(x, y, scale, scale, duration, color);
    }

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

    // ----------------------------------------------------------------
    // 蜈ｱ騾壹・譖ｴ譁ｰ
    // ----------------------------------------------------------------
    void EffectManager::Update(const sf::command::ICommand&) {

        float dt = sf::Time::DeltaTime();

        // 繧ｹ繝励Λ繧､繝域峩譁ｰ
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
                // UV繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ險育ｮ・
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

        // Effekseer譖ｴ譁ｰ
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

    void EffectManager::SetScale(Effekseer::Handle handle, float x, float y, float z) {
        if (efkManager != nullptr) {
            // Effekseer縺ｮ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｫ繝上Φ繝峨Ν縺ｮ繧ｵ繧､繧ｺ螟画峩繧剃ｾ晞ｼ
            efkManager->SetScale(handle, x, y, z);
        }
    }
}