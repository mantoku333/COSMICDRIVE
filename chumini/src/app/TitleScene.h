#pragma once
#include "App.h"
#include "SceneChangeComponent.h"

namespace app {
    namespace test {
        
        class BGMComponent;
        class Live2DComponent;
        class TitleCanvas;

        // ============================================================
        // タイトルボタン定義
        // ============================================================
        enum class TitleButton {
            Config = 0,
            Play = 1,
            Exit = 2
        };

        // ============================================================
        // TitleScene - タイトル画面のPresenter（MVP）
        // 
        // 役割:
        // - 入力処理（CanvasからのUI操作を受け取る）
        // - 状態管理（選択ボタン）
        // - シーン遷移ロジック
        // - Canvasへの描画指示
        // ============================================================
        class TitleScene : public sf::Scene<TitleScene> {
        public:
            void Init() override;
            void Update(const sf::command::ICommand& command);
            void Draw() override;
            void DrawOverlay() override;
            void OnGUI() override;

            // --------------------------------------------
            // 入力ハンドラ（Canvasから呼ばれる）
            // --------------------------------------------
            
            /// 左方向ナビゲーション
            void OnNavigateLeft();
            
            /// 右方向ナビゲーション
            void OnNavigateRight();
            
            /// ボタン選択（マウスホバー等）
            void OnSelectButton(TitleButton button);
            
            /// 確定アクション
            void OnConfirm();

            // --------------------------------------------
            // 状態取得（Canvas描画用）
            // --------------------------------------------
            
            /// 現在選択されているボタンを取得
            TitleButton GetSelectedButton() const { return selectedButton; }

        private:
            // --------------------------------------------
            // 状態
            // --------------------------------------------
            TitleButton selectedButton = TitleButton::Play;

            // --------------------------------------------
            // コンポーネント参照
            // --------------------------------------------
            sf::ref::Ref<sf::Actor> uiManagerActor;
            sf::SafePtr<app::test::BGMComponent> bgmPlayer;
            sf::SafePtr<Live2DComponent> l2dComp;
            sf::SafePtr<SceneChangeComponent> sceneChanger;
            sf::SafePtr<TitleCanvas> titleCanvas;

            sf::command::Command<> updateCommand;

            // --------------------------------------------
            // 内部メソッド
            // --------------------------------------------
            
            /// 選択中ボタンのアクションを実行
            void ExecuteButtonAction();
            
            /// 選曲画面へ遷移
            void NavigateToSelect();
            
            /// 設定画面へ遷移
            void NavigateToConfig();
            
            /// アプリケーション終了
            void ExitApplication();
        };

    }
}