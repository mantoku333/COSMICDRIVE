#pragma once
#include "Component.h"
#include "Image.h"
#include "Texture.h"
#include <vector>
#include <functional> // std::bind用

namespace app::test {

    // Color構造体の定義
    struct Color {
        float r, g, b, a;
    };

    // 前方宣言
    class TestCanvas;

    class EffectManager : public sf::Component {
    public:
        EffectManager();
        virtual ~EffectManager();

        // 初期化
        void Initialize(TestCanvas* canvas, sf::Texture* texture, int poolSize = 20);

        // ★追加：シーン遷移時などに強制的に全エフェクトを止める関数
        void ClearAll();

        // エフェクト再生
        void Spawn(float x, float y, float scale, float duration, const Color& color);

        // 更新処理
        void Update(const sf::command::ICommand& cmd);

    private:
        struct EffectInstance {
            sf::SafePtr<sf::ui::Image> image;
            bool active = false;
            float timer = 0.0f;
            float duration = 0.0f;
        };

        std::vector<EffectInstance> pool;
        sf::Texture* targetTexture = nullptr;
        TestCanvas* ownerCanvas = nullptr;

        // アニメーション用定数
        static constexpr int COLUMNS = 8;
        static constexpr int ROWS = 8;

        // コマンドの定義
        sf::command::Command<> updateCommand;
    };
}