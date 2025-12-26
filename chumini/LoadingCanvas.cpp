#include "LoadingCanvas.h"
#include "Time.h"
#include "DirectX11.h"

namespace app::test {

    void LoadingCanvas::Begin() {
        sf::ui::Canvas::Begin();
        auto* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetDevice();

        // ロード画面の背景（黒塗りつぶしなど）が必要ならここで AddUI<Image> する
        // 今回はシンプルにテキストだけ

       // 1. "NOW LOADING..."
        loadingText = AddUI<sf::ui::TextImage>();
        loadingText->Create(
            context,
            L"NOW LOADING...",
            L"851ゴチカクット",
            60.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            512, 128
        );
        loadingText->transform.SetPosition(Vector3(700, -450, 0));

        // 2. 曲タイトル (画面中央などでっかく)
        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->Create(
            context,
            L"", // 初期値は空
            L"851ゴチカクット",
            80.0f,
            D2D1::ColorF(D2D1::ColorF::DeepSkyBlue),
            1024, 200
        );
        songTitleText->transform.SetPosition(Vector3(0, 100, 0));

        // ★修正: 最初は隠しておく！
        songTitleText->SetVisible(false);

        // 3. アーティスト (その下)
        artistText = AddUI<sf::ui::TextImage>();
        artistText->Create(
            context,
            L"",
            L"851ゴチカクット",
            50.0f,
            D2D1::ColorF(D2D1::ColorF::LightGray),
            1024, 100
        );
        artistText->transform.SetPosition(Vector3(0, 0, 0));
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
            return;
        }

        // 文字列変換ラムダ
        auto toWstr = [](const std::string& src) -> std::wstring {
            if (src.empty()) return L"";
            int size = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
            std::wstring dst(size, 0);
            MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &dst[0], size);
            return dst;
            };

        // 情報がある時だけ表示をオンにする
        if (songTitleText) {
            songTitleText->SetText(toWstr(info.title));
            songTitleText->SetVisible(true); // ★ここで表示
        }
        if (artistText) {
            artistText->SetText(toWstr(info.artist));
            artistText->SetVisible(true); // ★ここで表示
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
                sf::debug::Debug::Log("ロード完了（待機中...）");
            }
        }

        // 3. 「ロード完了」かつ「3秒経過」の両方を満たしたら遷移
        if (isLoadCompleted && timer >= MIN_LOADING_TIME) {

            sf::debug::Debug::Log("遷移実行");
            targetScene->Activate();

            if (auto owner = actorRef.Target()) {
                owner->GetScene().DeActivate();
            }
        }

    }

}