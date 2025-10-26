#pragma once
#include "Image.h"
#include "Texture.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace sf {
    namespace ui {

        class TextImage : public Image {
        public:
            TextImage();
            ~TextImage();

            bool Create(ID3D11Device* device,
                const std::wstring& text,
                const std::wstring& fontName = L"Meiryo UI",
                float fontSize = 64.f,
                const D2D1_COLOR_F& color = D2D1::ColorF(D2D1::ColorF::White),
                UINT texWidth = 1024, UINT texHeight = 256);


            bool SetText(const std::wstring& newText);

        private:
            // ★ sf::Textureを継承したダミークラス（SRVを直接保持する）
            class InlineTexture : public sf::Texture {
            public:
                InlineTexture() = default;
                ~InlineTexture() override { Release(); }

                void Adopt(ID3D11ShaderResourceView* newSrv) {
                    SAFE_RELEASE(srv);
                    srv = newSrv;
                    if (srv) srv->AddRef();
                }
            };

            InlineTexture inlineTex;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

            std::wstring currentFontName;
            float currentFontSize = 0.f;
            D2D1_COLOR_F currentColor = D2D1::ColorF(D2D1::ColorF::White);
            UINT width = 0, height = 0;
            ID3D11Device* deviceRef = nullptr; // Create()でもらったdeviceを保持
        };

    }
} // namespace
