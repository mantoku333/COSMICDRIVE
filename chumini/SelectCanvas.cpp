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

namespace app::test {

    // 既存のヘルパー
    static std::string WstringToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    static std::wstring Utf8ToWstring(const std::string& str) {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
        return wstr;
    }

    // UTF-8 -> Shift-JIS (ログ表示 & 古いAPI用) 
    // これを通さないと、日本語パスの画像読み込みやログ出力が失敗します
    static std::string Utf8ToShiftJis(const std::string& utf8Str) {
        // 1. UTF-8 -> Unicode (wstring)
        std::wstring wstr = Utf8ToWstring(utf8Str);
        if (wstr.empty()) return "";

        // 2. Unicode -> Shift-JIS (CP_ACP)
        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);

        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

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
        songs.clear();
        allGenres.clear();
        namespace fs = std::filesystem;

        std::string rootPath = "Songs";

        if (fs::exists(rootPath) && fs::is_directory(rootPath)) {
            // 1. rootPath直下のフォルダを「ジャンル」として扱う
            for (const auto& genreEntry : fs::directory_iterator(rootPath)) {
                if (!genreEntry.is_directory()) continue;

                Genre newGenre;
                // フォルダ名をジャンル名とする
                newGenre.name = genreEntry.path().filename().string();

                // 2. そのジャンルフォルダ内を再帰的に走査して曲を探す
                for (const auto& fileEntry : fs::recursive_directory_iterator(genreEntry.path())) {
                    if (!fileEntry.is_regular_file()) continue;

                    std::wstring ext = fileEntry.path().extension().wstring();
                    if (ext == L".sus" || ext == L".ched") {
                         app::test::ChedParser parser;
                         if (parser.Load(fileEntry.path().wstring(), true)) {
                             SongInfo info;
                             info.title = parser.title.empty() ? "No Title" : parser.title;
                             info.artist = parser.artist.empty() ? "No Artist" : parser.artist;
                             info.chartPath = WstringToUtf8(fileEntry.path().wstring());
                             
                             fs::path parentDir = fileEntry.path().parent_path();
                             
                             // ジャケット
                             if (!parser.jacketFile.empty()) {
                                 std::wstring jacketW = Utf8ToWstring(parser.jacketFile);
                                 fs::path fullJacketPath = parentDir / jacketW;
                                 info.jacketPath = WstringToUtf8(fullJacketPath.wstring());
                             } else {
                                 info.jacketPath = "";
                             }

                             // 音声
                             if (!parser.waveFile.empty()) {
                                 std::wstring waveW = Utf8ToWstring(parser.waveFile);
                                 fs::path fullWavePath = parentDir / waveW;
                                 info.musicPath = WstringToUtf8(fullWavePath.wstring());
                             }

                             // プレビュー開始位置 (.pv check)
                             fs::path pvPath = fileEntry.path();
                             pvPath.replace_extension(".pv");
                             if (fs::exists(pvPath)) {
                                 std::ifstream ifs(pvPath);
                                 if (ifs) {
                                     float startTime = 0.0f;
                                     ifs >> startTime;
                                     info.previewStartTime = startTime;
                                 }
                             }

                             info.bpm = std::to_string(parser.bpm);
                             info.level = parser.level;
                             info.difficulty = parser.difficulty;
                             newGenre.songs.push_back(info);
                         }
                    }
                }

                // 曲が1つでもあればジャンルとして登録
                if (!newGenre.songs.empty()) {
                    allGenres.push_back(newGenre);
                    sf::debug::Debug::Log("Genre Loaded: " + newGenre.name + " (" + std::to_string(newGenre.songs.size()) + " songs)");
                }
            }
        }
        else {
            sf::debug::Debug::Log("Songs directory not found: " + rootPath);
        }

        // 3. 最初のジャンルを選択
        if (allGenres.empty()) {
            // ダミージャンル
            Genre dummy;
            dummy.name = "No Genre";
            dummy.songs.push_back({ "No Music Found", "System", "", "", "", "0" });
            allGenres.push_back(dummy);
        }

        currentGenreIndex = 0;
        songs = allGenres[currentGenreIndex].songs;

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
     // ★ 1. ふち（アウトライン）の生成
     // =========================================================

     // ふちの太さ（3.0f くらいが丁度いいです）
        float outlineSize = 3.0f;

        // 4方向のズレを定義（左、右、下、上）
        Vector3 offsets[4] = {
            Vector3(-outlineSize, 0, 0),
            Vector3(outlineSize, 0, 0),
            Vector3(0, -outlineSize, 0),
            Vector3(0,  outlineSize, 0)
        };

        // ループして4つ作る
        for (int i = 0; i < 4; ++i) {
            titleOutline[i] = AddUI<sf::ui::TextImage>();

            // 座標：メインの位置(0, 480) から少しズラす
            // Z座標：メインより奥に見せたいので 1.0f に設定
            titleOutline[i]->transform.SetPosition(Vector3(0 + offsets[i].x, LAYOUT_TITLE_Y + offsets[i].y, 1.0f));

            // スケール：メインと全く同じにする（ここ重要！）
            titleOutline[i]->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

            titleOutline[i]->Create(
                device,
                L"SONG SELECT",         // 同じ文字
                L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",     // 同じフォント
                120.0f,                 // 同じサイズ
                D2D1::ColorF(D2D1::ColorF::White),
                1024, 240
            );
        }

        // =========================================================
        // ★ 2. メイン文字（本体）の生成
        // =========================================================
        titleText = AddUI<sf::ui::TextImage>();

        // 座標：中心 (0, 480), Z座標は手前 (0.0f)
        titleText->transform.SetPosition(Vector3(0, LAYOUT_TITLE_Y, 0.0f));

        // スケール
        titleText->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

        titleText->Create(
            device,
            L"SONG SELECT",
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1800, 200);

        // ---------------------------------------------------------
    // ★追加: アーティスト名 (タイトルの下)
    // ---------------------------------------------------------
        artistText = AddUI<sf::ui::TextImage>();
        // Y座標をタイトル(-270)より下げる (-350あたり)
        artistText->transform.SetPosition(Vector3(0, LAYOUT_ARTIST_Y, LAYOUT_SONG_INFO_Z));
        // スケールはタイトルより少し小さく
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            device,
            L"", // 初期値
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
            80.0f, // フォントサイズ少し小さめ
            D2D1::ColorF(D2D1::ColorF::LightGray), // 色を少しグレーにすると階層感が出る
            1024, 200
        );

        // ---------------------------------------------------------
        // ★追加: BPM表示 (さらに下)
        // ---------------------------------------------------------
        bpmText = AddUI<sf::ui::TextImage>();
        // Y座標をさらに下げる (-420あたり)
        bpmText->transform.SetPosition(Vector3(0, LAYOUT_BPM_Y, LAYOUT_SONG_INFO_Z));
        bpmText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        bpmText->Create(
            device,
            L"", // 初期値
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
            80.0f, // フォントサイズ少し小さめ
            D2D1::ColorF(D2D1::ColorF::LightGray), // 色を少しグレーにすると階層感が出る
            1024, 200
        );

        // ---------------------------------------------------------
        // ★追加: 難易度表示 (右下スコアの上)
        // ---------------------------------------------------------
        difficultyText = AddUI<sf::ui::TextImage>();
        difficultyText->transform.SetPosition(Vector3(650, -300, 1.0f)); // スコアの上
        difficultyText->transform.SetScale(Vector3(4.0f, 1.0f, 1.0f));
        difficultyText->Create(
            device,
            L"", 
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 128
        );

        // プレイヤーレートを計算して表示（小数点第2位で切り捨て）
        float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
        // 切り捨て: 100倍して整数部分を取り、100で割る
        float truncatedRating = std::floor(playerRating * 100.0f) / 100.0f;
        wchar_t ratingBuf[64];
        swprintf_s(ratingBuf, L"Rating: %.2f [R or Click]", truncatedRating);
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
            L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
                L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
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
        if (allGenres.empty()) return;

        // インデックス正規化
        int N = (int)allGenres.size();
        currentGenreIndex = index % N;
        if (currentGenreIndex < 0) currentGenreIndex += N;

        // 曲リスト更新
        songs = allGenres[currentGenreIndex].songs;
        
        // 選択リセット
        targetIndex = 0;
        selectedIndex = 0;
        currentIndex = 0.0f;
        slideStartIdx = 0.0f;
        slideTimer = 0.0f;

        // タイトル等更新 (SelectNext相当のことをやる)
        // songsが切り替わったので、songs[0] を表示するように更新
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
        if (genreText) {
            genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));
        }

        // 前後のジャンル名更新
        if (prevGenreText) {
            int prevIdx = (currentGenreIndex - 1 + N) % N;
            prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
        }
        if (nextGenreText) {
            int nextIdx = (currentGenreIndex + 1) % N;
            nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
        }

        // ジャケットプール再構築 & リソースロード
        // InitializeTexturesでデフォはロード済みだが、曲ごとのジャケットはInitializeSongsでロードしていた
        // しかしInitializeSongsを書き換えたので、ここでは「現在のsongs用のジャケットテクスチャ」を用意する必要がある
        // 設計上、全曲分のテクスチャを持つとメモリが厳しいかもしれないが、
        // 今回のコードではInitializeSongsで `jacketTextures` を `songs.size()` 分確保していた
        // ジャンル切り替え時は `jacketTextures` を再生成する必要がある。
        
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
        ChangeGenre(currentGenreIndex + 1);
    }

    void SelectCanvas::SelectPreviousGenre() {
        ChangeGenre(currentGenreIndex - 1);
    }

} // namespace app::test
