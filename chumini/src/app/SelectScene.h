#pragma once
#include "App.h"
#include "SceneChangeComponent.h"
#include "SongInfo.h"

namespace app
{
	namespace test
	{
		class SelectCanvas;

		// ============================================================
		// SelectScene - 選曲画面のPresenter（MVP）
		// 
		// 役割:
		// - 入力処理（キーボード・マウス）
		// - 状態管理（選択インデックス、入力クールダウン、モード）
		// - シーン遷移ロジック
		// ============================================================
		class SelectScene : public sf::Scene<SelectScene>
        {
        public:
            void Init() override;
            void Update(const sf::command::ICommand& command);

			// --------------------------------------------
			// MVP: 入力ハンドラ（Canvasから呼ばれる）
			// --------------------------------------------
			void OnSelectNext();
			void OnSelectPrevious();
			void OnConfirmSelection();
			void OnCancelSelection();
			void OnModeUp();    // 曲→ジャンルモード
			void OnModeDown();  // ジャンル→曲モード
			void OnGenreNext();
			void OnGenrePrevious();
			void OnToggleRatingDetail();

			// --------------------------------------------
			// MVP: シーン遷移
			// --------------------------------------------
			void NavigateToGame(const SongInfo& song);
			void NavigateToTitle();

			// --------------------------------------------
			// MVP: 状態取得（Canvas描画用）
			// --------------------------------------------
			int GetTargetIndex() const { return targetIndex; }
			bool IsGenreSelectMode() const { return isGenreSelectMode; }
			bool IsShowRatingDetail() const { return showRatingDetail; }

        private:
			// --------------------------------------------
			// 状態
			// --------------------------------------------
			int targetIndex = 0;
			float inputCooldown = 0.0f;
			static constexpr float INPUT_DELAY = 0.15f;
			bool isGenreSelectMode = false;  // true=ジャンル選択, false=曲選択
			bool showRatingDetail = false;

			// --------------------------------------------
			// コンポーネント参照
			// --------------------------------------------
            sf::ref::Ref<sf::Actor> uiManagerActor;
			sf::SafePtr<SceneChangeComponent> sceneChanger;
			sf::SafePtr<SelectCanvas> selectCanvas;
            sf::command::Command<> updateCommand;

			// --------------------------------------------
			// 内部メソッド
			// --------------------------------------------
			void ProcessInput();
        };
	}
}
