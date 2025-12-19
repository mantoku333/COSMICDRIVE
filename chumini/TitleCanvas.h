#pragma once
#include "App.h"
#include "TextImage.h" // 追加: TextImageを使用するために必要

#include "SongInfo.h"

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

			float totalWidth = 0.0f; // 追加：ジャケット列の端から端までの長さ


			// --- 追加: ジャケットループ背景用 ---
			struct ScrollingJacket {
				sf::ui::Image* uiImage;
				float posX; // 初期配置のズレ
			};
			std::vector<sf::Texture> jacketTextures;

			std::vector<ScrollingJacket> scrollingJacketsTop; // 名前を変更
			std::vector<ScrollingJacket> scrollingJacketsBottom;

			const float jacketSpeedTop = 120.0f;     // 上段の速度（正の値）
			const float jacketSpeedBottom = -150.0f; // 下段の速度（負の値で少し速くすると遠近感が出る）

			const float jacketScale = 2.5f;
			const float jacketInterval = 270.0f;

			void InitializeJacketFlow(); // ジャケット読み込みと生成

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

			float animationTimer = 0.0f;


			sf::command::Command<> updateCommand;

			// シーン
			sf::SafePtr<sf::IScene> scene;
			sf::SafePtr<sf::IScene> sceneEdit;
		};
	}
}