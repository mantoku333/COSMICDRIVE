#pragma once
#include "App.h"
#include "TextImage.h"
#include "AppModel.h"
#include "SceneChangeComponent.h"
#include "SongInfo.h"
#include "Live2DManager.h"

// Ensure these are included for Image and Texture members
#include "Image.h"
#include "Texture.h"

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
			sf::SafePtr<SceneChangeComponent> sceneChanger;

			// UI Buttons
			sf::ui::TextImage* playButton = nullptr;
			sf::ui::TextImage* exitButton = nullptr;
			
			// White Backing (For Black Background behind Live2D)
			// Changed to Image* to support texture
			sf::ui::Image* whiteBacking = nullptr; 
			sf::Texture backTexture; 

			// --- State ---
			int selectedButton = 0; // 0: EXIT, 1: PLAY

			// Background Jacket Flow
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

			// --- Methods ---

			void InitializeJacketFlow();
			void HandleInput(const sf::command::ICommand& command);
			void UpdateButtonSelection();
			void OnButtonPressed();
			void ShowSongSelectScene();

			Vector2 GetMousePosition();
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);

			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;

			float animationTimer = 0.0f;
		};
	}
}