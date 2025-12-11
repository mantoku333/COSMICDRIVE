#pragma once
#include "Image.h"
#include "Texture.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h> // ComPtr用

namespace sf {
	namespace ui {

		class TextImage : public Image {
		public:
			TextImage();
			virtual ~TextImage();

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

			// D3D11 リソース
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

			// ★追加: D2D リソースをメンバ変数としてキャッシュする（これが重要）
			Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
			Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory;
			Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
			Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
			Microsoft::WRL::ComPtr<IDWriteTextFormat> format;

			// 状態保持用
			ID3D11Device* deviceRef = nullptr;
			UINT width = 0;
			UINT height = 0;
		};

	}
} // namespace