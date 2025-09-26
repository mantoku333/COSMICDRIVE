#include "SelectCanvas.h"
#include "Time.h"
#include <algorithm>
#include <cmath>
#include "TestScene.h"
#include "Easing.h"

namespace app::test {

    // ===== ローカルユーティリティ =====
    // 0..N へ正規化
    static inline float WrapFloat(float x, float N) {
        float r = std::fmod(x, N);
        if (r < 0) r += N;
        return r;
    }

    // current から target(0..N-1) へ向かう最短差分（-N/2..+N/2）
    static inline float ShortestDeltaOnRing(float current, float target, float N) {
        float c = WrapFloat(current, N);
        float diff = target - c;
        if (diff > N * 0.5f) diff -= N;
        if (diff < -N * 0.5f) diff += N;
        return diff;
    }

    // ===== ヘルパ =====
    float SelectCanvas::ScaleByDistance(float d, float scaleCenter, float scaleEdge) {
        // 中央(d=0)ほど大きく、離れるほど小さくする（ガウス減衰）
        const float falloff = std::exp(-0.5f * (d * d));
        return scaleEdge + (scaleCenter - scaleEdge) * falloff;
    }

    // ===== Begin =====
    void SelectCanvas::Begin() {
        sf::ui::Canvas::Begin();

        InitializeTextures();
        InitializeSongs();
        InitializeUI();

        // シーンがメモリ上に存在しなければスタンバイ
        if (scene.isNull()) {
            scene = TestScene::StandbyScene();
        }

        // 初期選択の同期
        selectedIndex = std::clamp(selectedIndex, 0, (int)songs.size() - 1);
        targetIndex = selectedIndex;
        currentIndex = (float)targetIndex;

        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        // 更新コマンド
        updateCommand.Bind(std::bind(&SelectCanvas::Update, this, std::placeholders::_1));
    }

    // ===== 初期化：テクスチャ =====
    void SelectCanvas::InitializeTextures() {
        textureBack.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");
        textureSelectFrame.LoadTextureFromFile("Assets\\Texture\\SelectFrame.png");
        textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\Jacket\\DefaultJacket.png");
        textureTitlePanel.LoadTextureFromFile("Assets\\Texture\\songselect.png");
        textureCC.LoadTextureFromFile("Assets\\Texture\\CC.png");
    }

    // ===== 初期化：楽曲 =====
    void SelectCanvas::InitializeSongs() {
        songs = {
            {"GOODTEK",     "ebimayo",
             "Assets\\Texture\\Jacket\\GOODTEK.png",
             "Save\\chart\\goodtek.chart",
             "Save\\music\\GOODTEK.wav",
             "190"},

            {"真千年女王",  "しらいし",
             "Assets\\Texture\\Jacket\\sinsennen.png",
             "Save\\chart\\sinsen.chart",
             "Save\\music\\sinsen.wav",
             "160"},

            {"Chronomia",   "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},

            {"Chronomia",   "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},

            {"Chronomia",   "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},

            {"Chronomia",   "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},

            {"Chronomia",   "Lime/Kankitsu",
             "Assets\\Texture\\Jacket\\Chronomia.png",
             "Save\\chart\\chronomia.chart",
             "Save\\music\\chronomia.wav",
             "227"},
        };

        // ジャケット読み込み（なければデフォルト）
        jacketTextures.resize(songs.size());
        for (size_t i = 0; i < songs.size(); ++i) {
            const std::string& jacketPath = songs[i].jacketPath;
            if (!jacketTextures[i].LoadTextureFromFile(jacketPath)) {
                jacketTextures[i] = textureDefaultJacket;
            }
        }
    }

    // ===== 初期化：UI =====
    void SelectCanvas::InitializeUI() {

        // 背景
        backgroundGradient = AddUI<sf::ui::Image>();
        backgroundGradient->transform.SetPosition(Vector3(-200, 0, -10));
        backgroundGradient->transform.SetScale(Vector3(16.0f, 10.0f, 1.0f));
        backgroundGradient->material.texture = &textureBack;

        // タイトル
        titlePanel = AddUI<sf::ui::Image>();
        titlePanel->transform.SetPosition(Vector3(-200, 300, 0));
        titlePanel->transform.SetScale(Vector3(3.0f, 1.0f, 1.0f));
        titlePanel->material.texture = &textureTitlePanel;

       
        

        // 選択フレーム
        selectFrame = AddUI<sf::ui::Image>();
        selectFrame->transform.SetPosition(Vector3(CENTER_X, CENTER_Y, 1));
        selectFrame->transform.SetScale(Vector3(2.0f, 2.0f, 1.0f));
        selectFrame->material.texture = &textureSelectFrame;

        // CCパネル
        CC = AddUI<sf::ui::Image>();
        CC->transform.SetPosition(Vector3(400, -200, 0));
        CC->transform.SetScale(Vector3(3.0f, 1.0f, 1.0f));
        CC->material.texture = &textureCC;

        // ジャケットUIプール生成（固定個数で再利用）
        RebuildJacketPool();

        // 初期並び
        UpdateJacketPositions();
    }

    // ===== プール再構築 =====
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

        // 入力クールダウン
        if (inputCooldown > 0) {
            inputCooldown -= sf::Time::DeltaTime();
        }

        UpdateInput();
        UpdateAnimation();       // currentIndex ← targetIndex へ最短経路で補間
        UpdateJacketPositions(); // 位置/スケール/深度を更新
        UpdateSelectedFrame();   // 枠は中央スロットへ
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

    // ===== アニメーション（リング最短補間） =====
    void SelectCanvas::UpdateAnimation() {
        // 枠のパルス (Yoyoイージングで往復)
        const float pulsePhase = std::fmod(animationTime * 0.8f, 1.0f);
        const float pulse = (float)Easing(pulsePhase, EASE::EaseYoyo);
        const float pulseScale = 1.0f + 0.04f * pulse;
        if (selectFrame) {
            selectFrame->transform.SetScale(Vector3(2.1f * pulseScale, 2.1f * pulseScale, 1.0f));
        }

        const int N = (int)songs.size();
        if (N <= 0) return;

        // slideStartIdx から見た「最短の終点」を毎フレーム決定
        const float endPos = slideStartIdx + ShortestDeltaOnRing(slideStartIdx, (float)targetIndex, (float)N);
        const float dist = std::fabs(endPos - slideStartIdx);

        if (dist > 1e-4f) {
            slideTimer += sf::Time::DeltaTime();
            float alpha = std::clamp(slideTimer / slideDuration, 0.0f, 1.0f);
            float eased = (float)Easing(alpha, EASE::EaseInOutCubic);
            currentIndex = slideStartIdx + (endPos - slideStartIdx) * eased;

            if (alpha >= 1.0f) {
                // 終了時は畳んでズレをなくす
                currentIndex = WrapFloat(endPos, (float)N);
                slideStartIdx = currentIndex;
                slideTimer = 0.0f;

                // 選択インデックスも揃える
                selectedIndex = (int)std::round(currentIndex) % N;
                if (selectedIndex < 0) selectedIndex += N;
                targetIndex = selectedIndex;
            }
        }
        else {
            // 動いていないときもレンジ内に畳んでおく
            currentIndex = WrapFloat(currentIndex, (float)N);
            slideStartIdx = currentIndex;
            slideTimer = 0.0f;
        }
    }

    // ===== 並び（位置/スケール/深度） =====
    void SelectCanvas::UpdateJacketPositions() {
        if (songs.empty() || jacketPool.empty()) return;

        const int N = (int)songs.size();
        const int pool = (int)jacketPool.size();
        const int half = pool / 2;

        // 描画用は currentIndex の小数部でスムーズにスクロール
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

    // ===== 選択フレーム =====
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

    // ===== インデックス操作 =====
    void SelectCanvas::SelectNext() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        // 論理的な選択は 0..N-1 で回す
        targetIndex = (targetIndex + 1) % N;
        selectedIndex = targetIndex;

        // 今の位置を起点に新しい移動を開始
        // 途中で連打されても、都度「いまの見た目位置」から最短で次へ
        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        sf::debug::Debug::Log("Selected: " + songs[targetIndex].title);
    }

    void SelectCanvas::SelectPrevious() {
        if (songs.empty()) return;
        const int N = (int)songs.size();

        targetIndex = (targetIndex - 1 + N) % N;
        selectedIndex = targetIndex;

        slideStartIdx = currentIndex;
        slideTimer = 0.0f;

        sf::debug::Debug::Log("Selected: " + songs[targetIndex].title);
    }

    // ===== キャンセル =====
    void SelectCanvas::CancelSelection() {
        sf::debug::Debug::Log("Cancel selection - returning to previous screen");
    }

    // ===== 決定 =====
    void SelectCanvas::ConfirmSelection() {
        if (songs.empty()) return;

        const SongInfo& selected = songs[targetIndex];
        sf::debug::Debug::Log("選択しました: " + selected.title + " by " + selected.artist + " 譜面情報:" + selected.chartPath);

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

    // ===== getter =====
    const SongInfo& SelectCanvas::GetSelectedSong() const {
        if (songs.empty()) {
            static SongInfo empty = { "", "", "" };
            return empty;
        }
        int idx = std::clamp(targetIndex, 0, (int)songs.size() - 1);
        return songs[idx];
    }

} // namespace app::test
