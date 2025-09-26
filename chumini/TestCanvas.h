#pragma once
#include "App.h"
#include "NoteData.h"
#include"NoteManager.h"

// 前方宣言
namespace app::test {
	class NoteManager;
}


namespace app
{
	namespace test
	{
		class TestCanvas :public sf::ui::Canvas
		{
		public:
			void Begin()override;
			void Update(const sf::command::ICommand&);

			void SetJudgeImage(JudgeResult result); // 追加

			void DrawNumber(int number, float x, float y, float digitWidth, float digitHeight, float sheetWidth, float sheetHeight, sf::Texture* numberTexture);
			//void DrawNumberString(const std::string& str, float x, float y, float digitWidth, float digitHeight, float sheetWidth, float sheetHeight, sf::Texture* numberTexture); // 追加

			void InitializeTimerDisplay();
			void InitializeComboDisplay();

			void UpdateTimerDisplay(const std::string& str);
			void UpdateComboDisplay(int combo);

			// NoteManager参照
			NoteManager* noteManager = nullptr;

			void UpdateJacketImage();  // 追加：ジャケット更新メソッド

		private:

			sf::Texture textureDefaultJacket;
			sf::ui::Image* Jacket;

			//テクスチャ
			sf::Texture textureJacket;
			sf::Texture textureOk;
			sf::Texture textureNumber; // 数字のスプライトシート

			sf::Texture textureCombo;

			sf::Texture texturePerfect, textureGreat, textureGood, textureMiss ;
			sf::ui::Image* judgeImage = nullptr; // 追加

			std::vector<sf::ui::Image*> timerDigits;
			static const int MAX_TIMER_DIGITS = 8;

			std::vector<sf::ui::Image*> comboDigits;
			static constexpr int MAX_COMBO_DIGITS = 4;
			

			sf::command::Command<> updateCommand;

			// 数字画像のパラメータ
			float digitWidth = 100.0f;
			float digitHeight = 100.0f;
			float sheetWidth = 1000.0f;
			float sheetHeight = 100.0f;

			float countUpTimer = 0.0f;


		};
	}
}