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
		class TitleScene;  // 蜑肴婿螳｣險
		enum class TitleButton;  // 蜑肴婿螳｣險

		// ============================================================
		// TitleCanvas - 繧ｿ繧､繝医Ν逕ｻ髱｢縺ｮView・・VP・・
		// 
		// 蠖ｹ蜑ｲ:
		// - UI隕∫ｴ縺ｮ逕滓・繝ｻ邂｡逅・
		// - 謠冗判蜃ｦ逅・
		// - 蜈･蜉帶､懷・竊単resenter縺ｫ蟋碑ｭｲ
		// - Presenter縺ｮ迥ｶ諷九ｒ蜿ら・縺励※陦ｨ遉ｺ譖ｴ譁ｰ
		// ============================================================
		class TitleCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);
			void Draw() override;
			virtual ~TitleCanvas();

			// --------------------------------------------
			// Presenter險ｭ螳夲ｼ・cene縺九ｉ蜻ｼ縺ｰ繧後ｋ・・
			// --------------------------------------------
			
			/// Presenter縺ｸ縺ｮ蜿ら・繧定ｨｭ螳・
			void SetPresenter(TitleScene* scene) { presenter = scene; }

		private:
			// --------------------------------------------
			// Presenter蜿ら・
			// --------------------------------------------
			TitleScene* presenter = nullptr;

			// --------------------------------------------
			// Live2D
			// --------------------------------------------
			AppModel* _hiyoriModel = nullptr;

			// --------------------------------------------
			// 譖ｴ譁ｰ繧ｳ繝槭Φ繝・
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

			// 繧ｸ繝｣繧ｱ繝・ヨ繧ｹ繧ｯ繝ｭ繝ｼ繝ｫ
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
			// 螳壽焚
			// --------------------------------------------
			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
			float animationTimer = 0.0f;

			// --------------------------------------------
			// 蜀・Κ繝｡繧ｽ繝・ラ
			// --------------------------------------------
			
			/// 繧ｸ繝｣繧ｱ繝・ヨ繝輔Ο繝ｼ蛻晄悄蛹・
			void InitializeJacketFlow();
			
			/// 蜈･蜉帶､懷・縺励※Presenter縺ｫ騾夂衍
			void DetectInputAndNotify(const sf::command::ICommand& command);
			
			/// 繧ｸ繝｣繧ｱ繝・ヨ繧ｹ繧ｯ繝ｭ繝ｼ繝ｫ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ譖ｴ譁ｰ
			void UpdateJacketScrolling(float dt);
			
			/// 繝懊ち繝ｳ繝上う繝ｩ繧､繝郁｡ｨ遉ｺ譖ｴ譁ｰ
			void UpdateButtonHighlight(TitleButton selected);

			/// 繝槭え繧ｹ菴咲ｽｮ蜿門ｾ・
			Vector2 GetMousePosition();
			
			/// 繝懊ち繝ｳ繝帙ヰ繝ｼ蛻､螳・
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);
		};
	}
}
