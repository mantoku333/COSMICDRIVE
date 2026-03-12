#pragma once
#include "App.h"
#include "TextImage.h"
#include "sf/AppModel.h"
#include "SongInfo.h"
#include "Live2DManager.h"
#include "Image.h"
#include "Texture.h"

namespace app
{
	namespace test
	{
		class TitleScene;
		enum class TitleButton;

		/// タイトル画面のUI表示・入力検知・アニメーションを管理するキャンバス
		/// MVPパターンの View に相当し、Presenterは TitleScene
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);
			void Draw() override;
			virtual ~TitleCanvas();

			/// Presenter（TitleScene）を設定する
			void SetPresenter(TitleScene* scene) { presenter = scene; }

		private:
			// --- Presenter参照 ---
			TitleScene* presenter = nullptr;

			// --- Live2Dモデル ---
			AppModel* _hiyoriModel = nullptr;

			// --- コマンド ---
			sf::command::Command<> updateCommand;

			// --- UIボタン ---
			sf::ui::TextImage* playButton = nullptr;    // PLAYボタン
			sf::ui::TextImage* exitButton = nullptr;    // EXITボタン
			sf::ui::TextImage* configButton = nullptr;  // CONFIGボタン
			
			// --- 背景 ---
			sf::ui::Image* whiteBacking = nullptr; 
			sf::Texture backTexture; 

			// --- ジャケットスクロール ---
			struct ScrollingJacket {
				sf::ui::Image* uiImage;
				float posX; 
			};

			std::vector<sf::Texture> jacketTextures;              // ジャケットテクスチャ群
			std::vector<ScrollingJacket> scrollingJacketsTop;     // 上段スクロールジャケット
			std::vector<ScrollingJacket> scrollingJacketsBottom;  // 下段スクロールジャケット

			float totalWidth = 0.0f;                   // スクロール全体幅
			const float jacketSpeedTop = 120.0f;       // 上段スクロール速度
			const float jacketSpeedBottom = -150.0f;   // 下段スクロール速度（逆方向）
			const float jacketScale = 2.5f; 		   // ジャケット表示スケール
			const float jacketInterval = 270.0f;       // ジャケット間隔（px）

			// --- 定数 ---
			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
			float animationTimer = 0.0f;  // アニメーション用タイマー

			// --- 内部処理メソッド ---
			void InitializeJacketFlow();                              // ジャケットスクロールの初期化
			void DetectInputAndNotify(const sf::command::ICommand& command); // 入力検知とPresenterへの通知
			void UpdateJacketScrolling(float dt);                     // ジャケットスクロールの更新
			void UpdateButtonHighlight(TitleButton selected);         // ボタンハイライトの更新
			Vector2 GetMousePosition();                               // マウス座標を画面座標に変換
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button); // ボタンのホバー判定
		};
	}
}
