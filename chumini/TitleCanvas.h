#pragma once
#include "App.h"
#include "TextImage.h" // 追加: TextImageを使用するために必要

namespace app
{
	namespace test
	{
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);

			int GetSelectedButton() const;
			void SetSelectedButton(int buttonIndex);

		private:
			// --- UIオブジェクト（ImageからTextImageに変更） ---
			sf::ui::TextImage* titleLogo = nullptr;
			sf::ui::TextImage* playButton = nullptr;
			sf::ui::TextImage* editButton = nullptr;

			// --- 状態管理 ---
			int selectedButton = 0; // 0: エディット, 1: プレイ

			// --- メソッド ---
			void HandleInput(const sf::command::ICommand& command);
			void UpdateButtonSelection();
			void OnButtonPressed();

			void ShowSongSelectScene();
			void ShowEditScene();

			// マウス座標取得用
			Vector2 GetMousePosition();

			// 当たり判定 (引数を TextImage* に変更)
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);

			// 画面サイズ
			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;

			sf::command::Command<> updateCommand;

			// シーン
			sf::SafePtr<sf::IScene> scene;
			sf::SafePtr<sf::IScene> sceneEdit;
		};
	}
}