#pragma once
#include "Image.h"
#include "Texture.h"
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_3.h>   // ★追加: これがないと IDWriteFontCollection1 が使えません
#include <wrl/client.h>
#include <string>

namespace sf {
    namespace ui {

        class TextImage : public Image {
        public:
            TextImage();
            virtual ~TextImage();

            bool Create(ID3D11Device* device,
                const std::wstring& text,
                const std::wstring& fontPathOrName = L"Meiryo UI",
                float fontSize = 64.f,
                const D2D1_COLOR_F& color = D2D1::ColorF(D2D1::ColorF::White),
                UINT texWidth = 1024, UINT texHeight = 256);

            bool SetText(const std::wstring& newText);
            void Draw() override;

        private:
            // ★第2引数に std::wstring& を追加
            bool LoadFontFile(const std::wstring& fontPath, std::wstring& outFontFamilyName);

            class InlineTexture : public sf::Texture {
            public:
                InlineTexture() = default;
                ~InlineTexture() override {
                    if (srv) { srv->Release(); srv = nullptr; }
                }
                void Adopt(ID3D11ShaderResourceView* newSrv) {
                    if (srv) { srv->Release(); srv = nullptr; }
                    srv = newSrv;
                    if (srv) srv->AddRef();
                }
            };

            InlineTexture inlineTex;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

            // ★ Static factories to prevent resource exhaustion
            static Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
            static Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory;

            Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;

            // ★ IDWriteFontCollection1 に修正
            Microsoft::WRL::ComPtr<IDWriteFontCollection1> customFontCollection;

            ID3D11Device* deviceRef = nullptr;
            UINT width = 0;
            UINT height = 0;
            std::wstring currentText;
        };

    }
}