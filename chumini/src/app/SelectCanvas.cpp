#include "SelectCanvas.h"
#include "SelectScene.h"  // MVP: Presenter
#include "sf/Time.h"
#include <algorithm>
#include <cmath>
#include "IngameScene.h"
#include "TitleScene.h"
#include "Easing.h"
#include "Config.h"

// 依存ヘッダ
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

    // ユーティリティを利用
    using sf::util::WstringToUtf8;
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;

    // ===== Utility =====
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

        // 条件分岐
        if (songTitleText && !songs.empty()) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // 条件分岐
        if (artistText) {
            artistText->SetText(Utf8ToWstring(songs[targetIndex].artist));
        }

        // 条件分岐
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 条件分岐
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

        // 楽曲リスト情報を更新
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

    // 処理本体
    void SelectCanvas::InitializeTextures() {
        textureBack.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        textureSelectFrame.LoadTextureFromFile("Assets\\Texture\\SelectFrame.png");
        textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\Jacket\\DefaultJacket.png");
        textureTitlePanel.LoadTextureFromFile("Assets\\Texture\\songselect.png");
        textureCC.LoadTextureFromFile("Assets\\Texture\\CC.png");
    }

    // 処理本体
    void SelectCanvas::InitializeSongs() {
        // SongListServiceにスキャンを委譲
        songListService.ScanSongs("Songs");
        songs = songListService.GetCurrentSongs();

        // ----------------------------------------------------
        // ジャケット表示を更新
        // ----------------------------------------------------
        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            bool loaded = false;
            if (!songs[i].jacketPath.empty()) {

                // ジャケット表示を更新
                // ジャケット表示を更新
                std::string sjisPath = Utf8ToShiftJis(songs[i].jacketPath);

                if (jacketTextures[i].LoadTextureFromFile(sjisPath)) {
                    loaded = true;
                }
                else {
                    // ジャケット表示を更新
                    sf::debug::Debug::Log("Jacket Load Failed: " + sjisPath);
                }
            }

            if (!loaded) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    // 処理本体
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

        // ジャケット表示を更新
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
     // 処理本体
     // =========================================================

     // 処理本体
        float outlineSize = 3.0f;

        // 処理本体
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
        // UI要素を生成
        // =========================================================
        titleText = AddUI<sf::ui::TextImage>();
        titleText->transform.SetPosition(Vector3(0, LAYOUT_TITLE_Y, 0.0f));
        titleText->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

        titleText->Create(
            device,
            L"SONG SELECT",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::DeepSkyBlue),
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
    // UI要素を生成
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
        // BPM陦ｨ遉ｺ
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
        difficultyText->transform.SetPosition(Vector3(650, -300, 1.0f));
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
        // UI要素を生成
        // ---------------------------------------------------------
        genreText = AddUI<sf::ui::TextImage>();
        // 位置を更新
        genreText->transform.SetPosition(Vector3(0, LAYOUT_GENRE_Y, 0)); 
        genreText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        genreText->Create(
            device,
            L"", 
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f, // 少し大きめ
            D2D1::ColorF(D2D1::ColorF::Yellow), // 逶ｮ遶九▽濶ｲ
            1024, 200
        );

        // UI要素を生成
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

        // UI要素を生成
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

        // UI要素を生成
        // UI要素を生成
        highScoreText = AddUI<sf::ui::TextImage>();
        highScoreText->transform.SetPosition(Vector3(650, -400, 1.0f)); 
        highScoreText->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f));
        highScoreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            40.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1536, 128
        );

        // UI要素を生成
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


        // UI要素を生成
        playerRatingText = AddUI<sf::ui::TextImage>();
        playerRatingText->transform.SetPosition(Vector3(-750, -450, 1.0f));
        playerRatingText->transform.SetScale(Vector3(6.0f, 1.5f, 1.0f));
        playerRatingText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 128
        );

        // 処理本体
        float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
        // 処理本体
        float truncatedRating = std::floor(playerRating * 100.0f) / 100.0f;
        wchar_t ratingBuf[64];
        swprintf_s(ratingBuf, L"Rating: %.2f", truncatedRating);
        playerRatingText->SetText(ratingBuf);

        // テクスチャを読み込む
        // テクスチャを読み込む
        ratingDetailBackgroundTexture.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        
        // UI要素を生成
        ratingDetailBackground = AddUI<sf::ui::Image>();
        ratingDetailBackground->transform.SetPosition(Vector3(-550, 0, 5.0f));
        ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f)); // 10倍に拡大
        ratingDetailBackground->material.texture = &ratingDetailBackgroundTexture;
        ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.90f}); // 半透明の黒（90%）
        ratingDetailBackground->SetVisible(false);

        // "RATING" ラベル（最上段）
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

        // UI要素を生成
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

        // UI要素を生成
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

        // ループ処理
        for (int i = 0; i < 10; ++i) {
            auto* line = AddUI<sf::ui::TextImage>();
            line->transform.SetPosition(Vector3(-650, 200 - i * 70, 10.0f)); // 220 -> 200
            line->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f));
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

    // ジャケット表示を更新
    void SelectCanvas::RebuildJacketPool() {
        // 全て一旦非表示
        for (auto* img : jacketComponents) {
            if (img) img->SetVisible(false);
        }
        jacketPool.clear();

        if (songs.empty()) return;

        // 処理本体
        const int poolSize = std::min<int>(MAX_VISIBLE, std::max<int>(3, (int)songs.size()));
        
        // ループ処理
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

        // 条件分岐
        if (showRatingDetail && ratingDetailAnimationTimer < 1.0f) {
            ratingDetailAnimationTimer += sf::Time::DeltaTime() / RATING_DETAIL_ANIMATION_DURATION;
            if (ratingDetailAnimationTimer > 1.0f) ratingDetailAnimationTimer = 1.0f;
        } else if (!showRatingDetail && ratingDetailAnimationTimer > 0.0f) {
            ratingDetailAnimationTimer -= sf::Time::DeltaTime() / RATING_DETAIL_ANIMATION_DURATION;
            if (ratingDetailAnimationTimer < 0.0f) ratingDetailAnimationTimer = 0.0f;
        }

        UpdateAnimation();
        UpdateJacketPositions();
        UpdateSelectedFrame();
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

        // 条件分岐
        if (ratingDetailAnimationTimer > 0.0f) {
            // レーティング詳細UIを更新
            float easedTimer = (float)Easing(ratingDetailAnimationTimer, EASE::EaseOutExpo);
            
            // 処理本体
            // 処理本体
            const float startOffsetY = -1080.0f;
            float currentOffsetY = startOffsetY * (1.0f - easedTimer);
            
            // 処理本体
            const float uiX = -650.0f;

            // 背景の更新
            if (ratingDetailBackground) {
                // 位置を更新
                ratingDetailBackground->transform.SetPosition(Vector3(uiX, 0.0f + currentOffsetY, 5.0f));
                // スケールを更新
                ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f));
                ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.50f * easedTimer});
                ratingDetailBackground->SetVisible(true);
            }
            
            // ラベルの更新
            if (ratingDetailLabelText) {
                // 位置を更新
                ratingDetailLabelText->transform.SetPosition(Vector3(uiX, 460.0f + currentOffsetY, 10.0f));
                ratingDetailLabelText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer});
                ratingDetailLabelText->SetVisible(true);
            }

            // 合計レートの更新
            if (ratingDetailTotalText) {
                // 位置を更新
                ratingDetailTotalText->transform.SetPosition(Vector3(uiX, 380.0f + currentOffsetY, 10.0f));
                ratingDetailTotalText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer}); // シアンで目立たせる
                ratingDetailTotalText->SetVisible(true);
            }
            
            // タイトルの更新
            if (ratingDetailTitle) {
                // 位置を更新
                ratingDetailTitle->transform.SetPosition(Vector3(uiX, 280.0f + currentOffsetY, 10.0f));
                ratingDetailTitle->material.SetColor({1.0f, 1.0f, 0.0f, easedTimer});
                ratingDetailTitle->SetVisible(true);
            }
            
            // 処理本体
            int lineIndex = 0;
            for (auto* line : ratingDetailLines) {
                if (line) {
                    // 処理本体
                    float baseLineY = 200.0f - lineIndex * 70.0f;
                    line->transform.SetPosition(Vector3(uiX, baseLineY + currentOffsetY, 10.0f));
                    line->material.SetColor({1.0f, 1.0f, 1.0f, easedTimer});
                    
                    // 条件分岐
                    if (lineIndex < ratingDetailItemCount) {
                        line->SetVisible(true);
                    } else {
                        line->SetVisible(false);
                    }
                }
                lineIndex++;
            }
        } else {
            // 条件分岐
            if (ratingDetailBackground) ratingDetailBackground->SetVisible(false);
            if (ratingDetailLabelText) ratingDetailLabelText->SetVisible(false);
            if (ratingDetailTitle) ratingDetailTitle->SetVisible(false);
            if (ratingDetailTotalText) ratingDetailTotalText->SetVisible(false);
            for (auto* line : ratingDetailLines) {
                if (line) line->SetVisible(false);
            }
        }
    }

    // ジャケット表示を更新
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

    // ===== 選択枠更新 =====
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

        // 条件分岐
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


        // デバッグログを出力
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));
        
        PlayPreview();
    }

    // ===== 蜑阪∈ =====
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
        // 条件分岐
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

        // デバッグログを出力
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));

        PlayPreview();
    }

    void SelectCanvas::CancelSelection() {
        // MVP: Presenterにシーン遷移を委譲
        if (presenter) {
            presenter->NavigateToTitle();
        }
    }

    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;

        const SongInfo& selected = songs[targetIndex];

        // MVP: Presenterにシーン遷移を委譲
        if (presenter) {
            presenter->NavigateToGame(selected);
        }
    }

    const SongInfo& SelectCanvas::GetSelectedSong() const {
        if (songs.empty()) {
            static SongInfo empty = { "", "", "" };
            return empty;
        }
        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        return songs[idx];
    }

    // ===== MVP: SetTargetIndex =====
    void SelectCanvas::SetTargetIndex(int index) {
        if (songs.empty()) return;
        const int N = (int)songs.size();
        targetIndex = std::clamp(index, 0, N - 1);
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // UI更新
        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }
        if (artistText) {
            artistText->SetText(Utf8ToWstring(songs[targetIndex].artist));
        }
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }
        if (difficultyText) {
            std::wstring levelStr = L"LEVEL: " + std::to_wstring(songs[targetIndex].level);
            difficultyText->SetText(levelStr);
        }
        // Score & Rank 更新
        if (highScoreText && rankMark) {
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
        PlayPreview();
    }

    // ===== MVP: SetGenreSelectMode =====
    void SelectCanvas::SetGenreSelectMode(bool isGenreMode) {
        selectionMode = isGenreMode ? InputMode::GenreSelect : InputMode::SongSelect;
    }

    // ===== MVP: SetShowRatingDetail =====
    void SelectCanvas::SetShowRatingDetail(bool show) {
        showRatingDetail = show;
        
        if (show && ratingDetailTotalText) {
            // 処理本体
            float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
            float truncated = std::floor(playerRating * 100.0f) / 100.0f;
            wchar_t totalBuf[64];
            swprintf_s(totalBuf, L"%.2f", truncated);
            ratingDetailTotalText->SetText(totalBuf);
            
            // 処理本体
            auto topCharts = app::test::RatingManager::Instance().GetTopCharts(10);
            ratingDetailItemCount = std::min<int>((int)topCharts.size(), (int)ratingDetailLines.size());
            
            for (size_t i = 0; i < ratingDetailLines.size(); ++i) {
                if (i < topCharts.size()) {
                    std::string title = std::get<0>(topCharts[i]);
                    float rating = std::get<1>(topCharts[i]);
                    float truncRating = std::floor(rating * 100.0f) / 100.0f;
                    wchar_t lineBuf[256];
                    std::wstring titleW = Utf8ToWstring(title);
                    swprintf_s(lineBuf, L"%d. %s - %.2f", (int)i + 1, titleW.c_str(), truncRating);
                    ratingDetailLines[i]->SetText(lineBuf);
                } else {
                    ratingDetailLines[i]->SetText(L"");
                }
            }
        }
    }

    // ===== プレビュー再生 =====
    void SelectCanvas::PlayPreview() {
        // プレビュー再生を制御
        previewPlayer.Stop();
        previewResource = nullptr;

        if (songs.empty()) return;

        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        const auto& info = songs[idx];

        if (info.musicPath.empty()) return;

        // パスをShift-JISに変換
        std::string sjisPath = Utf8ToShiftJis(info.musicPath);

        // 処理本体
        previewResource = new sf::sound::SoundResource();

        // 条件分岐
        if (FAILED(previewResource.Target()->LoadSound(sjisPath, true))) { // true = loop
            sf::debug::Debug::Log("Preview Load Failed: " + sjisPath);
            return;
        }

        previewPlayer.SetResource(previewResource);
        previewPlayer.SetVolume(gAudioVolume.master * gAudioVolume.bgm);
        previewPlayer.Play(info.previewStartTime);
    }

    // ===== ジャンル変更 =====
    void SelectCanvas::ChangeGenre(int direction) {
        // 楽曲リスト情報を更新
        int currentIdx = songListService.GetCurrentGenreIndex();
        int newIdx = currentIdx + direction;
        // 楽曲リスト情報を更新
        songs = songListService.ChangeGenre(newIdx);
        
        // 処理本体
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

        // 条件分岐
        if (prevGenreText && N > 0) {
            int prevIdx = (currentGenreIndex - 1 + N) % N;
            prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
        }
        if (nextGenreText && N > 0) {
            int nextIdx = (currentGenreIndex + 1) % N;
            nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
        }

        // ジャケット表示を更新
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
