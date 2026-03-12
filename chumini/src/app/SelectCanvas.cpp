#include "SelectCanvas.h"
#include "SelectScene.h"
#include "sf/Time.h"
#include <algorithm>
#include <cmath>
#include "IngameScene.h"
#include "TitleScene.h"
#include "Easing.h"
#include "Config.h"

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
#include "StringUtils.h"

namespace app::test {

    using sf::util::WstringToUtf8;
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;

    // ===== ユーティリティ関数 =====

    /// 浮動小数をN範囲でラップする（0〜Nの循環）
    static inline float WrapFloat(float x, float N) {
        float r = std::fmod(x, N);
        if (r < 0) r += N;
        return r;
    }

    /// 円環上での最短距離を計算する
    static inline float ShortestDeltaOnRing(float current, float target, float N) {
        float c = WrapFloat(current, N);
        float diff = target - c;
        if (diff > N * 0.5f) diff -= N;
        if (diff < -N * 0.5f) diff += N;
        return diff;
    }

    /// 距離に応じたスケール値を計算する（ガウス減衰）
    float SelectCanvas::ScaleByDistance(float d, float scaleCenter, float scaleEdge) {
        const float falloff = std::exp(-0.5f * (d * d));
        return scaleEdge + (scaleCenter - scaleEdge) * falloff;
    }

    // ===== 初期化 =====

    /// キャンバスの初期化 - テクスチャ・楽曲・UIを構築し、初期選択状態を設定する
    void SelectCanvas::Begin() {
        sf::ui::Canvas::Begin();

        // シーンチェンジコンポーネントの取得
        if (auto actor = actorRef.Target()) {
            auto* changer = actor->GetComponent<SceneChangeComponent>();
            if (changer) {
                sceneChanger = changer;
            }
        }

        InitializeTextures();
        InitializeSongs();
        InitializeUI();

        // 選択インデックスの初期化
        selectedIndex = std::clamp(selectedIndex, 0, (int)songs.size() - 1);
        targetIndex = selectedIndex;
        currentIndex = (float)targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 曲情報テキストの初期表示
        if (songTitleText && !songs.empty()) {
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

        // スコア・ランクの初期表示
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

        // ジャンル名の初期表示
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        if (!allGenres.empty()) {
            if(genreText) genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));

            int N = (int)allGenres.size();
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

    /// テクスチャリソースを読み込む
    void SelectCanvas::InitializeTextures() {
        textureBack.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        textureSelectFrame.LoadTextureFromFile("Assets\\Texture\\SelectFrame.png");
        textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\Jacket\\DefaultJacket.png");
        textureTitlePanel.LoadTextureFromFile("Assets\\Texture\\songselect.png");
        textureCC.LoadTextureFromFile("Assets\\Texture\\CC.png");
    }

    /// 楽曲データの初期化 - SongListServiceにスキャンを委譲し、ジャケットテクスチャを読み込む
    void SelectCanvas::InitializeSongs() {
        songListService.ScanSongs("Songs");
        songs = songListService.GetCurrentSongs();

        // ジャケットテクスチャの読み込み
        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            bool loaded = false;
            if (!songs[i].jacketPath.empty()) {
                // パスをShift-JISに変換して読み込み
                std::string sjisPath = Utf8ToShiftJis(songs[i].jacketPath);

                if (jacketTextures[i].LoadTextureFromFile(sjisPath)) {
                    loaded = true;
                }
                else {
                    sf::debug::Debug::Log("Jacket Load Failed: " + sjisPath);
                }
            }

            // 読み込み失敗時はデフォルトジャケットを使用
            if (!loaded) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    /// UI要素の生成 - テキスト・ジャケット・レーティング詳細パネルを構築する
    void SelectCanvas::InitializeUI() {

        auto* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();

        // 選択フレーム
        selectFrame = AddUI<sf::ui::Image>();
        selectFrame->transform.SetPosition(Vector3(LAYOUT_JACKET_CENTER_X, LAYOUT_JACKET_CENTER_Y, 1));
        selectFrame->transform.SetScale(Vector3(10.0f, 50.0f, 1.0f));
        selectFrame->material.texture = &textureSelectFrame;
        selectFrame->material.SetColor({ 1, 1, 1, 0.8f });

        // ジャケットUIコンポーネントの生成
        jacketComponents.clear();
        for(int i = 0; i < MAX_VISIBLE; ++i) {
             auto jacket = AddUI<sf::ui::Image>();
             jacket->transform.SetScale(Vector3(SCALE_EDGE, SCALE_EDGE, 1.0f));
             jacket->SetVisible(false);
             jacketComponents.push_back(jacket);
        }

        RebuildJacketPool();
        UpdateJacketPositions();

     // ===== ヘッダーテキスト「SONG SELECT」（アウトライン付き） =====
        float outlineSize = 3.0f;

        // アウトライン用オフセット（上下左右4方向）
        Vector3 offsets[4] = {
            Vector3(-outlineSize, 0, 0),
            Vector3(outlineSize, 0, 0),
            Vector3(0, -outlineSize, 0),
            Vector3(0,  outlineSize, 0)
        };

        // アウトラインテキストを4つ生成
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

        // ===== ヘッダーテキスト本体 =====
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

        // ===== 曲情報テキスト =====

        // 曲タイトル
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

        // アーティスト名
        artistText = AddUI<sf::ui::TextImage>();
        artistText->transform.SetPosition(Vector3(0, LAYOUT_ARTIST_Y, LAYOUT_SONG_INFO_Z));
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray), 
            1024, 200
        );

        // BPM表示
        bpmText = AddUI<sf::ui::TextImage>();
        bpmText->transform.SetPosition(Vector3(0, LAYOUT_BPM_Y, LAYOUT_SONG_INFO_Z));
        bpmText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        bpmText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray), 
            1024, 200
        );

        // 難易度表示
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

        // ===== ジャンルテキスト =====

        // 現在のジャンル名
        genreText = AddUI<sf::ui::TextImage>();
        genreText->transform.SetPosition(Vector3(0, LAYOUT_GENRE_Y, 0)); 
        genreText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        genreText->Create(
            device,
            L"", 
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f,
            D2D1::ColorF(D2D1::ColorF::Yellow),
            1024, 200
        );

        // 前のジャンル名（左配置）
        prevGenreText = AddUI<sf::ui::TextImage>();
        prevGenreText->transform.SetPosition(Vector3(-550.0f, LAYOUT_GENRE_Y, 0));
        prevGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f));
        prevGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // 次のジャンル名（右配置）
        nextGenreText = AddUI<sf::ui::TextImage>();
        nextGenreText->transform.SetPosition(Vector3(550.0f, LAYOUT_GENRE_Y, 0));
        nextGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f));
        nextGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // ===== スコア・ランク表示 =====

        // ハイスコアテキスト
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

        // ランクマーク
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

        // ===== プレイヤーレーティング表示 =====
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

        // レーティング値を小数点2桁で表示
        float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
        float truncatedRating = std::floor(playerRating * 100.0f) / 100.0f;
        wchar_t ratingBuf[64];
        swprintf_s(ratingBuf, L"Rating: %.2f", truncatedRating);
        playerRatingText->SetText(ratingBuf);

        // ===== レーティング詳細パネル =====

        // 背景テクスチャ
        ratingDetailBackgroundTexture.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        
        // 半透明背景
        ratingDetailBackground = AddUI<sf::ui::Image>();
        ratingDetailBackground->transform.SetPosition(Vector3(-550, 0, 5.0f));
        ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f));
        ratingDetailBackground->material.texture = &ratingDetailBackgroundTexture;
        ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.90f});
        ratingDetailBackground->SetVisible(false);

        // 「RATING」ラベル
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

        // レーティング合計値テキスト
        ratingDetailTotalText = AddUI<sf::ui::TextImage>();
        ratingDetailTotalText->transform.SetPosition(Vector3(-650, 380, 10.0f));
        ratingDetailTotalText->transform.SetScale(Vector3(10.0f, 3.0f, 1.0f));
        ratingDetailTotalText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 200
        );
        ratingDetailTotalText->SetVisible(false);

        // 「Top 10 Charts」タイトル
        ratingDetailTitle = AddUI<sf::ui::TextImage>();
        ratingDetailTitle->transform.SetPosition(Vector3(-650, 280, 10.0f));
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

        // 上位チャート詳細行（10行）
        for (int i = 0; i < 10; ++i) {
            auto* line = AddUI<sf::ui::TextImage>();
            line->transform.SetPosition(Vector3(-650, 200 - i * 70, 10.0f));
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

    /// ジャケット表示プールを再構築する
    void SelectCanvas::RebuildJacketPool() {
        // 全ジャケットを一旦非表示
        for (auto* img : jacketComponents) {
            if (img) img->SetVisible(false);
        }
        jacketPool.clear();

        if (songs.empty()) return;

        // 表示に必要なプールサイズを決定
        const int poolSize = std::min<int>(MAX_VISIBLE, std::max<int>(3, (int)songs.size()));
        
        // プールにジャケットを登録
        for (int i = 0; i < poolSize; ++i) {
            if (i < (int)jacketComponents.size()) {
                 auto* jacket = jacketComponents[i];
                 jacket->SetVisible(true);
                 jacketPool.push_back(jacket);
            }
        }
    }

    // ===== 更新処理 =====

    /// 毎フレーム更新 - アニメーション・ジャケット位置・フレーム位置を更新する
    void SelectCanvas::Update(const sf::command::ICommand& command) {
        animationTime += sf::Time::DeltaTime();

        // レーティング詳細パネルのアニメーション進行
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

    /// 選択スライドアニメーション・モード別のビジュアル状態を更新する
    void SelectCanvas::UpdateAnimation() {

        // 選択枠の呼吸アニメーション
        const float pulsePhase = std::fmod(animationTime * 0.8f, 1.0f);
        const float pulse = (float)Easing(pulsePhase, EASE::EaseYoyo);
        const float pulseScale = 1.0f + 0.04f * pulse;

        // ===== モード別のビジュアル更新 =====
        if (selectionMode == InputMode::GenreSelect) {
            // ジャンル選択モード：ジャンル名を点滅、曲エリアをグレーアウト
            float blink = 0.5f + 0.5f * (std::sin(animationTime * 8.0f) + 1.0f) * 0.5f;
            if (genreText) {
                genreText->material.SetColor({ 1.0f, 1.0f, 0.0f, blink });
            }
            if (prevGenreText) prevGenreText->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
            if (nextGenreText) nextGenreText->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });

            // 曲エリアのグレーアウト
            if (selectFrame) selectFrame->material.SetColor({ 0.3f, 0.3f, 0.3f, 0.5f });
            
            for (auto* jacket : jacketPool) {
                if(jacket) jacket->material.SetColor({ 0.4f, 0.4f, 0.4f, 1.0f });
            }
            if(songTitleText) songTitleText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(artistText) artistText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(bpmText) bpmText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});
            if(difficultyText) difficultyText->material.SetColor({0.5f, 0.5f, 0.5f, 1.0f});

        } else {
            // 曲選択モード：曲エリアをアクティブ表示
            if (genreText) {
                 genreText->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            }
            if (prevGenreText) prevGenreText->material.SetColor({ 0.4f, 0.4f, 0.4f, 0.8f });
            if (nextGenreText) nextGenreText->material.SetColor({ 0.4f, 0.4f, 0.4f, 0.8f });

            // 選択枠の呼吸アニメーション
           if (selectFrame)
            {
                float alpha = 0.5f + 0.5f * pulse;
                selectFrame->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
            }

            // ジャケットを通常色で表示
            for (auto* jacket : jacketPool) {
                if(jacket) jacket->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            }
             if(songTitleText) songTitleText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(artistText) artistText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(bpmText) bpmText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
             if(difficultyText) difficultyText->material.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
        }
        
        // 選択枠のスケールを中央ジャケットに合わせて呼吸させる
        if (selectFrame && !jacketPool.empty()) {
            const int half = (int)jacketPool.size() / 2;
            const Vector3 jacketScale = jacketPool[half]->transform.GetScale();

            selectFrame->transform.SetScale(Vector3(
                jacketScale.x * 1.05f * pulseScale,
                jacketScale.y * 1.05f * pulseScale,
                1.0f
            ));
        }

        // ===== スライド補間処理 =====
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

        // ===== レーティング詳細パネルのスライドアニメーション =====
        if (ratingDetailAnimationTimer > 0.0f) {
            float easedTimer = (float)Easing(ratingDetailAnimationTimer, EASE::EaseOutExpo);
            
            // 下からスライドインするオフセット
            const float startOffsetY = -1080.0f;
            float currentOffsetY = startOffsetY * (1.0f - easedTimer);
            
            const float uiX = -650.0f;

            // 背景
            if (ratingDetailBackground) {
                ratingDetailBackground->transform.SetPosition(Vector3(uiX, 0.0f + currentOffsetY, 5.0f));
                ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f));
                ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.50f * easedTimer});
                ratingDetailBackground->SetVisible(true);
            }
            
            // 「RATING」ラベル
            if (ratingDetailLabelText) {
                ratingDetailLabelText->transform.SetPosition(Vector3(uiX, 460.0f + currentOffsetY, 10.0f));
                ratingDetailLabelText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer});
                ratingDetailLabelText->SetVisible(true);
            }

            // レーティング合計値
            if (ratingDetailTotalText) {
                ratingDetailTotalText->transform.SetPosition(Vector3(uiX, 380.0f + currentOffsetY, 10.0f));
                ratingDetailTotalText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer});
                ratingDetailTotalText->SetVisible(true);
            }
            
            // 「Top 10 Charts」タイトル
            if (ratingDetailTitle) {
                ratingDetailTitle->transform.SetPosition(Vector3(uiX, 280.0f + currentOffsetY, 10.0f));
                ratingDetailTitle->material.SetColor({1.0f, 1.0f, 0.0f, easedTimer});
                ratingDetailTitle->SetVisible(true);
            }
            
            // 各チャート詳細行
            int lineIndex = 0;
            for (auto* line : ratingDetailLines) {
                if (line) {
                    float baseLineY = 200.0f - lineIndex * 70.0f;
                    line->transform.SetPosition(Vector3(uiX, baseLineY + currentOffsetY, 10.0f));
                    line->material.SetColor({1.0f, 1.0f, 1.0f, easedTimer});
                    
                    // データがある行のみ表示
                    line->SetVisible(lineIndex < ratingDetailItemCount);
                }
                lineIndex++;
            }
        } else {
            // パネル閉鎖時は全要素を非表示
            if (ratingDetailBackground) ratingDetailBackground->SetVisible(false);
            if (ratingDetailLabelText) ratingDetailLabelText->SetVisible(false);
            if (ratingDetailTitle) ratingDetailTitle->SetVisible(false);
            if (ratingDetailTotalText) ratingDetailTotalText->SetVisible(false);
            for (auto* line : ratingDetailLines) {
                if (line) line->SetVisible(false);
            }
        }
    }

    /// ジャケットの位置・スケール・深度を現在の選択位置に基づいて更新する
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

    /// 選択枠を中央ジャケットの位置に配置する
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

    // ===== 曲選択操作 =====

    /// 次の曲を選択し、曲情報UIを更新する
    void SelectCanvas::SelectNext() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        targetIndex = (targetIndex + 1) % N;
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 曲タイトル更新
        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // アーティスト名更新
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

        // スコア・ランク更新
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

        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));
        
        PlayPreview();
    }

    /// 前の曲を選択し、曲情報UIを更新する
    void SelectCanvas::SelectPrevious() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        targetIndex = (targetIndex - 1 + N) % N;
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 曲タイトル更新
        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // アーティスト名更新
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

        // スコア・ランク更新
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

        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));

        PlayPreview();
    }

    /// 選択をキャンセルしてタイトル画面に戻る
    void SelectCanvas::CancelSelection() {
        if (presenter) {
            presenter->NavigateToTitle();
        }
    }

    /// 選択を確定してゲーム画面に遷移する
    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;

        const SongInfo& selected = songs[targetIndex];

        if (presenter) {
            presenter->NavigateToGame(selected);
        }
    }

    /// 現在選択中の楽曲情報を返す
    const SongInfo& SelectCanvas::GetSelectedSong() const {
        if (songs.empty()) {
            static SongInfo empty = { "", "", "" };
            return empty;
        }
        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        return songs[idx];
    }

    /// 指定インデックスの曲を選択状態にし、UIを更新する
    void SelectCanvas::SetTargetIndex(int index) {
        if (songs.empty()) return;
        const int N = (int)songs.size();
        targetIndex = std::clamp(index, 0, N - 1);
        selectedIndex = targetIndex;
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 曲情報UIの更新
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

        // スコア・ランク更新
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

    /// ジャンル選択モードの切り替え
    void SelectCanvas::SetGenreSelectMode(bool isGenreMode) {
        selectionMode = isGenreMode ? InputMode::GenreSelect : InputMode::SongSelect;
    }

    /// レーティング詳細パネルの表示切り替え
    void SelectCanvas::SetShowRatingDetail(bool show) {
        showRatingDetail = show;
        
        if (show && ratingDetailTotalText) {
            // レーティング合計値を更新
            float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
            float truncated = std::floor(playerRating * 100.0f) / 100.0f;
            wchar_t totalBuf[64];
            swprintf_s(totalBuf, L"%.2f", truncated);
            ratingDetailTotalText->SetText(totalBuf);
            
            // 上位チャートの詳細行を更新
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

    /// 選択中の楽曲のプレビュー音声を再生する
    void SelectCanvas::PlayPreview() {
        previewPlayer.Stop();
        previewResource = nullptr;

        if (songs.empty()) return;

        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        const auto& info = songs[idx];

        if (info.musicPath.empty()) return;

        // パスをShift-JISに変換
        std::string sjisPath = Utf8ToShiftJis(info.musicPath);

        // サウンドリソースを生成して読み込み
        previewResource = new sf::sound::SoundResource();

        if (FAILED(previewResource.Target()->LoadSound(sjisPath, true))) {
            sf::debug::Debug::Log("Preview Load Failed: " + sjisPath);
            return;
        }

        previewPlayer.SetResource(previewResource);
        previewPlayer.SetVolume(gAudioVolume.master * gAudioVolume.bgm);
        previewPlayer.Play(info.previewStartTime);
    }

    // ===== ジャンル変更 =====

    /// ジャンルを変更し、楽曲リスト・ジャケット・UIを再構築する
    void SelectCanvas::ChangeGenre(int direction) {
        int currentIdx = songListService.GetCurrentGenreIndex();
        int newIdx = currentIdx + direction;
        songs = songListService.ChangeGenre(newIdx);
        
        // 選択位置をリセット
        targetIndex = 0;
        selectedIndex = 0;
        currentIndex = 0.0f;
        slideStartIdx = 0.0f;
        slideTimer = 0.0f;

        // 曲情報UIの更新
        if (!songs.empty()) {
            if (songTitleText) songTitleText->SetText(Utf8ToWstring(songs[0].title));
            if (artistText)    artistText->SetText(Utf8ToWstring(songs[0].artist));
            if (bpmText)       bpmText->SetText(L"BPM: " + Utf8ToWstring(songs[0].bpm));

            // スコア・ランク更新
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

        // ジャンル名表示の更新
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        int N = (int)allGenres.size();

        if (genreText && !allGenres.empty()) {
            genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));
        }

        // 前後のジャンル名を更新
        if (prevGenreText && N > 0) {
            int prevIdx = (currentGenreIndex - 1 + N) % N;
            prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
        }
        if (nextGenreText && N > 0) {
            int nextIdx = (currentGenreIndex + 1) % N;
            nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
        }

        // ジャケットテクスチャを再読み込み
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

    /// 次のジャンルに変更する
    void SelectCanvas::SelectNextGenre() {
        ChangeGenre(songListService.GetCurrentGenreIndex() + 1);
    }

    /// 前のジャンルに変更する
    void SelectCanvas::SelectPreviousGenre() {
        ChangeGenre(songListService.GetCurrentGenreIndex() - 1);
    }

} // namespace app::test
