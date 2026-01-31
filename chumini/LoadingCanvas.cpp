#include "LoadingCanvas.h"
#include "Time.h"
#include "DirectX11.h"
#include "StringUtils.h"  // 文字コード変換ユーティリティ

namespace app::test {

    void LoadingCanvas::Begin() {
        sf::ui::Canvas::Begin();
        auto* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetDevice();

        // ロード画面の背景（黒塗りつぶしなど）が必要ならここで AddUI<Image> する
        // 今回はシンプルにテキストだけ

        // 0. Jacket Image
        jacketImage = AddUI<sf::ui::Image>();
        jacketImage->transform.SetPosition(Vector3(0, 50, 0)); // 少し下げてテキストとの距離微調整
        jacketImage->transform.SetScale(Vector3(3.5f, 3.5f, 1.0f));
        jacketImage->SetVisible(false);

       // 1. "NOW LOADING..."
        loadingText = AddUI<sf::ui::TextImage>();
        loadingText->transform.SetScale(Vector3(5.0f, 1.5f, 1.0f));
        loadingText->Create(
            context,
            L"NOW LOADING...",
            L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
            150.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            2000, 128
        );
        loadingText->transform.SetPosition(Vector3(550, -400, 0));

        // 2. 曲タイトル (画面中央などでっかく)
        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        songTitleText->Create(
            context,
            L"", // 初期値は空
            L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1800, 200
        );
        songTitleText->transform.SetPosition(Vector3(0, -200, 0));

        // ★修正: 最初は隠しておく！
        songTitleText->SetVisible(false);

        // 3. アーティスト (その下)
        artistText = AddUI<sf::ui::TextImage>();
        artistText->transform.SetScale(Vector3(8.0f, 1.5f, 1.0f));
        artistText->Create(
            context,
            L"",
            L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray),
            1024, 200
        );
        artistText->transform.SetPosition(Vector3(0, -350, 0));
        // ★修正: 最初は隠しておく！
        artistText->SetVisible(false);

        updateCommand.Bind(std::bind(&LoadingCanvas::Update, this, std::placeholders::_1));
    }

    void LoadingCanvas::SetTargetScene(sf::SafePtr<sf::IScene> scene) {
        this->targetScene = scene;
    }

    void LoadingCanvas::SetSongInfo(const SongInfo& info) {

        if (currentType != LoadingType::InGame) {
            return; 
        }
        // 受け取った情報をUIにセット
        if (info.title.empty()) {
            if (songTitleText) songTitleText->SetVisible(false);
            if (artistText) artistText->SetVisible(false);
            if (jacketImage) jacketImage->SetVisible(false);
            return;
        }

        // 情報がある時だけ表示をオンにする
        if (songTitleText) {
            songTitleText->SetText(sf::util::Utf8ToWstring(info.title));
            songTitleText->SetVisible(true); // ★ここで表示
        }
        if (artistText) {
            artistText->SetText(sf::util::Utf8ToWstring(info.artist));
            artistText->SetVisible(true); // ★ここで表示
        }

        // Jacket Image
        if (jacketImage && !info.jacketPath.empty()) {
            std::string sjisPath = sf::util::Utf8ToShiftJis(info.jacketPath);
            if (jacketTexture.LoadTextureFromFile(sjisPath)) {
                jacketImage->material.texture = &jacketTexture;
                jacketImage->SetVisible(true);
            }
        }
    }

    void LoadingCanvas::SetLoadingType(LoadingType type) {
        this->currentType = type;

        // Commonなら強制非表示
        if (currentType == LoadingType::Common) {
            if (songTitleText) songTitleText->SetVisible(false);
            if (artistText) artistText->SetVisible(false);
            if (jacketImage) jacketImage->SetVisible(false);
        }
    }

    void LoadingCanvas::Update(const sf::command::ICommand&) {
        // 1. タイマーは常に進める
        timer += sf::Time::DeltaTime();

        // 点滅演出
        if (loadingText) {
            float alpha = 0.5f + 0.5f * std::sin(timer * 10.0f);
            loadingText->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }

        if (targetScene.isNull()) return;

        // 2. 裏で常にロードチェックを行う（n秒経ってなくてもやる！）
        if (!isLoadCompleted) {
            if (targetScene->StandbyThisScene()) {
                isLoadCompleted = true; // ロード終わった！
                sf::debug::Debug::Log("Load Complete (Standby...)");
            }
        }

        // 3. 「ロード完了」かつ「3秒経過」の両方を満たしたら遷移
        if (isLoadCompleted && timer >= MIN_LOADING_TIME) {

            sf::debug::Debug::Log("Transitioning");
            targetScene->Activate();

            if (auto owner = actorRef.Target()) {
                owner->GetScene().DeActivate();
            }
        }

    }

}