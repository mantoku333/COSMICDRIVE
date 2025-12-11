#include "SelectCanvas.h"
#include "Time.h"
#include <algorithm>
#include <cmath>
#include "TestScene.h"
#include "Easing.h"

// テキスト描画に必要
#include "Text.h"
#include "TextImage.h"
#include "DWriteContext.h"
#include "DirectX11.h"
#include <Windows.h> 

#include <filesystem>
#include "ChedParser.h" 

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

        InitializeTextures();
        InitializeSongs();
        InitializeUI();

        if (scene.isNull()) {
            scene = TestScene::StandbyScene();
        }

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
        namespace fs = std::filesystem;

        std::string rootPath = "Songs";

        if (fs::exists(rootPath) && fs::is_directory(rootPath)) {
            for (const auto& dirEntry : fs::directory_iterator(rootPath)) {
                if (!dirEntry.is_directory()) continue;

                for (const auto& fileEntry : fs::directory_iterator(dirEntry.path())) {
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

                            // ジャケットパス結合
                            if (!parser.jacketFile.empty()) {
                                std::wstring jacketW = Utf8ToWstring(parser.jacketFile);
                                fs::path fullJacketPath = parentDir / jacketW;
                                info.jacketPath = WstringToUtf8(fullJacketPath.wstring());
                            }
                            else {
                                info.jacketPath = "";
                            }

                            // 音声パス結合
                            if (!parser.waveFile.empty()) {
                                std::wstring waveW = Utf8ToWstring(parser.waveFile);
                                fs::path fullWavePath = parentDir / waveW;
                                info.musicPath = WstringToUtf8(fullWavePath.wstring());
                            }

                            info.bpm = std::to_string(parser.bpm);
                            songs.push_back(info);

                            // ★ログ文字化け対策: Shift-JISに変換して表示
                            sf::debug::Debug::Log("Load OK: " + Utf8ToShiftJis(info.title));
                        }
                    }
                }
            }
        }
        else {
            sf::debug::Debug::Log("Songs directory not found: " + rootPath);
        }

        if (songs.empty()) {
            songs.push_back({ "No Music Found", "System", "", "", "", "0" });
        }

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

       /* backgroundGradient = AddUI<sf::ui::Image>();
        backgroundGradient->transform.SetPosition(Vector3(0, 0, -10));
        backgroundGradient->transform.SetScale(Vector3(14.0f, 8.0f, 1.0f));
        backgroundGradient->material.texture = &textureBack;*/

        selectFrame = AddUI<sf::ui::Image>();
        selectFrame->transform.SetPosition(Vector3(CENTER_X, CENTER_Y, 1));
        selectFrame->transform.SetScale(Vector3(10.0f, 50.0f, 1.0f));
        selectFrame->material.texture = &textureSelectFrame;
        selectFrame->material.SetColor({ 1, 1, 1, 0.8f });

        CC = AddUI<sf::ui::Image>();
        CC->transform.SetPosition(Vector3(500, -350, 0));
        CC->transform.SetScale(Vector3(3.0f, 1.0f, 1.0f));
        CC->material.texture = &textureCC;

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

            // 座標：メインの位置(0, 300) から少しズラす
            // Z座標：メインより奥に見せたいので 1.0f に設定
            titleOutline[i]->transform.SetPosition(Vector3(0 + offsets[i].x, 300 + offsets[i].y, 1.0f));

            // スケール：メインと全く同じにする（ここ重要！）
            titleOutline[i]->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

            titleOutline[i]->Create(
                device,
                L"SONG SELECT",         // 同じ文字
                L"851ゴチカクット",     // 同じフォント
                120.0f,                 // 同じサイズ
                D2D1::ColorF(D2D1::ColorF::White),
                1024, 240
            );
        }

        // =========================================================
        // ★ 2. メイン文字（本体）の生成
        // =========================================================
        titleText = AddUI<sf::ui::TextImage>();

        // 座標：中心 (0, 300), Z座標は手前 (0.0f)
        titleText->transform.SetPosition(Vector3(0, 300, 0.0f));

        // スケール
        titleText->transform.SetScale(Vector3(15.0f, 3.0f, 1.0f));

        titleText->Create(
            device,
            L"SONG SELECT",
            L"851ゴチカクット",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // ★色は「白」
            1024, 240
        );

        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->transform.SetPosition(Vector3(0, -270, 0));
        songTitleText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        songTitleText->Create(
            device,
            L"",
            L"851ゴチカクット",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1800, 200);

        // ---------------------------------------------------------
    // ★追加: アーティスト名 (タイトルの下)
    // ---------------------------------------------------------
        artistText = AddUI<sf::ui::TextImage>();
        // Y座標をタイトル(-270)より下げる (-350あたり)
        artistText->transform.SetPosition(Vector3(0, -350, 0));
        // スケールはタイトルより少し小さく
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            device,
            L"", // 初期値
            L"851ゴチカクット",
            80.0f, // フォントサイズ少し小さめ
            D2D1::ColorF(D2D1::ColorF::LightGray), // 色を少しグレーにすると階層感が出る
            1024, 200
        );

        // ---------------------------------------------------------
        // ★追加: BPM表示 (さらに下)
        // ---------------------------------------------------------
        bpmText = AddUI<sf::ui::TextImage>();
        // Y座標をさらに下げる (-420あたり)
        bpmText->transform.SetPosition(Vector3(0, -420, 0));
        bpmText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        bpmText->Create(
            device,
            L"", // 初期値
            L"851ゴチカクット",
            80.0f, // フォントサイズ少し小さめ
            D2D1::ColorF(D2D1::ColorF::LightGray), // 色を少しグレーにすると階層感が出る
            1024, 200
        );

    }

    // ===== ジャケット再構築 =====
    void SelectCanvas::RebuildJacketPool() {
        for (auto* img : jacketPool) {
            if (img) img->SetVisible(false);
        }
        jacketPool.clear();

        if (songs.empty()) return;

        const int poolSize = std::min<int>(MAX_VISIBLE, std::max<int>(3, (int)songs.size()));
        jacketPool.reserve(poolSize);

        for (int i = 0; i < poolSize; ++i) {
            auto jacket = AddUI<sf::ui::Image>();
            jacket->transform.SetScale(Vector3(SCALE_EDGE, SCALE_EDGE, 1.0f));
            jacket->SetVisible(true);
            jacketPool.push_back(jacket);
        }
    }

    // ===== Update =====
    void SelectCanvas::Update(const sf::command::ICommand& command) {
        animationTime += sf::Time::DeltaTime();
        if (inputCooldown > 0) inputCooldown -= sf::Time::DeltaTime();

        UpdateInput();
        UpdateAnimation();
        UpdateJacketPositions();
        UpdateSelectedFrame();
    }

    // ===== 入力 =====
    void SelectCanvas::UpdateInput() {
        if (inputCooldown > 0) return;
        SInput& input = SInput::Instance();

        if (input.GetKeyDown(Key::KEY_RIGHT) || input.GetKeyDown(Key::KEY_D)) {
            SelectNext();
            inputCooldown = INPUT_DELAY;
        }
        if (input.GetKeyDown(Key::KEY_LEFT) || input.GetKeyDown(Key::KEY_A)) {
            SelectPrevious();
            inputCooldown = INPUT_DELAY;
        }
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
        const float pulsePhase = std::fmod(animationTime * 0.8f, 1.0f);
        const float pulse = (float)Easing(pulsePhase, EASE::EaseYoyo);
        const float pulseScale = 1.0f + 0.04f * pulse;

        if (selectFrame && !jacketPool.empty()) {
            const int half = (int)jacketPool.size() / 2;
            const Vector3 jacketScale = jacketPool[half]->transform.GetScale();

            selectFrame->transform.SetScale(Vector3(
                jacketScale.x * 1.05f * pulseScale,
                jacketScale.y * 1.05f * pulseScale,
                1.0f
            ));
        }

        if (selectFrame)
        {
            float alpha = 0.5f + 0.5f * pulse;
            auto color = selectFrame->material.GetColor();
            color.w = alpha;
            selectFrame->material.SetColor(color);
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
            float x = CENTER_X + rel * BASE_SPACING;
            float y = CENTER_Y;

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


        // ★文字化け対策: Shift-JIS変換してログ出力
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));
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

        // ★文字化け対策
        sf::debug::Debug::Log("Selected: " + Utf8ToShiftJis(songs[targetIndex].title));
    }

    void SelectCanvas::CancelSelection() {
        sf::debug::Debug::Log("Cancel selection - returning to previous screen");
    }

    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;
        const SongInfo& selected = songs[targetIndex];

        // ★文字化け対策
        sf::debug::Debug::Log("選択しました: " + Utf8ToShiftJis(selected.title));

        if (scene->StandbyThisScene()) {
            if (auto* testScene = dynamic_cast<app::test::TestScene*>(scene.Get())) {
                testScene->SetSelectedSong(selected);
            }
            scene->Activate();
        }

        if (auto actor = actorRef.Target()) {
            auto* thisscene = &actor->GetScene();
            thisscene->DeActivate();
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

} // namespace app::test