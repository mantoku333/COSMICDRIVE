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
		sf::command::Command<> updateCommand;
		sf::SafePtr<SceneChangeComponent> sceneChanger; //シーン遷移用

		sf::ui::TextImage* resultLabel = nullptr; // "RESULT"
		sf::ui::TextImage* scoreText = nullptr; // スコア
		sf::ui::TextImage* rankText = nullptr;       // ランク（本体）
		sf::ui::TextImage* rankOutline[4] = { nullptr }; // ランク（縁取り）
		sf::ui::TextImage* comboText = nullptr; //最大コンボ
                
                // 判定内訳（個別）
                sf::ui::TextImage* perfectText = nullptr;
                sf::ui::TextImage* greatText = nullptr;
                sf::ui::TextImage* goodText = nullptr;
                sf::ui::TextImage* missText = nullptr;
                sf::ui::TextImage* fastText = nullptr;
                sf::ui::TextImage* slowText = nullptr;

                sf::ui::TextImage* guideText = nullptr; // 操作ガイド

		float timer = 0.0f; //アニメーション用
		
		int targetScore = 0;
		float displayScore = 0.0f;
		bool isScoreAnimationFinished = false;

		
	};
}