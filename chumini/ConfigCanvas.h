#pragma once
#include "App.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"

namespace app
{
	namespace test
	{
		class ConfigCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);

		private:
			sf::command::Command<> updateCommand;
			sf::SafePtr<SceneChangeComponent> sceneChanger;

			// UI Elements
			sf::ui::TextImage* backButton = nullptr;
			sf::ui::TextImage* laneButtons[4] = { nullptr };
			sf::ui::TextImage* laneLabels[4] = { nullptr }; 

            // HiSpeed UI
            sf::ui::TextImage* speedUpButton = nullptr;
            sf::ui::TextImage* speedDownButton = nullptr;
            sf::ui::TextImage* speedLabel = nullptr;
            sf::ui::TextImage* speedValueLabel = nullptr; // 値表示用 

            // Controller Mode UI
            sf::ui::TextImage* controllerModeLabel = nullptr;
            sf::ui::TextImage* controllerModeButton = nullptr;

            // Tap Sound UI
            sf::ui::TextImage* tapSoundLabel = nullptr;
            sf::ui::TextImage* tapSoundButton = nullptr;

			// Offset
			sf::ui::TextImage* offsetLabel = nullptr;
			sf::ui::TextImage* offsetValueLabel = nullptr;
			sf::ui::TextImage* offsetDownButton = nullptr;
			sf::ui::TextImage* offsetUpButton = nullptr;

			// FAST/SLOW
			sf::ui::TextImage* fastSlowLabel = nullptr;
			sf::ui::TextImage* fastSlowButton = nullptr;

			// State
			bool isBackHovered = false;
			bool isLaneHovered[4] = { false };

			enum class State {
				Normal,
				WaitingForKey
			};
			State state = State::Normal;
			int waitingLaneIndex = -1;

			void HandleInput(const sf::command::ICommand& command);
			void OnButtonPressed();
			void OnLaneButtonPressed(int laneIndex);
			void UpdateButtonText(); 
            void UpdateSpeedText(); // Check if this was missing 
            void UpdateControllerModeText(); 
            void UpdateTapSoundText(); 
			void UpdateOffsetText(); 
			void UpdateFastSlowText(); 
			void DetectKeyInput();

			Vector2 GetMousePosition();
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);

			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
		};
	}
}
