#pragma once
#include "Component.h"
#include "sf/AppModel.h"
#include <string>
#include <d3d11.h>
#include <mutex>

namespace app {
    namespace test {

        /// Live2Dモデルの読み込み・更新・描画を管理するコンポーネント
        class Live2DComponent : public sf::Component {
        public:
            void Begin() override;
            void Update();
            void Draw();
            virtual ~Live2DComponent();

            /// 破棄済みかどうかを返す
            bool IsDestroyed() const { return m_isDestroyed; }

            /// モデルを読み込む
            void LoadModel(const std::string& dir, const std::string& fileName);

            /// モーションを再生する
            void PlayMotion(const char* group, int no, int priority);

            /// グリッチモーションを再生する
            void StartGlitchMotion(const char* group, int no);
            
            /// 視線追従の目標座標を設定する
            void SetDragging(float x, float y);

            /// 内部のAppModelを取得する
            AppModel* GetAppModel() const { return _model; }

        private:
            AppModel* _model = nullptr;    // Live2Dモデル本体
            bool m_isDestroyed = false;    // 破棄済みフラグ

            // --- キャッシュ済みパイプラインステート ---
            ID3D11BlendState* m_cachedBlendState = nullptr;
            ID3D11RasterizerState* m_cachedRasterState = nullptr;
            ID3D11DepthStencilState* m_cachedDepthState = nullptr;
            bool m_statesInitialized = false;

            // --- スレッドセーフ用ミューテックス ---
            mutable std::mutex m_drawMutex;

            /// パイプラインステートを初期化する（初回のみ）
            void InitCachedStates(ID3D11Device* device);

            /// キャッシュ済みパイプラインステートを解放する
            void ReleaseCachedStates();
        };

    }
}
