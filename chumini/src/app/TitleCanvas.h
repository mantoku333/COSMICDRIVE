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
		class TitleScene;  // 前方宣言
		enum class TitleButton;  // 前方宣言

		// ============================================================
		// 補助処理
		// 
		// 蠖ｹ蜑ｲ:
		// 処理本体
		// - 謠冗判蜃ｦ逅・
		// 処理本体
		// 処理本体
		// ============================================================
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);
			void Draw() override;
			virtual ~TitleCanvas();

			// --------------------------------------------
			// 処理本体
			// --------------------------------------------
			
			// 処理本体
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
			// 処理本体
			// --------------------------------------------
			sf::command::Command<> updateCommand;

			// --------------------------------------------
			// UI隕∫ｴ
			// --------------------------------------------
			sf::ui::TextImage* playButton = nullptr;
			sf::ui::TextImage* exitButton = nullptr;
			sf::ui::TextImage* configButton = nullptr;
			
			// 閭梧勹
			sf::ui::Image* whiteBacking = nullptr; 
			sf::Texture backTexture; 

			// ジャケット表示を更新
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
			// ジャケット表示を更新
			// --------------------------------------------
			
			// ジャケット表示を更新
			void InitializeJacketFlow();
			
			// 処理本体
			void DetectInputAndNotify(const sf::command::ICommand& command);
			
			// ジャケット表示を更新
			void UpdateJacketScrolling(float dt);
			
			// / ボタンハイライト表示更新
			void UpdateButtonHighlight(TitleButton selected);

			// 処理本体
			Vector2 GetMousePosition();
			
			// 処理本体
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);
		};
	}
}
