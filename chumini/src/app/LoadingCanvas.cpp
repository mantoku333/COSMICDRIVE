#include "LoadingCanvas.h"
#include "LoadingScene.h" // Full definition for Presenter calls
#include "sf/Time.h"
#include "DirectX11.h"
#include "StringUtils.h" 

namespace app::test {

    void LoadingCanvas::Begin() {
        sf::ui::Canvas::Begin();
        auto* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetDevice();

        // 0. Jacket Image
        jacketImage = AddUI<sf::ui::Image>();
        jacketImage->transform.SetPosition(Vector3(0, 0, 0)); 
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

        // 2. Song Title
        songTitleText = AddUI<sf::ui::TextImage>();
        songTitleText->transform.SetScale(Vector3(10.0f, 2.0f, 1.0f));
        songTitleText->Create(
            context,
            L"", 
            L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
            120.0f,
            D2D1::ColorF(D2D1::ColorF::White),
            1800, 200
        );
        songTitleText->transform.SetPosition(Vector3(0, -270, 0)); 
        songTitleText->SetVisible(false);

        // 3. Artist
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
        artistText->SetVisible(false);

        updateCommand.Bind(std::bind(&LoadingCanvas::Update, this, std::placeholders::_1));
    }

    void LoadingCanvas::SetSongInfo(const SongInfo& info) {
        if (currentType != LoadingType::InGame) {
            return; 
        }
        
        if (info.title.empty()) {
            if (songTitleText) songTitleText->SetVisible(false);
            if (artistText) artistText->SetVisible(false);
            if (jacketImage) jacketImage->SetVisible(false);
            return;
        }

        if (songTitleText) {
            songTitleText->SetText(sf::util::Utf8ToWstring(info.title));
            songTitleText->SetVisible(true);
        }
        if (artistText) {
            artistText->SetText(sf::util::Utf8ToWstring(info.artist));
            artistText->SetVisible(true);
        }

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

        if (currentType == LoadingType::Common) {
            if (songTitleText) songTitleText->SetVisible(false);
            if (artistText) artistText->SetVisible(false);
            if (jacketImage) jacketImage->SetVisible(false);
        }
    }

    void LoadingCanvas::Update(const sf::command::ICommand&) {
        // Animation
        animationTimer += sf::Time::DeltaTime();

        if (loadingText) {
            float alpha = 0.5f + 0.5f * std::sin(animationTimer * 10.0f);
            loadingText->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }
        
        // Logic (Transition) removed from here.
    }

    void LoadingCanvas::Draw() {
        sf::ui::Canvas::Draw();
        DrawLoadingGauge();
    }

    void LoadingCanvas::DrawLoadingGauge() {
        float ratio = 0.0f;
        float logicTimer = 0.0f; // For shader effect
        
        if (presenter) {
            ratio = presenter->GetProgress();
            logicTimer = presenter->GetTimer();
        }
        
        // Clamp
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;

        // Visual Layout
        float barWidth = 800.0f; 
        float barHeight = 20.0f;
        float baseY = -400.0f; 
        float centerX = 400.0f; 

        auto* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetContext();

        dx11->gsScoreGauge.SetGPU(dx11->GetMainDevice());
        dx11->psScoreGauge.SetGPU(dx11->GetMainDevice());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // --- Layer 1: Background ---
        {
            float scaleX = barWidth * 0.5f;
            float scaleY = barHeight * 0.5f;
            
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(centerX, baseY, 0.0f); 

            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            mtl material;
            material.diffuseColor = { 0.2f, 0.2f, 0.2f, 0.8f }; 
            float aspect = barWidth / barHeight;
            material.emissionColor = { aspect, 0, 0, 0 }; 
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            dx11->vsNone.SetGPU(dx11->GetMainDevice());
            context->Draw(1, 0);
        }

        // --- Layer 2: Foreground (Rainbow) ---
        if (ratio > 0.001f) {
            float currentWidth = barWidth * ratio;
            float scaleX = currentWidth * 0.5f;
            float scaleY = barHeight * 0.5f;

            float leftEdge = centerX - barWidth * 0.5f;
            float fgCenterX = leftEdge + currentWidth * 0.5f;

            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(fgCenterX, baseY, 0.0f);

            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f }; 
            
            float aspect = currentWidth / barHeight;
            if (aspect < 1.0f) aspect = 1.0f; 

            // Use logicTimer for rainbow effect
            material.emissionColor = { aspect, 1.0f, logicTimer, 0 }; 
            
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            context->Draw(1, 0);
        }

        context->GSSetShader(nullptr, nullptr, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        dx11->ps2d.SetGPU(dx11->GetMainDevice());
    }
}