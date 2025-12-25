#pragma once
#include "App.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"

namespace app::test {
	class ResultCanvas : public sf::ui::Canvas {
	public:
		void Begin() override;
		void Update(const sf::command::ICommand&);

	private:
		sf::Texture textureBackground;

		sf::ui::TextImage* resultLabel = nullptr;
		sf::ui::TextImage* scoreText = nullptr;

		// ランク表示用
		sf::ui::TextImage* rankText = nullptr;       // メイン（手前）
		sf::ui::TextImage* rankOutline[4] = { nullptr }; // ★追加: ふち用（奥）

		sf::ui::TextImage* comboText = nullptr;
		sf::ui::TextImage* judgeDetailText = nullptr;

		sf::ui::TextImage* guideText = nullptr;
		float timer = 0.0f;

		sf::command::Command<> updateCommand;
		sf::SafePtr<sf::IScene> nextScene;
		sf::SafePtr<SceneChangeComponent> sceneChanger;
	};
}