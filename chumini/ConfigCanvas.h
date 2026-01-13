#pragma once
#include "App.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"
#include <vector>
#include <memory>
#include <functional>

namespace app
{
	namespace test
	{
		class ConfigCanvas : public sf::ui::Canvas
		{
		public:
			void Begin() override;
			void Update(const sf::command::ICommand&);
            // End removed as base class does not have it.

		private:
			sf::command::Command<> updateCommand;
			sf::SafePtr<SceneChangeComponent> sceneChanger;

			// Global UI
			sf::ui::TextImage* backButton = nullptr;
			bool isBackHovered = false;

			// Scroll System
			float scrollY = 0.0f;
			float targetScrollY = 0.0f;
			float totalContentHeight = 0.0f;
			static constexpr float LIST_START_Y = 250.0f;
			static constexpr float ITEM_SPACING = 120.0f;

			// Config Item
			struct ConfigItem {
				sf::ui::TextImage* label = nullptr;
				sf::ui::TextImage* valueLabel = nullptr;
				sf::ui::TextImage* leftButton = nullptr;
				sf::ui::TextImage* rightButton = nullptr;

				float localY = 0.0f;
				float height = 100.0f;

				std::function<void()> onLeft;
				std::function<void()> onRight;
				std::function<void()> onUpdateView;

				bool IsHovered(sf::ui::TextImage* btn, const Vector2& mousePos);
				void Update(float currentY, const Vector2& mousePos, bool isClicked);
				void SetVisible(bool visible);
			};
			std::vector<std::shared_ptr<ConfigItem>> items;

			// State
			enum class State {
				Normal,
				WaitingForKey
			};
			State state = State::Normal;
			int waitingLaneIndex = -1;

			void HandleInput(const sf::command::ICommand& command);
			void UpdateScroll();
			void RebuildLayout();

			// Factories
			void AddHeader(const wchar_t* text);
			void AddBoolItem(const wchar_t* label, bool* targetBool);
			void AddFloatItem(const wchar_t* label, float* targetFloat, float step, float min, float max, const wchar_t* format = L"%.1f");
			void AddKeyItem(int laneIndex, const wchar_t* label);

			void OnButtonPressed();
			void DetectKeyInput();

			Vector2 GetMousePosition();
			bool IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button);

			static constexpr float screenWidth = 1920.0f;
			static constexpr float screenHeight = 1080.0f;
		};
	}
}
