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
        class SelectScene;

        /// 楽曲選択画面のUI・アニメーション・表示を管理するキャンバス
        /// MVPパターンの View に相当し、Presenterは SelectScene
        class SelectCanvas : public sf::ui::Canvas {
        public:
            void Begin() override;
            void Update(const sf::command::ICommand&);

            /// Presenter（SelectScene）を設定する
            void SetPresenter(SelectScene* scene) { presenter = scene; }

            // --- 入力操作 ---
            void SelectNext();         // 次の曲を選択
            void SelectPrevious();     // 前の曲を選択
            void ConfirmSelection();   // 選択を確定
            void CancelSelection();    // 選択をキャンセル（タイトルへ戻る）

            // --- Presenterから呼ばれるメソッド ---
            int GetSongCount() const { return (int)songs.size(); }
            void SetTargetIndex(int index);
            void SetGenreSelectMode(bool isGenreMode);
            void SetShowRatingDetail(bool show);
            void ChangeGenre(int direction);  // +1=次 / -1=前

            // --- 選択情報 ---
            int GetSelectedSongIndex() const { return targetIndex; }
            const SongInfo& GetSelectedSong() const;

            sf::ui::TextImage* songTitleText = nullptr; // 選択中の曲タイトル表示用

        private:
            SelectScene* presenter = nullptr;  // Presenter参照

            sf::command::Command<>  updateCommand;
            sf::SafePtr<SceneChangeComponent> sceneChanger;

            // ===== テクスチャリソース =====
            sf::Texture textureDefaultJacket;     // デフォルトジャケット
            sf::Texture textureSelectFrame;       // 選択フレーム
            sf::Texture textureBack;              // 背景
            sf::Texture textureDifficultyBasic;   // 難易度バー
            sf::Texture textureDifficultyAdvanced;
            sf::Texture textureDifficultyExpert;
            sf::Texture textureDifficultyMaster;
            sf::Texture textureDifficultyUltima;
            sf::Texture textureTitlePanel;        // タイトルパネル
            sf::Texture textureCC;                // クレジット

            // ===== 楽曲データ =====
            std::vector<SongInfo> songs;             // 楽曲リスト
            std::vector<sf::Texture> jacketTextures; // 各曲のジャケットテクスチャ

            // ===== UI参照 =====
            sf::ui::Image* selectFrame = nullptr;    // 選択フレーム画像
            sf::ui::Image* CC = nullptr;

            // ジャケット表示用プール
            std::vector<sf::ui::Image*> jacketPool;       // 表示中のジャケットポインタ
            std::vector<sf::ui::Image*> jacketComponents; // ジャケットUIコンポーネント実体

            // 曲情報テキスト
            sf::ui::TextImage* titleText = nullptr;       // 「SONG SELECT」ヘッダー
            sf::ui::TextImage* titleOutline[4] = { nullptr }; // ヘッダーのアウトライン
            sf::ui::TextImage* artistText = nullptr;      // アーティスト名
            sf::ui::TextImage* bpmText = nullptr;         // BPM
            sf::ui::TextImage* difficultyText = nullptr;  // 難易度レベル
            sf::ui::TextImage* highScoreText = nullptr;   // ハイスコア
            sf::ui::TextImage* rankMark = nullptr;        // ランクマーク
            sf::ui::TextImage* playerRatingText = nullptr;// プレイヤーレーティング

            // ===== レーティング詳細表示 =====
            bool showRatingDetail = false;
            float ratingDetailAnimationTimer = 0.0f;             // アニメーション進行率（0.0〜1.0）
            const float RATING_DETAIL_ANIMATION_DURATION = 0.5f; // アニメーション時間（秒）
            sf::Texture ratingDetailBackgroundTexture;            // レーティング詳細の背景テクスチャ
            sf::ui::Image* ratingDetailBackground = nullptr;     // 半透明背景
            sf::ui::TextImage* ratingDetailTitle = nullptr;      // 「Top 10 Charts」タイトル
            sf::ui::TextImage* ratingDetailLabelText = nullptr;  // 「RATING」ラベル
            sf::ui::TextImage* ratingDetailTotalText = nullptr;  // 合計レート表示
            std::vector<sf::ui::TextImage*> ratingDetailLines;   // 上位チャート詳細行
            int ratingDetailItemCount = 0;                       // アクティブな詳細行数

            // ===== レイアウト定数 =====
            int   MAX_VISIBLE = 7;           // 画面に表示する最大スロット数（奇数推奨）
            float BASE_SPACING = 350.0f;     // スロット間隔（px）
            float SCALE_CENTER = 3.5f;       // 中央ジャケットのスケール
            float SCALE_EDGE = 0.5f;         // 端ジャケットのスケール
            float DEPTH_NEAR = 0.0f;         // 手前のZ座標
            float DEPTH_FAR = -2.0f;         // 奥のZ座標

            // ジャケット中心座標
            float LAYOUT_JACKET_CENTER_X = 0.0f;
            float LAYOUT_JACKET_CENTER_Y = -80.0f;

            // 各UI要素のY座標
            float LAYOUT_TITLE_Y = 400.0f;       // 「SONG SELECT」ヘッダー
            float LAYOUT_GENRE_Y = 230.0f;       // ジャンル名
            float LAYOUT_SONG_TITLE_Y = -270.0f; // 曲名
            float LAYOUT_ARTIST_Y = -350.0f;     // アーティスト名
            float LAYOUT_BPM_Y = -420.0f;        // BPM

            float LAYOUT_SONG_INFO_Z = 2.0f;     // 曲情報のZ座標（ジャケットより手前）

            // ===== 選択・アニメーション =====
            int   selectedIndex = 0;       // 現在の選択インデックス
            float currentIndex = 0.0f;     // 表示用の補間位置（連続値）
            int   targetIndex = 0;         // 目標の選択インデックス（整数）
            float animationTime = 0.0f;    // アニメーション経過時間

            float slideSpeed = 12.0f;      // スライド補間速度
            float snapEps = 0.001f;        // スナップ許容誤差

            float slideStartIdx = 0.0f;    // 補間開始時のcurrentIndex
            float slideTimer = 0.0f;       // スライド経過時間
            float slideDuration = 0.25f;   // スライド補間時間（秒）

            // ===== 音声プレビュー =====
            sf::sound::SoundPlayer previewPlayer;
            sf::ref::Ref<sf::sound::SoundResource> previewResource;
            std::future<sf::sound::SoundResource*> loadingFuture; // 非同期読み込み用
            std::atomic<bool> isLoading{ false };

            // ===== 操作モード =====
            enum class InputMode {
                SongSelect,    // 曲選択モード
                GenreSelect    // ジャンル選択モード
            };
            InputMode selectionMode = InputMode::SongSelect;

            // ===== ジャンル管理 =====
            SongListService songListService;                 // ジャンル・楽曲リストの管理サービス
            sf::ui::TextImage* genreText = nullptr;          // 現在のジャンル名
            sf::ui::TextImage* prevGenreText = nullptr;      // 前のジャンル名
            sf::ui::TextImage* nextGenreText = nullptr;      // 次のジャンル名

            void SelectNextGenre();
            void SelectPreviousGenre();

            // ===== 内部処理メソッド =====
            void InitializeTextures();      // テクスチャの読み込み
            void InitializeSongs();         // 楽曲データの初期化
            void PlayPreview();             // プレビュー音声の再生
            void InitializeUI();            // UI要素の生成
            void RebuildJacketPool();       // ジャケット表示プールの再構築
            void UpdateAnimation();         // currentIndex → targetIndex へ補間
            void UpdateJacketPositions();   // ジャケットの位置・スケール・深度の更新
            void UpdateSelectedFrame();     // 選択枠を中央スロットへ配置
            void UpdateSongInfo();          // 曲情報テキストの更新
            static float ScaleByDistance(float d, float scaleCenter, float scaleEdge);

        };

    } // namespace test
} // namespace app
