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
#include <Windows.h> // UTF-8→UTF-16変換で使用

namespace app::test {

    // ===== UTF-8 → UTF-16 変換ユーティリティ =====
    static std::wstring Utf8ToWstring(const std::string& str)
    {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
        return wstr;
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
        songs = {
            {"GOODTEK", "ebimayo",
             "Assets\\Texture\\Jacket\\GOODTEK.png",
             "Save\\chart\\GOODTEK.sus",
             "Save\\music\\GOODTEK.wav",
             "190"},

            {"Queen Millennia", "しらいし",
             "Assets\\Texture\\Jacket\\sinsennen.png",
             "Save\\chart\\sinsen.chart",
             "Save\\music\\sinsen.wav",
             "160"},

            {"Chronomia", "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},
        };

        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            const std::string& jacketPath = songs[i].jacketPath;
            if (!jacketTextures[i].LoadTextureFromFile(jacketPath)) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    // ===== UI初期化 =====
    void SelectCanvas::InitializeUI() {
        backgroundGradient = AddUI<sf::ui::Image>();
        backgroundGradient->transform.SetPosition(Vector3(0, 0, -10));
        backgroundGradient->transform.SetScale(Vector3(14.0f, 8.0f, 1.0f));
        backgroundGradient->material.texture = &textureBack;

        titlePanel = AddUI<sf::ui::Image>();
        titlePanel->transform.SetPosition(Vector3(0, 300, 0));
        titlePanel->transform.SetScale(Vector3(5.0f, 1.0f, 1.0f));
        titlePanel->material.texture = &textureTitlePanel;

        selectFrame = AddUI<sf::ui::Image>();
        selectFrame->transform.SetPosition(Vector3(CENTER_X, CENTER_Y, 1));
        selectFrame->transform.SetScale(Vector3(10.0f, 10.0f, 1.0f));
        selectFrame->material.texture = &textureSelectFrame;
        selectFrame->material.SetColor({ 1, 1, 1, 0.8f });

        CC = AddUI<sf::ui::Image>();
        CC->transform.SetPosition(Vector3(500, -300, 0));
        CC->transform.SetScale(Vector3(3.0f, 1.0f, 1.0f));
        CC->material.texture = &textureCC;

        RebuildJacketPool();
        UpdateJacketPositions();

        // 曲タイトルテキストを1枚だけ作成
        auto* dx11 = sf::dx::DirectX11::Instance();
        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->transform.SetPosition(Vector3(0, -100, 0));
        songTitleText->transform.SetScale(Vector3(10.0f, 2.8f, 1.0f));
        songTitleText->Create(
            dx11->GetMainDevice().GetDevice(),
            L"", L"メイリオ",
            64.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1024, 256);
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
        // --- パルスアニメーション ---
        const float pulsePhase = std::fmod(animationTime * 0.8f, 1.0f);
        const float pulse = (float)Easing(pulsePhase, EASE::EaseYoyo);
        const float pulseScale = 1.0f + 0.04f * pulse; // 拡縮倍率（±4%）

        // --- 選択枠スケールを選択中ジャケットに追従させる ---
        if (selectFrame && !jacketPool.empty()) {
            const int half = (int)jacketPool.size() / 2;
            const Vector3 jacketScale = jacketPool[half]->transform.GetScale(); // 中央（選択中）のスケール

            // スケール＋パルス
            selectFrame->transform.SetScale(Vector3(
                jacketScale.x * 1.05f * pulseScale,
                jacketScale.y * 1.05f * pulseScale,
                1.0f
            ));
        }

        // --- アルファを脈動に合わせて変化 ---
        if (selectFrame)
        {
            float alpha = 0.5f + 0.5f * pulse;  // pulseが0〜1 → alphaが0.5〜1
            auto color = selectFrame->material.GetColor();
            color.w = alpha;                    // wがアルファ
            selectFrame->material.SetColor(color);
        }

        // --- スライドアニメーション（左右移動） ---
        const int N = (int)songs.size();
        if (N <= 0) return;

        const float endPos = slideStartIdx + ShortestDeltaOnRing(slideStartIdx, (float)targetIndex, (float)N);
        const float dist = std::fabs(endPos - slideStartIdx);

        if (dist > 1e-4f) {
            slideTimer += sf::Time::DeltaTime();
            float alpha = std::clamp(slideTimer / slideDuration, 0.0f, 1.0f);
            float eased = (float)Easing(alpha, EASE::EaseInOutCubic);
            currentIndex = slideStartIdx + (endPos - slideStartIdx) * eased;

            // スライド完了時
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

        // 曲タイトル更新
        if (songTitleText) {
            std::wstring title = Utf8ToWstring(songs[targetIndex].title);
            songTitleText->SetText(title);
        }

        sf::debug::Debug::Log("Selected: " + songs[targetIndex].title);
    }

    // ===== 前へ =====
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

        sf::debug::Debug::Log("Selected: " + songs[targetIndex].title);
    }

    // ===== キャンセル・決定 =====
    void SelectCanvas::CancelSelection() {
        sf::debug::Debug::Log("Cancel selection - returning to previous screen");
    }

    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;
        const SongInfo& selected = songs[targetIndex];
        sf::debug::Debug::Log("選択しました: " + selected.title + " by " + selected.artist);

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
