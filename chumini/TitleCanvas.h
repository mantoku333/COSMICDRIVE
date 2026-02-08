#pragma once
#include "App.h"
#include "TextImage.h"
#include "AppModel.h"
#include "SongInfo.h"
#include "Live2DManager.h"
#include "Image.h"
#include "Texture.h"

namespace app
{
	namespace test
	{
		class TitleScene;  // 前方宣言
		enum class TitleButton;  // 前方宣言

		// ============================================================
		// TitleCanvas - タイトル画面のView（MVP）
		// 
		// 役割:
		// - UI要素の生成・管理
		// - 描画処理
		// - 入力検出→Presenterに委譲
		// - Presenterの状態を参照して表示更新
		// ============================================================
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);
			void Draw() override;
			virtual ~TitleCanvas();

			// --------------------------------------------
			// Presenter設定（Sceneから呼ばれる）
			// --------------------------------------------
			
			/// Presenterへの参照を設定
			void SetPresenter(TitleScene* scene) { presenter = scene; }

		private:
			// --------------------------------------------
			// Presenter参照
			// --------------------------------------------
			TitleScene* presenter = nullptr;

			// --------------------------------------------
			// Live2D
			// --------------------------------------------
			AppModel* _hiyoriModel = nullptr;

			// --------------------------------------------
			// 更新コマンド
			// --------------------------------------------
			sf::command::Command<> updateCommand;

			// --------------------------------------------
			// UI要素
			// --------------------------------------------
			sf::ui::TextImage* playButton = nullptr;
			sf::ui::TextImage* exitButton = nullptr;
			sf::ui::TextImage* configButton = nullptr;
			
			// 背景
			sf::ui::Image* whiteBacking = nullptr; 
			sf::Texture backTexture; 

			// ジャケットスクロール
			struct ScrollingJacket {
				sf::ui::Image* uiImage;
				float posX; 
			};

			std::vector<sf::Texture> jacketTextures;
			std::vector<ScrollingJacket> scrollingJacketsTop;
			std::vector<ScrollingJacket> scrollingJacketsBottom;

			float totalWidth = 0.0f;                 
			const float jacketSpeedTop = 120.0f;     
			const float jacketSpeedBottom = -150.0f; 
			const float jacketScale = 2.5f; 		 
			const float jacketInterval = 270.0f;      

			// --------------------------------------------
			// 定数
			// --------------------------------------------
			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
			float animationTimer = 0.0f;

			// --------------------------------------------
			// 内部メソッド
			// --------------------------------------------
			
			/// ジャケットフロー初期化
			void InitializeJacketFlow();
			
			/// 入力検出してPresenterに通知
			void DetectInputAndNotify(const sf::command::ICommand& command);
			
			/// ジャケットスクロールアニメーション更新
			void UpdateJacketScrolling(float dt);
			
			/// ボタンハイライト表示更新
			void UpdateButtonHighlight(TitleButton selected);

			/// マウス位置取得
			Vector2 GetMousePosition();
			
			/// ボタンホバー判定
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);
		};
	}
}