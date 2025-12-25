#pragma once
#include "App.h"
#include "SongInfo.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"


namespace app {
    namespace test {
        class SelectCanvas : public sf::ui::Canvas {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand&);

            // 入力系
            void SelectNext();
            void SelectPrevious();
            void ConfirmSelection();
            void CancelSelection();

            // 選択情報（実際に確定されるのは targetIndex 基準）
            int GetSelectedSongIndex() const { return targetIndex; }
            const SongInfo& GetSelectedSong() const;

            sf::ui::TextImage* songTitleText = nullptr; // 現在選択中の曲タイトル表示用

        private:
            sf::command::Command<>  updateCommand;
            sf::SafePtr<SceneChangeComponent> sceneChanger;

            // =========================
            // リソース（テクスチャ）
            // =========================
            sf::Texture textureDefaultJacket;     // デフォルトジャケット
            sf::Texture textureSelectFrame;       // 選択フレーム
            sf::Texture textureBack;              // 背景
            sf::Texture textureDifficultyBasic;   // 難易度バー（使うなら）
            sf::Texture textureDifficultyAdvanced;
            sf::Texture textureDifficultyExpert;
            sf::Texture textureDifficultyMaster;
            sf::Texture textureDifficultyUltima;
            sf::Texture textureTitlePanel;        // タイトルパネル
            sf::Texture textureCC;                // クレジット等

            // =========================
            // 楽曲データ
            // =========================
            std::vector<SongInfo> songs;          // 楽曲リスト
            std::vector<sf::Texture> jacketTextures; // 各曲ジャケット

            // =========================
            // UI参照
            // =========================
            sf::ui::Image* selectFrame = nullptr;
            sf::ui::Image* CC = nullptr;

            // ジャケット用プール
            std::vector<sf::ui::Image*> jacketPool;

            sf::ui::TextImage* titleText = nullptr;
            sf::ui::TextImage* titleOutline[4] = { nullptr };
            sf::ui::TextImage* artistText = nullptr;
            sf::ui::TextImage* bpmText = nullptr;

            // =========================
            // レイアウト / 見た目
            // =========================
            // 画面に出す最大スロット数（奇数推奨：中央スロットを持つため）
            int   MAX_VISIBLE = 7;
            float BASE_SPACING = 350.0f;   // スロット間隔(px)
            float SCALE_CENTER = 3.5f;     // 中央スケール
            float SCALE_EDGE = 0.5f;     // 端スケール
            float DEPTH_NEAR = 0.0f;     // 手前Z
            float DEPTH_FAR = -2.0f;    // 奥Z

            // 画面中央
            static constexpr float CENTER_X = 0.0f;
            static constexpr float CENTER_Y = -100.0f;

            // =========================
            // 選択 / アニメ
            // =========================
            int   selectedIndex = 0;
            float currentIndex = 0.0f;     // 見た目上の選択位置（連続値）  // 実際の選択は targetIndex（整数）。表示は currentIndex（連続値）で補間
            int   targetIndex = 0;        // 実選択（整数）
            float animationTime = 0.0f;

            float slideSpeed = 12.0f;    // スライド補間速度（指数Lerp）
            float snapEps = 0.001f;   // スナップ許容誤差

            // 入力制御
            float inputCooldown = 0.0f;
            static constexpr float INPUT_DELAY = 0.15f;

            // スライド補間用
            float slideStartIdx = 0.0f;     // 補間開始時の currentIndex
            float slideTimer = 0.0f;     // 経過時間
            float slideDuration = 0.25f;    // 補間時間（秒）


            // =========================
            // 内部メソッド
            // =========================
            void InitializeTextures();
            void InitializeSongs();
            void InitializeUI();
            void RebuildJacketPool();
            void UpdateInput();
            void UpdateAnimation();        // currentIndex → targetIndex へ補間
            void UpdateJacketPositions();  // 位置/スケール/深度の更新
            void UpdateSelectedFrame();    // 枠は中央スロットへ
            void UpdateSongInfo();         // INFO=ジャケット流用
            static float ScaleByDistance(float d, float scaleCenter, float scaleEdge);

        };

    } // namespace test
} // namespace app
