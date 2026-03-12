#pragma once
#include "Component.h"
#include "Image.h"
#include "Texture.h"
#include <vector>
#include <map>
#include <string>
#include <functional>

#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

namespace app::test {

    /// RGBA色情報
    struct Color {
        float r, g, b, a;
    };

    /// エフェクトの管理コンポーネント
    /// Effekseer（3Dパーティクル）とスプライトシート（2Dエフェクト）の両方を統合管理する
    class EffectManager : public sf::Component {
    public:
        EffectManager();
        virtual ~EffectManager();

        // ===== 初期化 =====

        /// Effekseerの初期化（3Dエフェクト用）
        void InitializeEffekseer(ID3D11Device* device, ID3D11DeviceContext* context);

        /// スプライトエフェクトの初期化（2Dエフェクト用）
        using ImageFactory = std::function<sf::SafePtr<sf::ui::Image>()>;
        void InitializeSprite(ImageFactory factory, sf::Texture* texture, int poolSize = 20);

        // ===== Effekseer操作 =====

        /// Effekseerエフェクトファイルを読み込む
        void LoadEffekseer(const std::string& key, const std::u16string& path);

        /// Effekseerエフェクトを再生する
        Effekseer::Handle PlayEffekseer(const std::string& key, float x, float y, float z = 0.0f);

        /// Effekseerエフェクトを描画する（シーンのDraw内で呼び出す）
        void DrawEffekseer();

        /// Effekseerエフェクトのスケールを設定する
        void SetScale(Effekseer::Handle handle, float x, float y, float z);

        /// Effekseerの射影行列を設定する
        void SetProjectionMatrix(const Effekseer::Matrix44& proj);

        /// Effekseerのカメラ行列を設定する
        void SetCameraMatrix(const Effekseer::Matrix44& camera);

        // ===== スプライトエフェクト操作 =====

        /// スプライトエフェクトを再生する（均一スケール）
        void SpawnSprite(float x, float y, float scale, float duration, const Color& color);

        /// スプライトエフェクトを再生する（XY個別スケール）
        void SpawnSprite(float x, float y, float scaleX, float scaleY, float duration, const Color& color);

        /// スプライトシートのグリッド分割数を設定する
        void SetGrid(int c, int r) { gridCols = c; gridRows = r; }

        // ===== 共通 =====

        /// 全エフェクトを停止する
        void ClearAll();

        /// 毎フレーム更新処理
        void Update(const sf::command::ICommand& cmd);

    private:
        ID3D11DeviceContext* m_context = nullptr;  // 描画コンテキスト（Effekseer描画時に使用）

        // --- スプライトエフェクト管理 ---
        struct EffectInstance {
            sf::SafePtr<sf::ui::Image> image;  // スプライト画像
            bool active = false;                // 再生中フラグ
            float timer = 0.0f;                 // 経過時間
            float duration = 0.0f;              // 再生時間
        };
        std::vector<EffectInstance> spritePool;  // スプライトのオブジェクトプール

        int gridCols = 8;  // スプライトシートの列数
        int gridRows = 8;  // スプライトシートの行数

        // --- Effekseer管理 ---
        EffekseerRenderer::RendererRef efkRenderer;                    // Effekseerレンダラー
        Effekseer::ManagerRef efkManager;                              // Effekseerマネージャー
        std::map<std::string, Effekseer::EffectRef> efkResourceMap;   // エフェクトリソースマップ

        sf::command::Command<> updateCommand;  // 更新コマンド
    };
}