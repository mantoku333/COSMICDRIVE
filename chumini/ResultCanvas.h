#pragma once
#include "App.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"

namespace app::test {
	class ResultScene; // MVP: 前方宣言

	class ResultCanvas : public sf::ui::Canvas {
	public:
		void Begin() override;
		void Update(const sf::command::ICommand&);

		// MVP: Presenter設定
		void SetPresenter(ResultScene* scene) { presenter = scene; }

	private:
		sf::command::Command<> updateCommand;
		ResultScene* presenter = nullptr; // MVP: Presenter参照

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

		void UpdateRankDisplay(int score);
	};
}