#pragma once
#include "Component.h"
#include "sf/AppModel.h"
#include <string>
#include <d3d11.h>
#include <mutex>  // 繧ｹ繝ｬ繝・ラ繧ｻ繝ｼ繝慕畑

namespace app {
    namespace test {

        class Live2DComponent : public sf::Component {
        public:
            void Begin() override; // Init -> Begin
            void Update();         // Removed override
            void Draw();           // Removed override
            virtual ~Live2DComponent();

            // 繧ｷ繝ｼ繝ｳ驕ｷ遘ｻ譎ゅ・螳牙・諤ｧ繝√ぉ繝・け逕ｨ
            bool IsDestroyed() const { return m_isDestroyed; }

            void LoadModel(const std::string& dir, const std::string& fileName);
            void PlayMotion(const char* group, int no, int priority); // trigger animation
            void StartGlitchMotion(const char* group, int no); // 笘・ew Method
            
            void SetDragging(float x, float y); // Look At

            AppModel* GetAppModel() const { return _model; }

        private:
            AppModel* _model = nullptr;
            bool m_isDestroyed = false; // 遐ｴ譽・ヵ繝ｩ繧ｰ・医す繝ｼ繝ｳ驕ｷ遘ｻ繧ｯ繝ｩ繝・す繝･髦ｲ豁｢・・

            // 笘・く繝｣繝・す繝･縺輔ｌ縺櫂irectX繧ｹ繝・・繝茨ｼ域ｯ弱ヵ繝ｬ繝ｼ繝菴懈・縺ｮ繧ｪ繝ｼ繝舌・繝倥ャ繝峨ｒ蜑頑ｸ幢ｼ・
            ID3D11BlendState* m_cachedBlendState = nullptr;
            ID3D11RasterizerState* m_cachedRasterState = nullptr;
            ID3D11DepthStencilState* m_cachedDepthState = nullptr;
            bool m_statesInitialized = false;

            // 笘・せ繝ｬ繝・ラ繧ｻ繝ｼ繝慕畑: Draw荳ｭ縺ｫ繝・せ繝医Λ繧ｯ繧ｿ縺瑚ｵｰ繧峨↑縺・ｈ縺・↓繝ｭ繝・け
            mutable std::mutex m_drawMutex;

            void InitCachedStates(ID3D11Device* device);
            void ReleaseCachedStates();
        };

    }
}
