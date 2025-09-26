#pragma once
#include "App.h"

namespace app
{
	namespace test
	{
		class TitleCanvas :public sf::ui::Canvas
		{
		public:
			void Begin()override;
			void Update(const sf::command::ICommand&);

            int GetSelectedButton() const;
            void SetSelectedButton(int buttonIndex);

        private:
            // テクスチャ
            sf::Texture textureTitleLogo;
            sf::Texture textureEditButton;
            sf::Texture texturePlayButton;
            sf::Texture textureEditButtonSelected;
            sf::Texture texturePlayButtonSelected;

            // UIオブジェクト
            sf::ui::Image* titleLogo;
            sf::ui::Image* editButton;
            sf::ui::Image* playButton;

            // 状態管理
            int selectedButton; // 0: エディット, 1: プレイ

            // メソッド
            void HandleInput(const sf::command::ICommand& command);
            void UpdateButtonSelection();
            void OnButtonPressed();

			void ShowSongSelectScene();
            void ShowEditScene();

            enum class MouseButton {
                Left,
                Right,
                Middle
            };

            bool IsMouseButtonPressed(MouseButton button);

            // 画面サイズ（実際の値に設定してください）
            static constexpr float screenWidth = 1920.0f;
            static constexpr float screenHeight = 1080.0f;

            // マウス操作用のメソッド
            bool IsButtonHovered(const Vector2& mousePos, sf::ui::Image* button);
            Vector2 GetMousePosition();

			sf::command::Command<> updateCommand;
            // シーン
            sf::SafePtr<sf::IScene> scene;
            sf::SafePtr<sf::IScene> sceneEdit;


		};
	}
}