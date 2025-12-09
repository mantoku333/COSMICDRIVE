#pragma once
#include "App.h"
#include "NoteData.h"
#include "NoteManager.h"
#include "EffectManager.h"
#include <vector>
#include <memory>

// 前方宣言
namespace app::test {
	class NoteManager;
}

namespace app
{
	namespace test
	{
		class TestCanvas : public sf::ui::Canvas
		{
		public:
			// EffectManagerがAddUIを使えるように公開
			using sf::ui::Canvas::AddUI;

			void Begin() override;
			void Update(const sf::command::ICommand&);

			// NoteManagerから呼ばれる窓口
			void SpawnHitEffect(float x, float y, float scale, float duration, const Color& color);

			// ... (その他のUI関数) ...
			void SetJudgeImage(JudgeResult result);
			void DrawNumber(int number, float x, float y, float digitWidth, float digitHeight, float sheetWidth, float sheetHeight, sf::Texture* numberTexture);
			void InitializeTimerDisplay();
			void InitializeComboDisplay();
			void InitializeJudgeCountDisplay();
			void UpdateTimerDisplay(const std::string& str);
			void UpdateComboDisplay(int combo);
			void UpdateJudgeCountDisplay();
			void UpdateJacketImage();

			NoteManager* noteManager = nullptr;

			void DestroyEffect(sf::ui::Image* effect);

		private:
		

			// ... (既存のテクスチャ) ...
			sf::Texture textureDefaultJacket;
			sf::ui::Image* Jacket;
			sf::Texture textureJacket;
			sf::Texture textureNone;
			sf::Texture textureNumber;
			sf::Texture textureCombo;
			sf::Texture texturePanel;

			// ★追加: エフェクト用テクスチャの実体はここ（Canvas）が持つ
			sf::Texture textureHitEffect;

			sf::ui::Image* judgePanel = nullptr;

			sf::SafePtr<EffectManager> effectManager;

			// ... (以下既存のメンバ変数) ...
			std::vector<sf::ui::Image*> judgeCountDigitsPerfect;
			std::vector<sf::ui::Image*> judgeCountDigitsGreat;
			std::vector<sf::ui::Image*> judgeCountDigitsGood;
			std::vector<sf::ui::Image*> judgeCountDigitsMiss;

			sf::Texture texturePerfect, textureGreat, textureGood, textureMiss;
			sf::ui::Image* judgeImage = nullptr;

			std::vector<sf::ui::Image*> timerDigits;
			static const int MAX_TIMER_DIGITS = 8;

			std::vector<sf::ui::Image*> comboDigits;
			static constexpr int MAX_COMBO_DIGITS = 4;

			sf::command::Command<> updateCommand;

			float digitWidth = 100.0f;
			float digitHeight = 100.0f;
			float sheetWidth = 1000.0f;
			float sheetHeight = 100.0f;

			float countUpTimer = 0.0f;
		};
	}
}