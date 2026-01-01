#pragma once
#include "App.h"
#include "BGMComponent.h"
#include "Live2DComponent.h" // Include this
//#include "GeometryCube.h" // Reverted

namespace app {
    namespace test {

        class TitleScene : public sf::Scene<TitleScene> {
        public:
            void Init() override;
            void Update(const sf::command::ICommand& command);
            void Draw() override;
            void DrawOverlay() override;
            void OnGUI() override;


        private:
            sf::ref::Ref<sf::Actor> uiManagerActor;
            sf::SafePtr<app::test::BGMComponent> bgmPlayer;
            sf::SafePtr<Live2DComponent> l2dComp; // Add pointer to Live2DComponent
            // sf::geometry::GeometryCube g_cube; // Reverted

            sf::command::Command<> updateCommand;

            int selectedButton;
            bool isButtonPressed;
        };

    }
}