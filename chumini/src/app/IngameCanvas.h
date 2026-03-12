#pragma once
#include "App.h"
#include "NoteData.h"
#include "NoteManager.h"
#include "EffectManager.h"
#include <vector>
#include <memory>
// テキスト描画用
#include "TextImage.h"
#include "GameSession.h" // 追加

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
            void Draw() override;

			// NoteManagerから呼ばれる窓口
			void SpawnHitEffect(float x, float y, float scale, float duration, const Color& color);
            void SpawnSlashEffect(float x, float y, float scaleX, float scaleY, float duration, const Color& color);

			void UpdateTimerDisplay(float time);
			void UpdateComboDisplay(int combo);
			void UpdateJudgeCountDisplay();

			// ★ここを SetJudgeImage に戻しました（中身はテキスト更新です）
			void SetJudgeImage(JudgeResult result);

			void UpdateJacketImage();

			sf::SafePtr<NoteManager> noteManager;
			void DestroyEffect(sf::ui::Image* effect);

			// 依存性注入（Dependency Injection）
			void SetSongInfo(const SongInfo* info) { songInfoPtr = info; }


			// 3.2.1.START の表示更新
			void UpdateCountdownDisplay(float time, bool isStart);

			void ShowFastSlow(int type); // 1:FAST, 2:SLOW
            
            // Modified: Receive Score and Rank directly
            void UpdateScoreDisplay(int score, GameSession::Rank rank);
            
            void DrawScoreGauge();

		private:
			/// 選択された曲情報への参照（DI）
			const SongInfo* songInfoPtr = nullptr;

			// ... (既存のテクスチャ) ...
			sf::Texture textureDefaultJacket;
			sf::ui::Image* Jacket;
			sf::Texture textureJacket;

			// エフェクト用
			sf::Texture textureHitEffect;
            sf::Texture textureSlashEffect; // New Slash Texture

			sf::SafePtr<EffectManager> effectManager;
            sf::SafePtr<EffectManager> slashEffectManager; // New Manager for Slash

			// ---------------------------------------------------------
			// テキストUI
			// ---------------------------------------------------------
			sf::ui::TextImage* comboText = nullptr;
			// 判定内訳（個別）
			sf::ui::TextImage* perfectText = nullptr;
			sf::ui::TextImage* greatText = nullptr;
			sf::ui::TextImage* goodText = nullptr;
			sf::ui::TextImage* missText = nullptr;

			// 判定結果表示用テキスト
			sf::ui::TextImage* judgeResultText = nullptr;
			sf::ui::TextImage* fastSlowText = nullptr; // FAST/SLOW
			sf::ui::TextImage* countdownText = nullptr;
            
            // Score & Rank
            sf::ui::TextImage* scoreText = nullptr;
            sf::ui::TextImage* rankText = nullptr;
            sf::ui::TextImage* rankLabels[4] = { nullptr }; // C, B, A, S Markers
            sf::ui::TextImage* rankOutline[4] = { nullptr }; // Outline for Rank

            // Song Info (Title, BPM, Difficulty)
            sf::ui::TextImage* titleText = nullptr;
            sf::ui::TextImage* bpmText = nullptr;
            sf::ui::TextImage* difficultyText = nullptr;



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

			int lastTotalJudges = -1;
			float judgeScaleTimer = 0.0f;
			float fastSlowScaleTimer = 0.0f;

		};
	}
}
