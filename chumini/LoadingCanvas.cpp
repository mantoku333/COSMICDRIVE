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
        jacketImage->transform.SetPosition(Vector3(0, 0, 0)); // Center
        jacketImage->transform.SetScale(Vector3(3.5f, 3.5f, 1.0f));
        jacketImage->SetVisible(false);

        // 1. "NOW LOADING..."
        loadingText = AddUI<sf::ui::TextImage>();
        loadingText->transform.SetScale(Vector3(5.0f, 1.2f, 1.0f));
        loadingText->Create(
            context,
            L"NOW LOADING...",
            L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
            150.0f,
            D2D1::ColorF(D2D1::ColorF::Cyan),
            2000, 128
        );
        loadingText->transform.SetPosition(Vector3(600, -300, 0));

        // 2. 曲タイトル
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
        songTitleText->transform.SetPosition(Vector3(0, -270, 0)); // Match SelectCanvas

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
        artistText->transform.SetPosition(Vector3(0, -350, 0)); // Match SelectCanvas
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
            
            // Commonは 0.5秒
            MIN_LOADING_TIME = 0.5f;
        } else {
            // InGameは 1.0秒
            MIN_LOADING_TIME = 1.0f;
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

    void LoadingCanvas::Draw() {
        sf::ui::Canvas::Draw();
        DrawLoadingGauge();
    }

    void LoadingCanvas::DrawLoadingGauge() {
        // ロード進行度を取得
        float ratio = 0.0f;
        if (!targetScene.isNull()) {
            ratio = targetScene->loadProgress;
        }
        
        // 完了時は強制1.0
        if (isLoadCompleted) ratio = 1.0f;
        
        // Clamp
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;

        // Visual Layout
        float barWidth = 800.0f; // Slightly smaller to fit on right
        float barHeight = 20.0f;
        float baseY = -400.0f; // Bottom
        
        // Right Side Placement
        // Screen Width ~1920. Center is 0. Right Edge is ~960.
        // Let's place it at X = 400 (Center of bar)
        float centerX = 400.0f; 

        auto* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetContext();

        // Shader Setup (ScoreGauge流用)
        dx11->gsScoreGauge.SetGPU(dx11->GetMainDevice());
        dx11->psScoreGauge.SetGPU(dx11->GetMainDevice());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // --- Layer 1: Background ---
        {
            float scaleX = barWidth * 0.5f;
            float scaleY = barHeight * 0.5f;
            
            // 下地は常に全幅
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(centerX, baseY, 0.0f); 

            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            // Material
            mtl material;
            material.diffuseColor = { 0.2f, 0.2f, 0.2f, 0.8f }; // Dark Grey
            float aspect = barWidth / barHeight;
            material.emissionColor = { aspect, 0, 0, 0 }; // x=aspect for rounded corners
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            dx11->vsNone.SetGPU(dx11->GetMainDevice());
            context->Draw(1, 0);
        }

        // --- Layer 2: Foreground (Rainbow) ---
        if (ratio > 0.001f) {
            float currentWidth = barWidth * ratio;
            float scaleX = currentWidth * 0.5f;
            float scaleY = barHeight * 0.5f;

            // Bar Left Edge = CenterX - HalfTotalWidth
            float leftEdge = centerX - barWidth * 0.5f;
            
            // Current Content Center = LeftEdge + HalfCurrentWidth
            float fgCenterX = leftEdge + currentWidth * 0.5f;

            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(fgCenterX, baseY, 0.0f);

            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            // Material
            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // Base color (can be ignored by rainbow logic)
            
            float aspect = currentWidth / barHeight;
            // 短すぎると見た目が崩れる場合の補正（Shader依存だが一応）
            if (aspect < 1.0f) aspect = 1.0f; 

            // Enable Rainbow: y=1.0f, Time: z=timer
            material.emissionColor = { aspect, 1.0f, timer, 0 }; 
            
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            context->Draw(1, 0);
        }

        // Restore Standard Shader
        dx11->ps2d.SetGPU(dx11->GetMainDevice());
    }

}