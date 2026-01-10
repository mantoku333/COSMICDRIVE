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

			// State
			bool isBackHovered = false;

			void HandleInput(const sf::command::ICommand& command);
			void OnButtonPressed();
			Vector2 GetMousePosition();
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);

			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
		};
	}
}
