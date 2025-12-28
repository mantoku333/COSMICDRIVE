#pragma once
#include "App.h"
#include "TextImage.h" // 追加: TextImageを使用するために必要
#include "SceneChangeComponent.h"
#include "SongInfo.h"

#include "AppModel.h"
#include "Live2DManager.h"

namespace app
{
	namespace test
	{
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);

			void Draw() override;
			virtual ~TitleCanvas();
		private:

			AppModel* _hiyoriModel = nullptr;

			sf::command::Command<> updateCommand;
			sf::SafePtr<SceneChangeComponent> sceneChanger;// シーン遷移用

			// ボタンUI
			sf::ui::TextImage* playButton = nullptr;
			sf::ui::TextImage* exitButton = nullptr;

			// --- 状態管理 ---
			int selectedButton = 0; // 0: EXIT, 1: PLAY

			// ジャケットループ背景用
			struct ScrollingJacket {
				sf::ui::Image* uiImage;
				float posX; // 初期配置のズレ
			};

			std::vector<sf::Texture> jacketTextures;
			std::vector<ScrollingJacket> scrollingJacketsTop;
			std::vector<ScrollingJacket> scrollingJacketsBottom;

			float totalWidth = 0.0f;                 // ジャケット列の端から端までの長さ
			const float jacketSpeedTop = 120.0f;     // 上段の速度（正の値）
			const float jacketSpeedBottom = -150.0f; // 下段の速度（負の値で少し速くすると遠近感が出る）
			const float jacketScale = 2.5f; 		 // ジャケットの拡大率
			const float jacketInterval = 270.0f;     // ジャケット同士の間隔 

			// --- メソッド ---

			

			void InitializeJacketFlow(); // ジャケット周り初期化
			void HandleInput(const sf::command::ICommand& command); // 入力処理
			void UpdateButtonSelection(); // ボタンUIのUpdate
			void OnButtonPressed(); // ボタンが押されたときの処理
			void ShowSongSelectScene();

			Vector2 GetMousePosition();// マウス座標取得用
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);// 当たり判定 (引数を TextImage* に変更)

			// 画面サイズ
			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;

			float animationTimer = 0.0f;


		};
	}
}