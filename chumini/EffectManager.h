#pragma once
#include "Component.h"
#include "Image.h"
#include "Texture.h"
#include <vector>
#include <map>
#include <string>
#include <functional> // std::function用

// Effekseer
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

namespace app::test {

    struct Color {
        float r, g, b, a;
    };

    class EffectManager : public sf::Component {
    public:
        EffectManager();
        virtual ~EffectManager();

        // ========================================================================
        // ★初期化メソッド（使いたい機能だけ呼んでください）
        // ========================================================================

        // 1. Effekseerを使う場合の初期化 (3Dエフェクト用)
        //    UIやCanvasは不要です。
        void InitializeEffekseer(ID3D11Device* device, ID3D11DeviceContext* context);

        // 2. 2Dスプライトを使う場合の初期化 (板ポリ用)
        //    UIを生成するための関数(ラムダ式)を渡します。
        using ImageFactory = std::function<sf::SafePtr<sf::ui::Image>()>;
        void InitializeSprite(ImageFactory factory, sf::Texture* texture, int poolSize = 20);


        // ========================================================================
        // ★操作メソッド
        // ========================================================================

        // Effekseerの再生
        Effekseer::Handle PlayEffekseer(const std::string& key, float x, float y, float z = 0.0f);
        // Effekseerファイルのロード
        void LoadEffekseer(const std::string& key, const std::u16string& path);
        // Effekseerの描画 (シーンのDrawで呼ぶ)
        void DrawEffekseer();


        // スプライトの再生
        void SpawnSprite(float x, float y, float scale, float duration, const Color& color);


        // 共通
        void ClearAll(); // 全停止
        void Update(const sf::command::ICommand& cmd); // 更新

        // カメラ設定
        void SetProjectionMatrix(const Effekseer::Matrix44& proj);
        void SetCameraMatrix(const Effekseer::Matrix44& camera);

    private:
        // スプライト管理用
        struct EffectInstance {
            sf::SafePtr<sf::ui::Image> image;
            bool active = false;
            float timer = 0.0f;
            float duration = 0.0f;
        };
        std::vector<EffectInstance> spritePool;
        static constexpr int COLUMNS = 8;
        static constexpr int ROWS = 8;

        // Effekseer管理用
        EffekseerRenderer::RendererRef efkRenderer;
        Effekseer::ManagerRef efkManager;
        std::map<std::string, Effekseer::EffectRef> efkResourceMap;

        sf::command::Command<> updateCommand;
    };
}