#pragma once
#include "Component.h"
#include "sf/AppModel.h"
#include <string>
#include <d3d11.h>
#include <mutex>

namespace app {
    namespace test {

        class Live2DComponent : public sf::Component {
        public:
            void Begin() override; // Init -> Begin
            void Update();         // Removed override
            void Draw();           // Removed override
            virtual ~Live2DComponent();

            // 処理本体
            bool IsDestroyed() const { return m_isDestroyed; }

            void LoadModel(const std::string& dir, const std::string& fileName);
            void PlayMotion(const char* group, int no, int priority); // trigger animation
            void StartGlitchMotion(const char* group, int no); // 笘・ew Method
            
            void SetDragging(float x, float y); // Look At

            AppModel* GetAppModel() const { return _model; }

        private:
            AppModel* _model = nullptr;
            bool m_isDestroyed = false;

            // 処理本体
            ID3D11BlendState* m_cachedBlendState = nullptr;
            ID3D11RasterizerState* m_cachedRasterState = nullptr;
            ID3D11DepthStencilState* m_cachedDepthState = nullptr;
            bool m_statesInitialized = false;

            // 処理本体
            mutable std::mutex m_drawMutex;

            void InitCachedStates(ID3D11Device* device);
            void ReleaseCachedStates();
        };

    }
}
