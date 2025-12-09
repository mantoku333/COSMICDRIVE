#include "EffectManager.h"
#include "TestCanvas.h" 
#include "Time.h"
#include "Debug.h"

namespace app::test {

    EffectManager::EffectManager() {}

    EffectManager::~EffectManager() {
        // ★クラッシュ対策★
        // デストラクタが呼ばれる頃には、CanvasやEngine側のリソースが既に破棄されている可能性があります。
        // ここでUIを触るとアクセス違反になるため、プールをメモリ上からクリアするだけに留めます。
        pool.clear();
    }

    // ★追加：シーン遷移前にこれを呼んでください
    void EffectManager::ClearAll() {
        for (auto& effect : pool) {
            effect.active = false;
            effect.timer = 0.0f;

            // UIが生きていれば非表示にする（これでDrawが呼ばれなくなる）
            if (!effect.image.isNull()) {
                effect.image->SetVisible(false);
            }
        }
        // ここで pool.clear() はしません（再利用する可能性がある場合のため）。
        // 完全に破棄したい場合は呼んだ後に delete effectManager するはずなので、
        // ここでは「動作と描画を止める」ことに専念します。
    }

    void EffectManager::Initialize(TestCanvas* canvas, sf::Texture* texture, int poolSize) {
        ownerCanvas = canvas;
        targetTexture = texture;

        if (!targetTexture || !ownerCanvas) {
            sf::debug::Debug::LogError("EffectManager: Canvas or Texture is null!");
            return;
        }

        pool.clear();
        pool.reserve(poolSize);

        for (int i = 0; i < poolSize; ++i) {
            // CanvasにUIを作ってもらう
            sf::SafePtr<sf::ui::Image> img = ownerCanvas->AddUI<sf::ui::Image>();

            if (!img.isNull()) {
                img->material.texture = targetTexture;
                img->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
                img->SetVisible(false);

                EffectInstance instance;
                instance.image = img;
                instance.active = false;
                pool.push_back(instance);
            }
        }

        // Updateを登録
        updateCommand.Bind(std::bind(&EffectManager::Update, this, std::placeholders::_1));
    }

    void EffectManager::Spawn(float x, float y, float scale, float duration, const Color& color) {
        for (auto& effect : pool) {
            if (!effect.active) {
                // UIが何かの拍子に消えていたらスキップ
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

    void EffectManager::Update(const sf::command::ICommand&) {

        float dt = sf::Time::DeltaTime();

        for (auto& effect : pool) {
            if (!effect.active) continue;

            if (effect.image.isNull()) {
                effect.active = false;
                continue;
            }

            effect.timer += dt;

            if (effect.timer >= effect.duration) {
                effect.active = false;
                effect.image->SetVisible(false);
                continue;
            }

            // ==========================================
            // UV計算（8x8対応）
            // ==========================================
            float t = effect.timer / effect.duration;
            int totalFrames = COLUMNS * ROWS;
            int currentFrame = static_cast<int>(t * totalFrames);
            if (currentFrame >= totalFrames) currentFrame = totalFrames - 1;

            int col = currentFrame % COLUMNS;
            int row = currentFrame / COLUMNS;

            // 1コマあたりのサイズ
            float uSize = 1.0f / COLUMNS;
            float vSize = 1.0f / ROWS;

            // 開始座標
            float u0 = col * uSize;
            float v0 = row * vSize;

            // 終了座標
            float u1 = (col + 1) * uSize;
            float v1 = (row + 1) * vSize;

            // SetUVには (開始x, 開始y, 終了x, 終了y) を渡す
            effect.image->SetUV(u0, v0, u1, v1);
        }
    }
}