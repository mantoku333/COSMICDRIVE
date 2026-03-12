#include "SelectCanvas.h"
#include "SelectScene.h"  // MVP: Presenter
#include "sf/Time.h"
#include <algorithm>
#include <cmath>
#include "IngameScene.h"
#include "TitleScene.h"
#include "Easing.h"
#include "Config.h"

// 繝・く繧ｹ繝域緒逕ｻ縺ｫ蠢・ｦ・
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
#include "StringUtils.h"  // 譁・ｭ励さ繝ｼ繝牙､画鋤繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ

namespace app::test {

    // sf::util 縺ｮ髢｢謨ｰ繧剃ｽｿ逕ｨ・医Ο繝ｼ繧ｫ繝ｫ髢｢謨ｰ縺ｯ蜑企勁・・
    using sf::util::WstringToUtf8;
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;

    // ===== 繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ =====
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

        // 襍ｷ蜍墓凾・壽峇繧ｿ繧､繝医Ν蛻晄悄蛹・
        if (songTitleText && !songs.empty()) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        // 笘・ｿｽ蜉: 繧｢繝ｼ繝・ぅ繧ｹ繝・
        if (artistText) {
            artistText->SetText(Utf8ToWstring(songs[targetIndex].artist));
        }

        // 笘・ｿｽ蜉: BPM
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 笘・ｿｽ蜉: 髮｣譏灘ｺｦ
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

        // 笘・ｿｽ蜉: 繧ｸ繝｣繝ｳ繝ｫ蜷・
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        if (!allGenres.empty()) {
            if(genreText) genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));

            int N = (int)allGenres.size(); // 繧ｸ繝｣繝ｳ繝ｫ謨ｰ
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

    // ===== 繝・け繧ｹ繝√Ε蛻晄悄蛹・=====
    void SelectCanvas::InitializeTextures() {
        textureBack.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        textureSelectFrame.LoadTextureFromFile("Assets\\Texture\\SelectFrame.png");
        textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\Jacket\\DefaultJacket.png");
        textureTitlePanel.LoadTextureFromFile("Assets\\Texture\\songselect.png");
        textureCC.LoadTextureFromFile("Assets\\Texture\\CC.png");
    }

    // ===== 讌ｽ譖ｲ蛻晄悄蛹・=====
    void SelectCanvas::InitializeSongs() {
        // SongListService縺ｫ繧ｹ繧ｭ繝｣繝ｳ繧貞ｧ碑ｭｲ
        songListService.ScanSongs("Songs");
        songs = songListService.GetCurrentSongs();

        // ----------------------------------------------------
        // 繧ｸ繝｣繧ｱ繝・ヨ繝・け繧ｹ繝√Ε縺ｮ繝ｭ繝ｼ繝・(笘・ｿｮ豁｣邂・園)
        // ----------------------------------------------------
        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            bool loaded = false;
            if (!songs[i].jacketPath.empty()) {

                // 笘・ｿｮ豁｣: WindowsAPI (LoadTextureFromFile) 縺ｫ貂｡縺吶◆繧√↓ Shift-JIS 縺ｫ螟画鋤縺吶ｋ
                // UTF-8 縺ｮ縺ｾ縺ｾ縺縺ｨ縲梧欠螳壹＆繧後◆繝輔ぃ繧､繝ｫ縺瑚ｦ九▽縺九ｊ縺ｾ縺帙ｓ縲阪↓縺ｪ繧翫∪縺・
                std::string sjisPath = Utf8ToShiftJis(songs[i].jacketPath);

                if (jacketTextures[i].LoadTextureFromFile(sjisPath)) {
                    loaded = true;
                }
                else {
                    // 繝ｭ繝ｼ繝牙､ｱ謨励Ο繧ｰ繧４hift-JIS縺ｧ蜃ｺ縺吶→譁・ｭ怜喧縺代＠縺ｪ縺・
                    sf::debug::Debug::Log("Jacket Load Failed: " + sjisPath);
                }
            }

            if (!loaded) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    // ===== UI蛻晄悄蛹・=====
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

        // 繧ｸ繝｣繧ｱ繝・ヨUI繧ｳ繝ｳ繝昴・繝阪Φ繝医・莠句燕逕滓・
        jacketComponents.clear();
        for(int i = 0; i < MAX_VISIBLE; ++i) {
             auto jacket = AddUI<sf::ui::Image>();
             jacket->transform.SetScale(Vector3(SCALE_EDGE, SCALE_EDGE, 1.0f));
             jacket->SetVisible(false); // 蛻晄悄縺ｯ髱櫁｡ｨ遉ｺ
             jacketComponents.push_back(jacket);
        }

        RebuildJacketPool();
        UpdateJacketPositions();

     // =========================================================
     // 繧｢繧ｦ繝医Λ繧､繝ｳ縺ｮ逕滓・
     // =========================================================

     // 縺ｵ縺｡縺ｮ螟ｪ縺・
        float outlineSize = 3.0f;

        // 4譁ｹ蜷代・繧ｺ繝ｬ繧貞ｮ夂ｾｩ
        Vector3 offsets[4] = {
            Vector3(-outlineSize, 0, 0),
            Vector3(outlineSize, 0, 0),
            Vector3(0, -outlineSize, 0),
            Vector3(0,  outlineSize, 0)
        };

        // 繝ｫ繝ｼ繝励＠縺ｦ4縺､菴懊ｋ
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
        // 繝｡繧､繝ｳ譁・ｭ暦ｼ域悽菴難ｼ峨・逕滓・
        // =========================================================
        titleText = AddUI<sf::ui::TextImage>();
        titleText->transform.SetPosition(Vector3(0, LAYOUT_TITLE_Y, 0.0f));
        titleText->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

        titleText->Create(
            device,
            L"SONG SELECT",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 笘・牡縺ｯ縲檎區縲・
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
    // 繧｢繝ｼ繝・ぅ繧ｹ繝亥錐
    // ---------------------------------------------------------
        artistText = AddUI<sf::ui::TextImage>();
        artistText->transform.SetPosition(Vector3(0, LAYOUT_ARTIST_Y, LAYOUT_SONG_INFO_Z));
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            device,
            L"", // 蛻晄悄蛟､
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
            L"", // 蛻晄悄蛟､
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray), 
            1024, 200
        );

        // ---------------------------------------------------------
        // 髮｣譏灘ｺｦ陦ｨ遉ｺ
        // ---------------------------------------------------------
        difficultyText = AddUI<sf::ui::TextImage>();
        difficultyText->transform.SetPosition(Vector3(650, -300, 1.0f)); // 繧ｹ繧ｳ繧｢縺ｮ荳・
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
        // 笘・ｿｽ蜉: 繧ｸ繝｣繝ｳ繝ｫ蜷崎｡ｨ遉ｺ (SONG SELECT縺ｮ荳・
        // ---------------------------------------------------------
        genreText = AddUI<sf::ui::TextImage>();
        // SONG SELECT(480) 繧医ｊ荳・(350縺ゅ◆繧・
        genreText->transform.SetPosition(Vector3(0, LAYOUT_GENRE_Y, 0)); 
        genreText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        genreText->Create(
            device,
            L"", 
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f, // 蟆代＠螟ｧ縺阪ａ
            D2D1::ColorF(D2D1::ColorF::Yellow), // 逶ｮ遶九▽濶ｲ
            1024, 200
        );

        // 笘・ｿｽ蜉: 蜑阪・繧ｸ繝｣繝ｳ繝ｫ
        prevGenreText = AddUI<sf::ui::TextImage>();
        prevGenreText->transform.SetPosition(Vector3(-550.0f, LAYOUT_GENRE_Y, 0)); // 蟾ｦ縺ｫ驟咲ｽｮ
        prevGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f)); // 繧ｵ繧､繧ｺ繝繧ｦ繝ｳ
        prevGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f, // 繝輔か繝ｳ繝医し繧､繧ｺ繝繧ｦ繝ｳ (70 -> 50)
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // 笘・ｿｽ蜉: 谺｡縺ｮ繧ｸ繝｣繝ｳ繝ｫ
        nextGenreText = AddUI<sf::ui::TextImage>();
        nextGenreText->transform.SetPosition(Vector3(550.0f, LAYOUT_GENRE_Y, 0)); // 蜿ｳ縺ｫ驟咲ｽｮ
        nextGenreText->transform.SetScale(Vector3(6.0f, 1.2f, 1.0f)); // 繧ｵ繧､繧ｺ繝繧ｦ繝ｳ
        nextGenreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f, // 繝輔か繝ｳ繝医し繧､繧ｺ繝繧ｦ繝ｳ (70 -> 50)
            D2D1::ColorF(D2D1::ColorF::Gray),
            512, 100
        );

        // 笘・ｿｽ蜉: 繝上う繧ｹ繧ｳ繧｢
        // 笘・ｿｽ蜉: 繝上う繧ｹ繧ｳ繧｢
        highScoreText = AddUI<sf::ui::TextImage>();
        highScoreText->transform.SetPosition(Vector3(650, -400, 1.0f)); 
        highScoreText->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f));
        highScoreText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            40.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1536, 128  // 讓ｪ蟷・ｒ1024縺九ｉ1536縺ｫ諡｡螟ｧ
        );

        // 笘・ｿｽ蜉: 繝ｩ繝ｳ繧ｯ繝槭・繧ｯ
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


        // 笘・ｿｽ蜉: 繝励Ξ繧､繝､繝ｼ繝ｬ繝ｼ繝郁｡ｨ遉ｺ・亥ｷｦ荳具ｼ・
        playerRatingText = AddUI<sf::ui::TextImage>();
        playerRatingText->transform.SetPosition(Vector3(-750, -450, 1.0f)); // 蟾ｦ荳・
        playerRatingText->transform.SetScale(Vector3(6.0f, 1.5f, 1.0f));
        playerRatingText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 128
        );

        // 繝励Ξ繧､繝､繝ｼ繝ｬ繝ｼ繝医ｒ險育ｮ励＠縺ｦ陦ｨ遉ｺ・亥ｰ乗焚轤ｹ隨ｬ2菴阪〒蛻・ｊ謐ｨ縺ｦ・・
        float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
        // 蛻・ｊ謐ｨ縺ｦ: 100蛟阪＠縺ｦ謨ｴ謨ｰ驛ｨ蛻・ｒ蜿悶ｊ縲・00縺ｧ蜑ｲ繧・
        float truncatedRating = std::floor(playerRating * 100.0f) / 100.0f;
        wchar_t ratingBuf[64];
        swprintf_s(ratingBuf, L"Rating: %.2f", truncatedRating);
        playerRatingText->SetText(ratingBuf);

        // 笘・ｿｽ蜉: 繝ｬ繝ｼ繝郁ｩｳ邏ｰ陦ｨ遉ｺUI・亥・譛溘・髱櫁｡ｨ遉ｺ・・
        // 閭梧勹逕ｨ繝・け繧ｹ繝√Ε繧定ｪｭ縺ｿ霎ｼ繧・域里蟄倥・閭梧勹繧貞・蛻ｩ逕ｨ・・
        ratingDetailBackgroundTexture.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        
        // 蜊企乗・閭梧勹・亥ｷｦ蛛ｴ縺ｫ驟咲ｽｮ・・
        ratingDetailBackground = AddUI<sf::ui::Image>();
        ratingDetailBackground->transform.SetPosition(Vector3(-550, 0, 5.0f)); // 繝・く繧ｹ繝医ｈ繧雁･･縺ｫ
        ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f)); // 10蛟阪↓諡｡螟ｧ
        ratingDetailBackground->material.texture = &ratingDetailBackgroundTexture;
        ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.90f}); // 鮟偵・0%荳埼乗・
        ratingDetailBackground->SetVisible(false);

        // "RATING" 繝ｩ繝吶Ν・井ｸ逡ｪ荳奇ｼ・
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

        // 蜷郁ｨ医Ξ繝ｼ繝郁｡ｨ遉ｺ・医Λ繝吶Ν縺ｮ荳具ｼ・
        ratingDetailTotalText = AddUI<sf::ui::TextImage>();
        ratingDetailTotalText->transform.SetPosition(Vector3(-650, 380, 10.0f)); // 400 -> 380
        ratingDetailTotalText->transform.SetScale(Vector3(10.0f, 3.0f, 1.0f));
        ratingDetailTotalText->Create(
            device,
            L"",
            L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
            100.0f, // 螟ｧ縺阪￥
            D2D1::ColorF(D2D1::ColorF::Cyan),
            1024, 200
        );
        ratingDetailTotalText->SetVisible(false);

        // 繧ｿ繧､繝医Ν・亥粋險医Ξ繝ｼ繝医・荳具ｼ・
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

        // 隧ｳ邏ｰ繝ｪ繧ｹ繝茨ｼ・0陦後∝ｷｦ蛛ｴ・・
        for (int i = 0; i < 10; ++i) {
            auto* line = AddUI<sf::ui::TextImage>();
            line->transform.SetPosition(Vector3(-650, 200 - i * 70, 10.0f)); // 220 -> 200
            line->transform.SetScale(Vector3(8.0f, 1.0f, 1.0f)); // 蟆代＠蟆上＆縺・
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

    // ===== 繧ｸ繝｣繧ｱ繝・ヨ蜀肴ｧ狗ｯ・=====
    void SelectCanvas::RebuildJacketPool() {
        // 蜈ｨ縺ｦ荳譌ｦ髱櫁｡ｨ遉ｺ
        for (auto* img : jacketComponents) {
            if (img) img->SetVisible(false);
        }
        jacketPool.clear();

        if (songs.empty()) return;

        // 蠢・ｦ∵焚繧定ｨ育ｮ・
        const int poolSize = std::min<int>(MAX_VISIBLE, std::max<int>(3, (int)songs.size()));
        
        // 莠句燕逕滓・貂医∩繧ｳ繝ｳ繝昴・繝阪Φ繝医°繧牙牡繧雁ｽ薙※
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

        // 繝ｬ繝ｼ繝・ぅ繝ｳ繧ｰ隧ｳ邏ｰ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ譖ｴ譁ｰ
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


    // ===== 繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ =====
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

        // 繝ｬ繝ｼ繝・ぅ繝ｳ繧ｰ隧ｳ邏ｰ陦ｨ遉ｺ縺ｮ繧ｹ繝ｩ繧､繝峨う繝ｳ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ・井ｸ九°繧我ｸ翫↓繧ｹ繝ｩ繧､繝会ｼ・
        if (ratingDetailAnimationTimer > 0.0f) {
            // 繧､繝ｼ繧ｸ繝ｳ繧ｰ驕ｩ逕ｨ・・aseOutExpo縺ｧ繧ｷ繝･繝・→蜃ｺ縺ｦ縺上ｋ諢溘§縺ｫ・・
            float easedTimer = (float)Easing(ratingDetailAnimationTimer, EASE::EaseOutExpo);
            
            // 繧ｹ繝ｩ繧､繝峨う繝ｳ縺ｮ繧ｪ繝輔そ繝・ヨ險育ｮ暦ｼ井ｸ九°繧会ｼ・
            // 逕ｻ髱｢螟厄ｼ井ｸ具ｼ峨°繧牙ｮ壻ｽ咲ｽｮ・・・峨∈遘ｻ蜍・
            const float startOffsetY = -1080.0f;
            float currentOffsetY = startOffsetY * (1.0f - easedTimer);
            
            // X蠎ｧ讓吶・菴咲ｽｮ隱ｿ謨ｴ・亥ｷｦ縺ｫ隧ｰ繧√ｋ・・-550 -> -750 -> -650・・
            const float uiX = -650.0f;

            // 閭梧勹縺ｮ譖ｴ譁ｰ
            if (ratingDetailBackground) {
                // 菴咲ｽｮ繧呈峩譁ｰ・・縺ｯ蝗ｺ螳壹〆縺ｯ繧ｹ繝ｩ繧､繝峨〇縺ｯ縺昴・縺ｾ縺ｾ・・
                ratingDetailBackground->transform.SetPosition(Vector3(uiX, 0.0f + currentOffsetY, 5.0f));
                // 繧ｹ繧ｱ繝ｼ繝ｫ縺ｯ蝗ｺ螳夲ｼ域怙螟ｧ繧ｵ繧､繧ｺ・・
                ratingDetailBackground->transform.SetScale(Vector3(7.0f, 16.0f, 1.0f));
                ratingDetailBackground->material.SetColor({0.0f, 0.0f, 0.0f, 0.50f * easedTimer});
                ratingDetailBackground->SetVisible(true);
            }
            
            // 繝ｩ繝吶Ν縺ｮ譖ｴ譁ｰ
            if (ratingDetailLabelText) {
                // 螳壻ｽ咲ｽｮ(460) + 繧ｪ繝輔そ繝・ヨ
                ratingDetailLabelText->transform.SetPosition(Vector3(uiX, 460.0f + currentOffsetY, 10.0f));
                ratingDetailLabelText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer});
                ratingDetailLabelText->SetVisible(true);
            }

            // 蜷郁ｨ医Ξ繝ｼ繝医・譖ｴ譁ｰ
            if (ratingDetailTotalText) {
                // 螳壻ｽ咲ｽｮ(380) + 繧ｪ繝輔そ繝・ヨ
                ratingDetailTotalText->transform.SetPosition(Vector3(uiX, 380.0f + currentOffsetY, 10.0f));
                ratingDetailTotalText->material.SetColor({0.0f, 1.0f, 1.0f, easedTimer}); // 繧ｷ繧｢繝ｳ縺ｧ逶ｮ遶九◆縺帙ｋ
                ratingDetailTotalText->SetVisible(true);
            }
            
            // 繧ｿ繧､繝医Ν縺ｮ譖ｴ譁ｰ
            if (ratingDetailTitle) {
                // 螳壻ｽ咲ｽｮ(280) + 繧ｪ繝輔そ繝・ヨ
                ratingDetailTitle->transform.SetPosition(Vector3(uiX, 280.0f + currentOffsetY, 10.0f));
                ratingDetailTitle->material.SetColor({1.0f, 1.0f, 0.0f, easedTimer});
                ratingDetailTitle->SetVisible(true);
            }
            
            // 繝ｪ繧ｹ繝医・譖ｴ譁ｰ
            int lineIndex = 0;
            for (auto* line : ratingDetailLines) {
                if (line) {
                    // 螳壻ｽ咲ｽｮ(200 - i*70) + 繧ｪ繝輔そ繝・ヨ
                    float baseLineY = 200.0f - lineIndex * 70.0f;
                    line->transform.SetPosition(Vector3(uiX, baseLineY + currentOffsetY, 10.0f));
                    line->material.SetColor({1.0f, 1.0f, 1.0f, easedTimer});
                    
                    // 繧ｫ繧ｦ繝ｳ繝医↓蝓ｺ縺･縺・※陦ｨ遉ｺ蛻ｶ蠕｡
                    if (lineIndex < ratingDetailItemCount) {
                        line->SetVisible(true);
                    } else {
                        line->SetVisible(false);
                    }
                }
                lineIndex++;
            }
        } else {
            // 繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ邨ゆｺ・凾縺ｯ髱櫁｡ｨ遉ｺ
            if (ratingDetailBackground) ratingDetailBackground->SetVisible(false);
            if (ratingDetailLabelText) ratingDetailLabelText->SetVisible(false);
            if (ratingDetailTitle) ratingDetailTitle->SetVisible(false);
            if (ratingDetailTotalText) ratingDetailTotalText->SetVisible(false);
            for (auto* line : ratingDetailLines) {
                if (line) line->SetVisible(false);
            }
        }
    }

    // ===== 繧ｸ繝｣繧ｱ繝・ヨ驟咲ｽｮ譖ｴ譁ｰ =====
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

    // ===== 譫譖ｴ譁ｰ =====
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

    // ===== 谺｡縺ｸ =====
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

        // 繧｢繝ｼ繝・ぅ繧ｹ繝域峩譁ｰ
        if (artistText) {
            std::wstring artist = Utf8ToWstring(songs[targetIndex].artist);
            artistText->SetText(artist);
        }

        // BPM譖ｴ譁ｰ
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 髮｣譏灘ｺｦ譖ｴ譁ｰ
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


        // 笘・枚蟄怜喧縺大ｯｾ遲・ Shift-JIS螟画鋤縺励※繝ｭ繧ｰ蜃ｺ蜉・
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
        // 繧｢繝ｼ繝・ぅ繧ｹ繝域峩譁ｰ
        if (artistText) {
            std::wstring artist = Utf8ToWstring(songs[targetIndex].artist);
            artistText->SetText(artist);
        }

        // BPM譖ｴ譁ｰ
        if (bpmText) {
            std::wstring bpmStr = L"BPM: " + Utf8ToWstring(songs[targetIndex].bpm);
            bpmText->SetText(bpmStr);
        }

        // 髮｣譏灘ｺｦ譖ｴ譁ｰ
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

        // 笘・枚蟄怜喧縺大ｯｾ遲・
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));

        PlayPreview();
    }

    void SelectCanvas::CancelSelection() {
        // MVP: Presenter縺ｫ繧ｷ繝ｼ繝ｳ驕ｷ遘ｻ繧貞ｧ碑ｭｲ
        if (presenter) {
            presenter->NavigateToTitle();
        }
    }

    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;

        const SongInfo& selected = songs[targetIndex];

        // MVP: Presenter縺ｫ繧ｷ繝ｼ繝ｳ驕ｷ遘ｻ繧貞ｧ碑ｭｲ
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

        // UI譖ｴ譁ｰ
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
        // Score & Rank譖ｴ譁ｰ
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
            // 蜷郁ｨ医Ξ繝ｼ繝郁ｨｭ螳・
            float playerRating = app::test::RatingManager::Instance().GetPlayerRating();
            float truncated = std::floor(playerRating * 100.0f) / 100.0f;
            wchar_t totalBuf[64];
            swprintf_s(totalBuf, L"%.2f", truncated);
            ratingDetailTotalText->SetText(totalBuf);
            
            // 隧ｳ邏ｰ繝ｪ繧ｹ繝郁ｨｭ螳・
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

    // ===== 繝励Ξ繝薙Η繝ｼ蜀咲函 =====
    void SelectCanvas::PlayPreview() {
        // 蜑阪・譖ｲ繧呈ｭ｢繧√ｋ
        previewPlayer.Stop();
        previewResource = nullptr;

        if (songs.empty()) return;

        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        const auto& info = songs[idx];

        if (info.musicPath.empty()) return;

        // 繝代せ繧担hift-JIS縺ｫ螟画鋤
        std::string sjisPath = Utf8ToShiftJis(info.musicPath);

        // 譁ｰ縺励＞繝ｪ繧ｽ繝ｼ繧ｹ繧剃ｽ懈・
        previewResource = new sf::sound::SoundResource();

        // 繝ｭ繝ｼ繝牙､ｱ謨励＠縺溘ｉ蜀咲函縺励↑縺・
        if (FAILED(previewResource.Target()->LoadSound(sjisPath, true))) { // true = loop
            sf::debug::Debug::Log("Preview Load Failed: " + sjisPath);
            return;
        }

        previewPlayer.SetResource(previewResource);
        previewPlayer.SetVolume(gAudioVolume.master * gAudioVolume.bgm); // 蠢・ｦ√↓蠢懊§縺ｦ隱ｿ謨ｴ
        previewPlayer.Play(info.previewStartTime);
    }

    // ===== 繧ｸ繝｣繝ｳ繝ｫ螟画峩 =====
    void SelectCanvas::ChangeGenre(int direction) {
        // 迴ｾ蝨ｨ縺ｮ繧ｸ繝｣繝ｳ繝ｫ + direction 縺ｧ谺｡/蜑阪・繧ｸ繝｣繝ｳ繝ｫ縺ｫ遘ｻ蜍・
        int currentIdx = songListService.GetCurrentGenreIndex();
        int newIdx = currentIdx + direction;
        // SongListService縺ｫ繧ｸ繝｣繝ｳ繝ｫ螟画峩繧貞ｧ碑ｭｲ・亥ｾｪ迺ｰ蜃ｦ逅・・SongListService蜀・〒陦後ｏ繧後ｋ・・
        songs = songListService.ChangeGenre(newIdx);
        
        // 驕ｸ謚槭Μ繧ｻ繝・ヨ
        targetIndex = 0;
        selectedIndex = 0;
        currentIndex = 0.0f;
        slideStartIdx = 0.0f;
        slideTimer = 0.0f;

        // 繧ｿ繧､繝医Ν遲画峩譁ｰ
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

        // 繧ｸ繝｣繝ｳ繝ｫ蜷肴峩譁ｰ
        const auto& allGenres = songListService.GetAllGenres();
        int currentGenreIndex = songListService.GetCurrentGenreIndex();
        int N = (int)allGenres.size();

        if (genreText && !allGenres.empty()) {
            genreText->SetText(Utf8ToWstring(allGenres[currentGenreIndex].name));
        }

        // 蜑榊ｾ後・繧ｸ繝｣繝ｳ繝ｫ蜷肴峩譁ｰ
        if (prevGenreText && N > 0) {
            int prevIdx = (currentGenreIndex - 1 + N) % N;
            prevGenreText->SetText(Utf8ToWstring(allGenres[prevIdx].name));
        }
        if (nextGenreText && N > 0) {
            int nextIdx = (currentGenreIndex + 1) % N;
            nextGenreText->SetText(Utf8ToWstring(allGenres[nextIdx].name));
        }

        // 笘・ｿｮ豁｣: jacketTextures 縺ｮ蜀阪Ο繝ｼ繝・
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
