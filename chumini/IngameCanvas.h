#pragma once
#include "App.h"
#include "NoteData.h"
#include "NoteManager.h"
#include "EffectManager.h"
#include <vector>
#include <memory>
// テキスト描画用
#include "TextImage.h"

// 前方宣言
namespace app::test {
	class NoteManager;
}

namespace app
{
	namespace test
	{
		class IngameCanvas : public sf::ui::Canvas
		{
		public:
			// EffectManagerがAddUIを使えるように公開
			using sf::ui::Canvas::AddUI;

			void Begin() override;
			void Update(const sf::command::ICommand&);

			// NoteManagerから呼ばれる窓口
			void SpawnHitEffect(float x, float y, float scale, float duration, const Color& color);

			void UpdateTimerDisplay(float time);
			void UpdateComboDisplay(int combo);
			void UpdateJudgeCountDisplay();

			// ★ここを SetJudgeImage に戻しました（中身はテキスト更新です）
			void SetJudgeImage(JudgeResult result);

			void UpdateJacketImage();

			NoteManager* noteManager = nullptr;
			void DestroyEffect(sf::ui::Image* effect);

			// 3.2.1.START の表示更新
			void UpdateCountdownDisplay(float time, bool isStart);

		private:
			// ... (既存のテクスチャ) ...
			sf::Texture textureDefaultJacket;
			sf::ui::Image* Jacket;
			sf::Texture textureJacket;
			sf::Texture texturePanel;

			// エフェクト用
			sf::Texture textureHitEffect;

			sf::ui::Image* judgePanel = nullptr;
			sf::SafePtr<EffectManager> effectManager;

			// ---------------------------------------------------------
			// テキストUI
			// ---------------------------------------------------------
			sf::ui::TextImage* timerText = nullptr;
			sf::ui::TextImage* comboText = nullptr;
			sf::ui::TextImage* judgeStatsText = nullptr;

			// 判定結果表示用テキスト
			sf::ui::TextImage* judgeResultText = nullptr;



			sf::command::Command<> updateCommand;

			float countUpTimer = 0.0f;

			// コンボ演出用
			int lastCombo = -1;
			float comboScaleTimer = 0.0f;

			// 判定表示用
			JudgeResult lastJudgeResult = JudgeResult::None;

			// カウントダウン表示制御用
			int lastCountdownVal = -1;
			bool isCountdownStartShown = false;
			
			// カウントダウン中かどうか（タイマー更新抑制用）
			bool isCountdownActive = false;
		};
	}
}
