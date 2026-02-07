#include "SelectCanvas.h"
#include "Time.h"
#include <algorithm>
#include <cmath>
#include "IngameScene.h"
#include "TitleScene.h"
#include "Easing.h"
#include "Config.h"

// テキスト描画に必要
#include "Text.h"
#include "TextImage.h"
#include "DWriteContext.h"
#include "DirectX11.h"
#include <Windows.h> 
#include "LoadingScene.h"

#include <filesystem>
#include <fstream>
#include "ChedParser.h" 
#include "ScoreManager.h" 
#include "RatingManager.h"
#include "StringUtils.h"  // 文字コード変換ユーティリティ

namespace app::test {

    // sf::util の関数を使用（ローカル関数は削除）
    using sf::util::WstringToUtf8;
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;

    // ===== ユーティリティ =====
    static inline float WrapFloat(float x, float N) {
        float r = std::fmod(x, N);
        if (r < 0) r += N;
        return r;
    }

    static inline float ShortestDeltaOnRing(float current, float target, float N) {
        float c = WrapFloat(current, N);
        float diff = target - c;
        if (diff > N * 0.5f) diff -= N;
        if (diff < -N * 0.5f) diff += N;
        return diff;
    }

    float SelectCanvas::ScaleByDistance(float d, float scaleCenter, float scaleEdge) {
        const float falloff = std::exp(-0.5f * (d * d));
        return scaleEdge + (scaleCenter - scaleEdge) * falloff;
    }

    // ===== Begin =====
    void SelectCanvas::Begin() {
        sf::ui::Canvas::Begin();

        if (auto actor = actorRef.Target()) {
            auto* changer = actor->GetComponent<SceneChangeComponent>();
            if (changer) {
                sceneChanger = changer;
            }
        }

        InitializeTextures();
        InitializeSongs();
        InitializeUI();

        selectedIndex = std::clamp(selectedIndex, 0, (int)songs.size() - 1);
        targetIndex = selectedIndex;
        currentIndex = (float)targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 起動時：曲タイトル初期化
        if (songTitleText && !songs.empty()) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // ★追加: アーティスト
        if (artistText) {
            artistText->SetText(Utf8ToWstring(songs[targetIndex].artist));
        }

        // ★追加: BPM
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // ★追加: 難易度
        if (difficultyText) {
            std::wstring levelStr = L"LEVEL: " + std::to_wstring(songs[targetIndex].level);
            difficultyText->SetText(levelStr);
        }

        // Score & Rank Update
        if (!songs.empty() && highScoreText && rankMark) {
            auto record = app::test::ScoreManager::Instance().GetScore(songs[targetIndex].chartPath);
            if (record.highScore > 0) {
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"High Score: %d", record.highScore);
                highScoreText->SetText(scoreBuf);
                std::wstring rW(record.rank.begin(), record.rank.end());
                rankMark->SetText(rW);
            } else {
                highScoreText->SetText(L"High Score: --------");
                rankMark->SetText(L"-");
            }
        }

        // ★追加: ジャンル名
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        if (!allGenres.empty()) {
            if(genreText) genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));

            int N = (int)allGenres.size(); // ジャンル数
            if (prevGenreText) {
                int prevIdx = (currentGenreIndex - 1 + N) % N;
                prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
            }
            if (nextGenreText) {
                int nextIdx = (currentGenreIndex + 1) % N;
                nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
            }
        }

        PlayPreview();

        updateCommand.Bind(std::bind(&SelectCanvas::Update, this, std::placeholders::_1));
    }

    // ===== テクスチャ初期化 =====
    void SelectCanvas::InitializeTextures() {
        textureBack.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        textureSelectFrame.LoadTextureFromFile("Assets\\Texture\\SelectFrame.png");
        textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\Jacket\\DefaultJacket.png");
        textureTitlePanel.LoadTextureFromFile("Assets\\Texture\\songselect.png");
        textureCC.LoadTextureFromFile("Assets\\Texture\\CC.png");
    }

    // ===== 楽曲初期化 =====
    void SelectCanvas::InitializeSongs() {
        // SongListServiceにスキャンを委譲
        songListService.ScanSongs("Songs");
        songs = songListService.GetCurrentSongs();

        // ----------------------------------------------------
        // ジャケットテクスチャのロード (★修正箇所)
        // ----------------------------------------------------
        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            bool loaded = false;
            if (!songs[i].jacketPath.empty()) {

                // ★修正: WindowsAPI (LoadTextureFromFile) に渡すために Shift-JIS に変換する
                // UTF-8 のままだと「指定されたファイルが見つかりません」になります
                std::string sjisPath = Utf8ToShiftJis(songs[i].jacketPath);

                if (jacketTextures[i].LoadTextureFromFile(sjisPath)) {
                    loaded = true;
                }
                else {
                    // ロード失敗ログもShift-JISで出すと文字化けしない
                    sf::debug::Debug::Log("Jacket Load Failed: " + sjisPath);
                }
            }

            if (!loaded) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    // ===== UI初期化 =====
    void SelectCanvas::InitializeUI() {

        auto* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();

        selectFrame = AddUI<sf::ui::Image>();
        selectFrame->transform.SetPosition(Vector3(LAYOUT_JACKET_CENTER_X, LAYOUT_JACKET_CENTER_Y, 1));
        selectFrame->transform.SetScale(Vector3(10.0f, 50.0f, 1.0f));
        selectFrame->material.texture = &textureSelectFrame;
        selectFrame->material.SetColor({ 1, 1, 1, 0.8f });

        /*CC = AddUI<sf::ui::Image>();
        CC->transform.SetPosition(Vector3(500, -350, 0));
        CC->transform.SetScale(Vector3(3.0f, 1.0f, 1.0f));
        CC->material.texture = &textureCC;*/

        // ジャケットUIコンポーネントの事前生成
        jacketComponents.clear();
        for(int i = 0; i < MAX_VISIBLE; ++i) {
             auto jacket = AddUI<sf::ui::Image>();
             jacket->transform.SetScale(Vector3(SCALE_EDGE, SCALE_EDGE, 1.0f));
             jacket->SetVisible(false); // 初期は非表示
             jacketComponents.push_back(jacket);
        }

        RebuildJacketPool();
        UpdateJacketPositions();

     // =========================================================
     // アウトラインの生成
     // =========================================================

     // ふちの太さ
        float outlineSize = 3.0f;

        // 4方向のズレを定義
        Vector3 offsets[4] = {
            Vector3(-outlineSize, 0, 0),
            Vector3(outlineSize, 0, 0),
            Vector3(0, -outlineSize, 0),
            Vector3(0,  outlineSize, 0)
        };

        // ループして4つ作る
        for (int i = 0; i < 4; ++i) {
            titleOutline[i] = AddUI<sf::ui::TextImage>();

            titleOutline[i]->transform.SetPosition(Vector3(0 + offsets[i].x, LAYOUT_TITLE_Y + offsets[i].y, 1.0f));

            titleOutline[i]->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

            titleOutline[i]->Create(
                device,
                L"SONG SELECT",     
                L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",   
                120.0f,         
                D2D1::ColorF(D2D1::ColorF::White),
                1024, 240
            );
        }

        // =========================================================
        // メイン文字（本体）の生成
        // =========================================================
        titleText = AddUI<sf::ui::TextImage>();
        titleText->transform.SetPosition(Vector3(0, LAYOUT_TITLE_Y, 0.0f));
        titleText->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

        titleText->Create(
            device,
            L"SONG SELECT",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // ★色は「白」
            1024, 240
        );

        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->transform.SetPosition(Vector3(0, LAYOUT_SONG_TITLE_Y, LAYOUT_SONG_INFO_Z));
        songTitleText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        songTitleText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1800, 200);

    // ---------------------------------------------------------
    // アーティスト名
    // ---------------------------------------------------------
        artistText = AddUI<sf::ui::TextImage>();
        artistText->transform.SetPosition(Vector3(0, LAYOUT_ARTIST_Y, LAYOUT_SONG_INFO_Z));
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            device,
            L"", // 初期値
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray), 
            1024, 200
        );

        // ---------------------------------------------------------
        // BPM表示
        // ---------------------------------------------------------
        bpmText = AddUI<sf::ui::TextImage>();
        bpmText->transform.SetPosition(Vector3(0, LAYOUT_BPM_Y, LAYOUT_SONG_INFO_Z));
        bpmText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        bpmText->Create(
            device,
            L"", // 初期値
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray), 
            1024, 200
        );

        // ---------------------------------------------------------
        // 難易度表示
        // ---------------------------------------------------------
        difficultyText = AddUI<sf::ui::TextImage>();
        difficultyText->transform.SetPosition(Vector3(650, -300, 1.0f)); // スコアの上
        difficultyText->transform.SetScale(Vector3(4.0f, 1.0f, 1.0f));
        difficultyText->Create(
            device,
            L"", 
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            40.0f, 
            D2D1::ColorF(D2D1::ColorF::White), 
            512, 128
        );

        // ---------------------------------------------------------
        // ★追加: ジャンル名表示 (SONG SELECTの下)
        // ---------------------------------------------------------
        genreText = AddUI<sf::ui::TextImage>();
        // SONG SELECT(480) より下 (350あたり)
        genreText->transform.SetPosition(Vector3(0, LAYOUT_GENRE_Y, 0)); 
        genreText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        genreText->Create(
            device,
            L"", 
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f, // 少し大きめ
            D2D1::ColorF(D2D1::ColorF::Yellow), // 目立つ色
            1024, 200
        );

        // ★追加: 前のジャンル
        prevGenreText = AddUI<sf::ui::TextImage>();
        prevGenreText->transform.SetPosition(Vector3(-550.0f, LAYOUT_GENRE_Y, 0)); // 左に配置
        prevGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f)); // サイズダウン
        prevGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f, // フォントサイズダウン (70 -> 50)
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // ★追加: 次のジャンル
        nextGenreText = AddUI<sf::ui::TextImage>();
        nextGenreText->transform.SetPosition(Vector3(550.0f, LAYOUT_GENRE_Y, 0)); // 右に配置
        nextGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f)); // サイズダウン
        nextGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f, // フォントサイズダウン (70 -> 50)
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // ★追加: ハイスコア
        // ★追加: ハイスコア
        highScoreText = AddUI<sf::ui::TextImage>();
        highScoreText->transform.SetPosition(Vector3(650, -400, 1.0f)); 
        highScoreText->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f));
        highScoreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            40.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1536, 128  // 横幅を1024から1536に拡大
        );

        // ★追加: ランクマーク
        rankMark = AddUI<sf::ui::TextImage>();
        rankMark->transform.SetPosition(Vector3(800, -350, 1.0f)); 
        rankMark->transform.SetScale(Vector3(3.0f, 3.0f, 1.0f));
        rankMark->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::Yellow),
            256, 256
        );


        // ★追加: プレイヤーレート表示（左下）
        playerRatingText = AddUI<sf::ui::TextImage>();
        playerRatingText->transform.SetPosition(Vector3(-750, -450, 1.0f)); // 左下
        playerRatingText->transform.SetScale(Vector3(6.0f, 1.5f, 1.0f));
        playerRatingText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 128
        );

        // プレイヤーレートを計算して表示（小数点第2位で切り捨て）
        float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
        // 切り捨て: 100倍して整数部分を取り、100で割る
        float truncatedRating = std::floor(playerRating * 100.0f) / 100.0f;
        wchar_t ratingBuf[64];
        swprintf_s(ratingBuf, L"Rating: %.2f", truncatedRating);
        playerRatingText->SetText(ratingBuf);

        // ★追加: レート詳細表示UI（初期は非表示）
        // 背景用テクスチャを読み込む（既存の背景を再利用）
        ratingDetailBackgroundTexture.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        
        // 半透明背景（左側に配置）
        ratingDetailBackground = AddUI<sf::ui::Image>();
        ratingDetailBackground->transform.SetPosition(Vector3(-550, 0, 5.0f)); // テキストより奥に
        ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f)); // 10倍に拡大
        ratingDetailBackground->material.texture = &ratingDetailBackgroundTexture;
        ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.90f}); // 黒、90%不透明
        ratingDetailBackground->SetVisible(false);

        // "RATING" ラベル（一番上）
        ratingDetailLabelText = AddUI<sf::ui::TextImage>();
        ratingDetailLabelText->transform.SetPosition(Vector3(-650, 460, 10.0f));
        ratingDetailLabelText->transform.SetScale(Vector3(6.0f, 2.0f, 1.0f));
        ratingDetailLabelText->Create(
            device,
            L"RATING",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            60.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            512, 128
        );
        ratingDetailLabelText->SetVisible(false);

        // 合計レート表示（ラベルの下）
        ratingDetailTotalText = AddUI<sf::ui::TextImage>();
        ratingDetailTotalText->transform.SetPosition(Vector3(-650, 380, 10.0f)); // 400 -> 380
        ratingDetailTotalText->transform.SetScale(Vector3(10.0f, 3.0f, 1.0f));
        ratingDetailTotalText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f, // 大きく
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 200
        );
        ratingDetailTotalText->SetVisible(false);

        // タイトル（合計レートの下）
        ratingDetailTitle = AddUI<sf::ui::TextImage>();
        ratingDetailTitle->transform.SetPosition(Vector3(-650, 280, 10.0f)); // 300 -> 280
        ratingDetailTitle->transform.SetScale(Vector3(8.0f, 2.0f, 1.0f));
        ratingDetailTitle->Create(
            device,
            L"Top 10 Charts",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            60.0f,
            D2D1::ColorF(D2D1::ColorF::Yellow),
            1024, 128
        );
        ratingDetailTitle->SetVisible(false);

        // 詳細リスト（10行、左側）
        for (int i = 0; i < 10; ++i) {
            auto* line = AddUI<sf::ui::TextImage>();
            line->transform.SetPosition(Vector3(-650, 200 - i * 70, 10.0f)); // 220 -> 200
            line->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f)); // 少し小さく
            line->Create(
                device,
                L"",
                L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
                40.0f,
                D2D1::ColorF(D2D1::ColorF::White),
                2048, 128
            );
            line->SetVisible(false);
            ratingDetailLines.push_back(line);
        }



    }

    // ===== ジャケット再構築 =====
    void SelectCanvas::RebuildJacketPool() {
        // 全て一旦非表示
        for (auto* img : jacketComponents) {
            if (img) img->SetVisible(false);
        }
        jacketPool.clear();

        if (songs.empty()) return;

        // 必要数を計算
        const int poolSize = std::min<int>(MAX_VISIBLE, std::max<int>(3, (int)songs.size()));
        
        // 事前生成済みコンポーネントから割り当て
        for (int i = 0; i < poolSize; ++i) {
            if (i < (int)jacketComponents.size()) {
                 auto* jacket = jacketComponents[i];
                 jacket->SetVisible(true);
                 jacketPool.push_back(jacket);
            }
        }
    }

    // ===== Update =====
    void SelectCanvas::Update(const sf::command::ICommand& command) {
        animationTime += sf::Time::DeltaTime();
        if (inputCooldown > 0) inputCooldown -= sf::Time::DeltaTime();

        // レーティング詳細アニメーション更新
        if (showRatingDetail && ratingDetailAnimationTimer < 1.0f) {
            ratingDetailAnimationTimer += sf::Time::DeltaTime() / RATING_DETAIL_ANIMATION_DURATION;
            if (ratingDetailAnimationTimer > 1.0f) ratingDetailAnimationTimer = 1.0f;
        } else if (!showRatingDetail && ratingDetailAnimationTimer > 0.0f) {
            ratingDetailAnimationTimer -= sf::Time::DeltaTime() / RATING_DETAIL_ANIMATION_DURATION;
            if (ratingDetailAnimationTimer < 0.0f) ratingDetailAnimationTimer = 0.0f;
        }

        UpdateInput();
        UpdateAnimation();
        UpdateJacketPositions();
        UpdateSelectedFrame();
    }

    // ===== 入力 =====
    void SelectCanvas::UpdateInput() {
        if (inputCooldown > 0) return;
        SInput& input = SInput::Instance();

        // モード切り替え (上下)
        if (input.GetKeyDown(Key::KEY_UP) || input.GetKeyDown(Key::KEY_W)) {
            // 曲 -> ジャンル
            if (selectionMode == InputMode::SongSelect) {
                selectionMode = InputMode::GenreSelect;
                sf::debug::Debug::Log("Mode: Genre Select");
                // 視覚的な変化 (initializer listを使用)
                if(genreText) genreText->material.SetColor({1.0f, 1.0f, 0.0f, 1.0f}); // 黄色強調
                if(selectFrame) selectFrame->material.SetColor({0.5f, 0.5f, 0.5f, 0.5f}); // 枠を暗く
            }
            inputCooldown = INPUT_DELAY;
        }
        if (input.GetKeyDown(Key::KEY_DOWN) || input.GetKeyDown(Key::KEY_S)) {
            // ジャンル -> 曲
            if (selectionMode == InputMode::GenreSelect) {
                selectionMode = InputMode::SongSelect;
                sf::debug::Debug::Log("Mode: Song Select");
                if(genreText) genreText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 白に戻す
                if(selectFrame) selectFrame->material.SetColor({1.0f, 1.0f, 1.0f, 0.8f}); // 枠を明るく
            }
            inputCooldown = INPUT_DELAY;
        }

        // クリック判定: レーティングテキストをクリックで詳細表示切り替え
        if (SInput::Instance().GetMouseDown(0)) {  // 左クリック
            POINT mousePoint;
            GetCursorPos(&mousePoint);
            HWND hwnd = GetActiveWindow();
            ScreenToClient(hwnd, &mousePoint);
            
            // マウス座標をUI座標系に変換（画面中央が原点）
            float uiX = static_cast<float>(mousePoint.x) - 1920.0f * 0.5f;
            float uiY = 1080.0f * 0.5f - static_cast<float>(mousePoint.y);
            
            // PlayerRatingTextの当たり判定
            if (playerRatingText) {
                Vector3 pos = playerRatingText->transform.GetPosition();
                Vector3 scale = playerRatingText->transform.GetScale();
                
                // 幅と高さの計算（経験的な係数を使用）
                float width = scale.x * 80.0f;
                float height = scale.y * 40.0f;
                
                float left = pos.x - width * 0.5f;
                float right = pos.x + width * 0.5f;
                float top = pos.y + height * 0.5f;
                float bottom = pos.y - height * 0.5f;
                
                // 当たり判定
                if (uiX >= left && uiX <= right && uiY >= bottom && uiY <= top) {
                    showRatingDetail = !showRatingDetail;
                    
                    if (showRatingDetail) {
                        // データ設定（アニメーションはUpdateAnimationで自動処理）
                        // 1. 合計レート設定
                        if (ratingDetailTotalText) {
                            float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
                            float truncated = std::floor(playerRating * 100.0f) / 100.0f;
                            wchar_t totalBuf[64];
                            swprintf_s(totalBuf, L"%.2f", truncated);
                            ratingDetailTotalText->SetText(totalBuf);
                        }

                        // 2. 詳細リスト設定
                        auto topCharts = app::test::RatingManager::Instance().GetTopCharts(10);
                        ratingDetailItemCount = std::min((int)topCharts.size(), (int)ratingDetailLines.size()); // 件数を保存
                        
                        for (size_t i = 0; i < ratingDetailLines.size(); ++i) {
                            if (i < topCharts.size()) {
                                // titleを取得
                                std::string title = std::get<0>(topCharts[i]);
                                
                                // 切り捨てレート値
                                float rating = std::get<1>(topCharts[i]);
                                float truncated = std::floor(rating * 100.0f) / 100.0f;
                                
                                // テキスト設定（UTF-8からwstringへ変換）
                                wchar_t lineBuf[256];
                                std::wstring titleW = Utf8ToWstring(title);
                                swprintf_s(lineBuf, L"%d. %s - %.2f", (int)i + 1, titleW.c_str(), truncated);
                                
                                ratingDetailLines[i]->SetText(lineBuf);
                            } else {
                                ratingDetailLines[i]->SetText(L"");
                            }
                        }
                    }
                    
                    inputCooldown = INPUT_DELAY;
                }
            }
        }

        // Rキー: レート詳細表示の切り替え
        if (input.GetKeyDown(Key::KEY_R)) {
            showRatingDetail = !showRatingDetail;
            
            if (showRatingDetail) {
                // データ設定（アニメーションはUpdateAnimationで自動処理）
                // 1. 合計レート設定
                if (ratingDetailTotalText) {
                    float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
                    float truncated = std::floor(playerRating * 100.0f) / 100.0f;
                    wchar_t totalBuf[64];
                    swprintf_s(totalBuf, L"%.2f", truncated);
                    ratingDetailTotalText->SetText(totalBuf);
                }

                // 2. 詳細リスト設定
                auto topCharts = app::test::RatingManager::Instance().GetTopCharts(10);
                ratingDetailItemCount = std::min((int)topCharts.size(), (int)ratingDetailLines.size()); // 件数を保存
                
                for (size_t i = 0; i < ratingDetailLines.size(); ++i) {
                    if (i < topCharts.size()) {
                        // titleを取得
                        std::string title = std::get<0>(topCharts[i]);
                        
                        // 切り捨てレート値
                        float rating = std::get<1>(topCharts[i]);
                        float truncated = std::floor(rating * 100.0f) / 100.0f;
                        
                        // テキスト設定（UTF-8からwstringへ変換）
                        wchar_t lineBuf[256];
                        std::wstring titleW = Utf8ToWstring(title);
                        swprintf_s(lineBuf, L"%d. %s - %.2f", (int)i + 1, titleW.c_str(), truncated);
                        
                        ratingDetailLines[i]->SetText(lineBuf);
                    } else {
                         ratingDetailLines[i]->SetText(L"");
                    }
                }
            }
            
            inputCooldown = INPUT_DELAY;
        }

        // 左右操作 (モード依存)
        if (input.GetKeyDown(Key::KEY_RIGHT) || input.GetKeyDown(Key::KEY_D)) {
            if (selectionMode == InputMode::SongSelect) {
                SelectNext();
            } else {
                SelectNextGenre();
            }
            inputCooldown = INPUT_DELAY;
        }
        if (input.GetKeyDown(Key::KEY_LEFT) || input.GetKeyDown(Key::KEY_A)) {
            if (selectionMode == InputMode::SongSelect) {
                SelectPrevious();
            } else {
                SelectPreviousGenre();
            }
            inputCooldown = INPUT_DELAY;
        }

        // 決定・キャンセル
        if (input.GetKeyDown(Key::SPACE) || input.GetKeyDown(Key::KEY_Z)) {
            ConfirmSelection();
            inputCooldown = INPUT_DELAY;
        }
        if (input.GetKeyDown(Key::ESCAPE) || input.GetKeyDown(Key::KEY_X)) {
            CancelSelection();
            inputCooldown = INPUT_DELAY;
        }
    }

    // ===== アニメーション =====
    void SelectCanvas::UpdateAnimation() {
        // ... (existing slide logic) ...

        const float pulsePhase = std::fmod(animationTime * 0.8f, 1.0f);
        const float pulse = (float)Easing(pulsePhase, EASE::EaseYoyo);
        const float pulseScale = 1.0f + 0.04f * pulse;

        // Visual State Update based on Mode
        if (selectionMode == InputMode::GenreSelect) {
            // Genre: Blink Yellow
            float blink = 0.5f + 0.5f * (std::sin(animationTime * 8.0f) + 1.0f) * 0.5f;
            if (genreText) {
                // R=1, G=1, B=0, A=blink
                genreText->material.SetColor({ 1.0f, 1.0f, 0.0f, blink });
            }
            if (prevGenreText) prevGenreText->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
            if (nextGenreText) nextGenreText->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });

            // Song Area: Gray out
            if (selectFrame) selectFrame->material.SetColor({ 0.3f, 0.3f, 0.3f, 0.5f }); // Dim
            
            // Jackets: Dim
            for (auto* jacket : jacketPool) {
                if(jacket) jacket->material.SetColor({ 0.4f, 0.4f, 0.4f, 1.0f });
            }
            if(songTitleText) songTitleText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(artistText) artistText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(bpmText) bpmText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(difficultyText) difficultyText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});

        } else {
            // Song Select: Active
            if (genreText) {
                 // White, steady
                 genreText->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            }
            if (prevGenreText) prevGenreText->material.SetColor({ 0.4f, 0.4f, 0.4f, 0.8f }); // Slightly dim in song mode
            if (nextGenreText) nextGenreText->material.SetColor({ 0.4f, 0.4f, 0.4f, 0.8f });

            // Song Area: Active (Pulse Frame)
           if (selectFrame)
            {
                float alpha = 0.5f + 0.5f * pulse;
                selectFrame->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
            }

            // Jackets: White (Normal)
            for (auto* jacket : jacketPool) {
                if(jacket) jacket->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            }
             if(songTitleText) songTitleText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(artistText) artistText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(bpmText) bpmText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(difficultyText) difficultyText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
        }
        
        // Frame Breathing (Scale only)
        if (selectFrame && !jacketPool.empty()) {
            const int half = (int)jacketPool.size() / 2;
            const Vector3 jacketScale = jacketPool[half]->transform.GetScale();

            selectFrame->transform.SetScale(Vector3(
                jacketScale.x * 1.05f * pulseScale,
                jacketScale.y * 1.05f * pulseScale,
                1.0f
            ));
        }


        const int N = (int)songs.size();
        if (N <= 0) return;

        const float endPos = slideStartIdx + ShortestDeltaOnRing(slideStartIdx, (float)targetIndex, (float)N);
        const float dist = std::fabs(endPos - slideStartIdx);

        if (dist > 1e-4f) {
            slideTimer += sf::Time::DeltaTime();
            float alpha = std::clamp(slideTimer / slideDuration, 0.0f, 1.0f);
            float eased = (float)Easing(alpha, EASE::EaseInOutCubic);
            currentIndex = slideStartIdx + (endPos - slideStartIdx) * eased;

            if (alpha >= 1.0f) {
                currentIndex = WrapFloat(endPos, (float)N);
                slideStartIdx = currentIndex;
                slideTimer = 0.0f;
                selectedIndex = (int)std::round(currentIndex) % N;
                if (selectedIndex < 0) selectedIndex += N;
                targetIndex = selectedIndex;
            }
        }
        else {
            currentIndex = WrapFloat(currentIndex, (float)N);
            slideStartIdx = currentIndex;
            slideTimer = 0.0f;
        }

        // レーティング詳細表示のスライドインアニメーション（下から上にスライド）
        if (ratingDetailAnimationTimer > 0.0f) {
            // イージング適用（EaseOutExpoでシュッと出てくる感じに）
            float easedTimer = (float)Easing(ratingDetailAnimationTimer, EASE::EaseOutExpo);
            
            // スライドインのオフセット計算（下から）
            // 画面外（下）から定位置（0）へ移動
            const float startOffsetY = -1080.0f;
            float currentOffsetY = startOffsetY * (1.0f - easedTimer);
            
            // X座標の位置調整（左に詰める： -550 -> -750 -> -650）
            const float uiX = -650.0f;

            // 背景の更新
            if (ratingDetailBackground) {
                // 位置を更新（Xは固定、Yはスライド、Zはそのまま）
                ratingDetailBackground->transform.SetPosition(Vector3(uiX, 0.0f + currentOffsetY, 5.0f));
                // スケールは固定（最大サイズ）
                ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f));
                ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.50f * easedTimer});
                ratingDetailBackground->SetVisible(true);
            }
            
            // ラベルの更新
            if (ratingDetailLabelText) {
                // 定位置(460) + オフセット
                ratingDetailLabelText->transform.SetPosition(Vector3(uiX, 460.0f + currentOffsetY, 10.0f));
                ratingDetailLabelText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer});
                ratingDetailLabelText->SetVisible(true);
            }

            // 合計レートの更新
            if (ratingDetailTotalText) {
                // 定位置(380) + オフセット
                ratingDetailTotalText->transform.SetPosition(Vector3(uiX, 380.0f + currentOffsetY, 10.0f));
                ratingDetailTotalText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer}); // シアンで目立たせる
                ratingDetailTotalText->SetVisible(true);
            }
            
            // タイトルの更新
            if (ratingDetailTitle) {
                // 定位置(280) + オフセット
                ratingDetailTitle->transform.SetPosition(Vector3(uiX, 280.0f + currentOffsetY, 10.0f));
                ratingDetailTitle->material.SetColor({1.0f, 1.0f, 0.0f, easedTimer});
                ratingDetailTitle->SetVisible(true);
            }
            
            // リストの更新
            int lineIndex = 0;
            for (auto* line : ratingDetailLines) {
                if (line) {
                    // 定位置(200 - i*70) + オフセット
                    float baseLineY = 200.0f - lineIndex * 70.0f;
                    line->transform.SetPosition(Vector3(uiX, baseLineY + currentOffsetY, 10.0f));
                    line->material.SetColor({1.0f, 1.0f, 1.0f, easedTimer});
                    
                    // カウントに基づいて表示制御
                    if (lineIndex < ratingDetailItemCount) {
                        line->SetVisible(true);
                    } else {
                        line->SetVisible(false);
                    }
                }
                lineIndex++;
            }
        } else {
            // アニメーション終了時は非表示
            if (ratingDetailBackground) ratingDetailBackground->SetVisible(false);
            if (ratingDetailLabelText) ratingDetailLabelText->SetVisible(false);
            if (ratingDetailTitle) ratingDetailTitle->SetVisible(false);
            if (ratingDetailTotalText) ratingDetailTotalText->SetVisible(false);
            for (auto* line : ratingDetailLines) {
                if (line) line->SetVisible(false);
            }
        }
    }

    // ===== ジャケット配置更新 =====
    void SelectCanvas::UpdateJacketPositions() {
        if (songs.empty() || jacketPool.empty()) return;
        const int N = (int)songs.size();
        const int pool = (int)jacketPool.size();
        const int half = pool / 2;
        const int leftInt = (int)std::floor(currentIndex) - half;

        for (int i = 0; i < pool; ++i) {
            int songIdx = (leftInt + i) % N;
            if (songIdx < 0) songIdx += N;

            auto* img = jacketPool[i];
            img->material.texture = &jacketTextures[songIdx];

            float slotCenterIndex = (float)(leftInt + i);
            float dist = std::fabs(slotCenterIndex - currentIndex);
            float rel = (float)(i - half) - (currentIndex - std::floor(currentIndex));
            float x = LAYOUT_JACKET_CENTER_X + rel * BASE_SPACING;
            float y = LAYOUT_JACKET_CENTER_Y;

            float s = ScaleByDistance(dist, SCALE_CENTER, SCALE_EDGE);
            float z = DEPTH_NEAR + (DEPTH_FAR - DEPTH_NEAR) * std::min(dist / (float)half, 1.0f);

            img->transform.SetPosition(Vector3(x, y, z));
            img->transform.SetScale(Vector3(s, s, 1.0f));
            img->SetVisible(true);
        }
    }

    // ===== 枠更新 =====
    void SelectCanvas::UpdateSelectedFrame() {
        if (songs.empty() || jacketPool.empty()) {
            if (selectFrame) selectFrame->SetVisible(false);
            return;
        }
        const int half = (int)jacketPool.size() / 2;
        Vector3 midPos = jacketPool[half]->transform.GetPosition();
        selectFrame->transform.SetPosition(Vector3(midPos.x, midPos.y, 1));
        selectFrame->SetVisible(true);
    }

    // ===== 次へ =====
    void SelectCanvas::SelectNext() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        targetIndex = (targetIndex + 1) % N;
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // アーティスト更新
        if (artistText) {
            std::wstring artist = Utf8ToWstring(songs[targetIndex].artist);
            artistText->SetText(artist);
        }

        // BPM更新
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 難易度更新
        if (difficultyText) {
            std::wstring levelStr = L"LEVEL: " + std::to_wstring(songs[targetIndex].level);
            difficultyText->SetText(levelStr);
        }

        // Score & Rank Update
        if (!songs.empty() && highScoreText && rankMark) {
            auto record = app::test::ScoreManager::Instance().GetScore(songs[targetIndex].chartPath);
            if (record.highScore > 0) {
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"High Score: %d", record.highScore);
                highScoreText->SetText(scoreBuf);
                std::wstring rW(record.rank.begin(), record.rank.end());
                rankMark->SetText(rW);
            } else {
                highScoreText->SetText(L"High Score: --------");
                rankMark->SetText(L"-");
            }
        }


        // ★文字化け対策: Shift-JIS変換してログ出力
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));
        
        PlayPreview();
    }

    // ===== 前へ =====
    void SelectCanvas::SelectPrevious() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        targetIndex = (targetIndex - 1 + N) % N;
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }
        // アーティスト更新
        if (artistText) {
            std::wstring artist = Utf8ToWstring(songs[targetIndex].artist);
            artistText->SetText(artist);
        }

        // BPM更新
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 難易度更新
        if (difficultyText) {
            std::wstring levelStr = L"LEVEL: " + std::to_wstring(songs[targetIndex].level);
            difficultyText->SetText(levelStr);
        }

        // Score & Rank Update
        if (!songs.empty() && highScoreText && rankMark) {
            auto record = app::test::ScoreManager::Instance().GetScore(songs[targetIndex].chartPath);
            if (record.highScore > 0) {
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"High Score: %d", record.highScore);
                highScoreText->SetText(scoreBuf);
                std::wstring rW(record.rank.begin(), record.rank.end());
                rankMark->SetText(rW);
            } else {
                highScoreText->SetText(L"High Score: --------");
                rankMark->SetText(L"-");
            }
        }

        // ★文字化け対策
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));

        PlayPreview();
    }

    void SelectCanvas::CancelSelection() {
        if (sceneChanger.isNull()) return;

        sf::debug::Debug::Log("タイトルに遷移");

        sceneChanger->ChangeScene(TitleScene::StandbyScene());
    }

    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;
        if (sceneChanger.isNull()) return;

        const SongInfo& selected = songs[targetIndex];

        auto next = IngameScene::StandbyScene();
        if (next) {
            next->SetSelectedSong(selected);
        }

        LoadingScene::SetLoadingType(LoadingType::InGame);
        LoadingScene::SetNextSongInfo(selected);

        sceneChanger->ChangeScene(next);
    }

    const SongInfo& SelectCanvas::GetSelectedSong() const {
        if (songs.empty()) {
            static SongInfo empty = { "", "", "" };
            return empty;
        }
        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        return songs[idx];
    }

    // ===== プレビュー再生 =====
    void SelectCanvas::PlayPreview() {
        // 前の曲を止める
        previewPlayer.Stop();
        previewResource = nullptr;

        if (songs.empty()) return;

        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        const auto& info = songs[idx];

        if (info.musicPath.empty()) return;

        // パスをShift-JISに変換
        std::string sjisPath = Utf8ToShiftJis(info.musicPath);

        // 新しいリソースを作成
        previewResource = new sf::sound::SoundResource();

        // ロード失敗したら再生しない
        if (FAILED(previewResource.Target()->LoadSound(sjisPath, true))) { // true = loop
            sf::debug::Debug::Log("Preview Load Failed: " + sjisPath);
            return;
        }

        previewPlayer.SetResource(previewResource);
        previewPlayer.SetVolume(gAudioVolume.master * gAudioVolume.bgm); // 必要に応じて調整
        previewPlayer.Play(info.previewStartTime);
    }

    // ===== ジャンル変更 =====
    void SelectCanvas::ChangeGenre(int index) {
        // SongListServiceにジャンル変更を委譲
        songs = songListService.ChangeGenre(index);
        
        // 選択リセット
        targetIndex = 0;
        selectedIndex = 0;
        currentIndex = 0.0f;
        slideStartIdx = 0.0f;
        slideTimer = 0.0f;

        // タイトル等更新
        if (!songs.empty()) {
            if (songTitleText) songTitleText->SetText(Utf8ToWstring(songs[0].title));
            if (artistText)    artistText->SetText(Utf8ToWstring(songs[0].artist));
            if (bpmText)       bpmText->SetText(L"BPM: " + Utf8ToWstring(songs[0].bpm));

            // Score & Rank Update
            if (highScoreText && rankMark) {
                auto record = app::test::ScoreManager::Instance().GetScore(songs[0].chartPath);
                if (record.highScore > 0) {
                    wchar_t scoreBuf[64];
                    swprintf_s(scoreBuf, L"High Score: %d", record.highScore);
                    highScoreText->SetText(scoreBuf);
                    std::wstring rW(record.rank.begin(), record.rank.end());
                    rankMark->SetText(rW);
                } else {
                    highScoreText->SetText(L"High Score: --------");
                    rankMark->SetText(L"-");
                }
            }
        }

        // ジャンル名更新
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        int N = (int)allGenres.size();

        if (genreText && !allGenres.empty()) {
            genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));
        }

        // 前後のジャンル名更新
        if (prevGenreText && N > 0) {
            int prevIdx = (currentGenreIndex - 1 + N) % N;
            prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
        }
        if (nextGenreText && N > 0) {
            int nextIdx = (currentGenreIndex + 1) % N;
            nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
        }

        // ★修正: jacketTextures の再ロード
        jacketTextures.clear();
        jacketTextures.resize(songs.size());
        
        for (size_t i = 0; i < songs.size(); ++i) {
            bool loaded = false;
            if (!songs[i].jacketPath.empty()) {
                std::string sjisPath = Utf8ToShiftJis(songs[i].jacketPath);
                if (jacketTextures[i].LoadTextureFromFile(sjisPath)) {
                    loaded = true;
                }
            }
            if (!loaded) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }

        RebuildJacketPool();
        UpdateJacketPositions();
        
        PlayPreview();
    }

    void SelectCanvas::SelectNextGenre() {
        ChangeGenre(songListService.GetCurrentGenreIndex() + 1);
    }

    void SelectCanvas::SelectPreviousGenre() {
        ChangeGenre(songListService.GetCurrentGenreIndex() - 1);
    }

} // namespace app::test
