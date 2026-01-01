#pragma once
#include "Component.h"
#include "AppModel.h"
#include <string>

namespace app {
    namespace test {

        class Live2DComponent : public sf::Component {
        public:
            void Begin() override; // Init -> Begin
            void Update();         // Removed override
            void Draw();           // Removed override
            virtual ~Live2DComponent();

            void LoadModel(const std::string& dir, const std::string& fileName);
            void PlayMotion(const char* group, int no, int priority); // trigger animation
            void StartGlitchMotion(const char* group, int no); // ★New Method
            
            void SetDragging(float x, float y); // Look At

            AppModel* GetAppModel() const { return _model; }

        private:
            AppModel* _model = nullptr;
        };

    }
}
