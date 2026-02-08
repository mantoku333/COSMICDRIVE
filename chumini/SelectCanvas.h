#pragma once
#include "App.h"
#include "SongInfo.h"
#include "TextImage.h"
#include "SceneChangeComponent.h"
#include "SoundPlayer.h"
#include "SoundResource.h"
#include "SongListService.h"


#include <future>
#include <mutex>
#include <atomic>

namespace app {
    namespace test {
        struct SongInfo;
        struct Genre;
        class SelectScene;  // MVP: Presenter前方宣言

        class SelectCanvas : public sf::ui::Canvas {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand&);

            // MVP: Presenter設定
            void SetPresenter(SelectScene* scene) { presenter = scene; }

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
            // MVP: Presenter参照
            SelectScene* presenter = nullptr;

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
            std::vector<sf::ui::Image*> jacketPool;      // 現在使用中のジャケット(へのポインタ)
            std::vector<sf::ui::Image*> jacketComponents; // 実体(Component)管理用

            sf::ui::TextImage* titleText = nullptr;
            sf::ui::TextImage* titleOutline[4] = { nullptr };
            sf::ui::TextImage* artistText = nullptr;
            sf::ui::TextImage* bpmText = nullptr;
            sf::ui::TextImage* difficultyText = nullptr; // Define difficulty text
            sf::ui::TextImage* highScoreText = nullptr;
            sf::ui::TextImage* rankMark = nullptr;
            sf::ui::TextImage* playerRatingText = nullptr; // Player rating display

            // Rating detail display
            bool showRatingDetail = false;
            float ratingDetailAnimationTimer = 0.0f; // アニメーション用タイマー（0.0f〜1.0f）
            const float RATING_DETAIL_ANIMATION_DURATION = 0.5f; // アニメーション時間（秒） 0.3 -> 0.5
            sf::Texture ratingDetailBackgroundTexture; // 背景用テクスチャ
            sf::ui::Image* ratingDetailBackground = nullptr; // 半透明背景
            sf::ui::TextImage* ratingDetailTitle = nullptr;
            sf::ui::TextImage* ratingDetailLabelText = nullptr; // "RATING" ラベル
            sf::ui::TextImage* ratingDetailTotalText = nullptr; // 合計レート表示
            std::vector<sf::ui::TextImage*> ratingDetailLines; // Top 10 chart details
            int ratingDetailItemCount = 0; // アクティブな詳細ライン数

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
            // static constexpr float CENTER_X = 0.0f;
            // static constexpr float CENTER_Y = 50.0f; // ★ -100 -> 50 に上げて、曲情報との被りを防ぐ

            // レイアウト微調整用 (ヘッダーでいじれるように)
            // ジャケット中心
            float LAYOUT_JACKET_CENTER_X = 0.0f;
            float LAYOUT_JACKET_CENTER_Y = -80.0f;

            // 各UIのY座標
            float LAYOUT_TITLE_Y = 400.0f;       // SONG SELECT
            float LAYOUT_GENRE_Y = 230.0f;       // Genre Name
            float LAYOUT_SONG_TITLE_Y = -270.0f; // Song Title
            float LAYOUT_ARTIST_Y = -350.0f;     // Artist Name
            float LAYOUT_BPM_Y = -420.0f;        // BPM

            // Zオーダー (手前を大きくする)
            float LAYOUT_SONG_INFO_Z = 2.0f;     // ジャケット(0)より手前に

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

            // 音声プレビュー
            sf::sound::SoundPlayer previewPlayer;
            sf::ref::Ref<sf::sound::SoundResource> previewResource;

            // 非同期読み込み用
            std::future<sf::sound::SoundResource*> loadingFuture;
            std::atomic<bool> isLoading{ false };

            // ★操作モード
            enum class InputMode {
                SongSelect,
                GenreSelect
            };
            InputMode selectionMode = InputMode::SongSelect; // デフォルトは曲選択


            // ジャンル管理は SongListService に委譲
            SongListService songListService;
            sf::ui::TextImage* genreText = nullptr;
            sf::ui::TextImage* prevGenreText = nullptr; // 前のジャンル
            sf::ui::TextImage* nextGenreText = nullptr; // 次のジャンル

            void ChangeGenre(int index);
            void SelectNextGenre();
            void SelectPreviousGenre();

            void InitializeTextures();
            void InitializeSongs();
            void PlayPreview();
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
