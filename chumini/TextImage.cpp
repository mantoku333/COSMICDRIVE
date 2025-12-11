#include "TextImage.h"
// D2Dヘルパー (d2d1.h, dwrite.h はヘッダーでinclude済み)
#include <d2d1helper.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace sf::ui;

TextImage::TextImage() {}
TextImage::~TextImage() {}

bool TextImage::Create(ID3D11Device* device,
	const std::wstring& text,
	const std::wstring& fontName,
	float fontSize,
	const D2D1_COLOR_F& color,
	UINT texWidth, UINT texHeight)
{
	if (!device) return false;

	// メンバ変数に保存
	deviceRef = device;
	width = texWidth;
	height = texHeight;

	// ---------------------------------------------------
	// 1. ファクトリの作成 (D2D, DirectWrite)
	// ---------------------------------------------------
	// 既に持っている場合はリセット
	d2dFactory.Reset();
	dwFactory.Reset();
	rt.Reset();
	brush.Reset();
	format.Reset();
	tex.Reset();
	srv.Reset();

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf())))
		return false;

	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
		return false;

	// ---------------------------------------------------
	// 2. D3D11テクスチャの作成
	// ---------------------------------------------------
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texWidth;
	desc.Height = texHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // D2Dと互換性のあるフォーマット
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, tex.ReleaseAndGetAddressOf())))
		return false;

	// ---------------------------------------------------
	// 3. D2D RenderTarget の作成
	// ---------------------------------------------------
	Microsoft::WRL::ComPtr<IDXGISurface> surface;
	if (FAILED(tex.As(&surface))) return false;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		96.f, 96.f);

	if (FAILED(d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf())))
		return false;

	// ---------------------------------------------------
	// 4. ブラシとフォント設定の作成
	// ---------------------------------------------------
	if (FAILED(rt->CreateSolidColorBrush(color, brush.GetAddressOf())))
		return false;

	if (FAILED(dwFactory->CreateTextFormat(
		fontName.c_str(), nullptr,
		DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		fontSize, L"ja-jp", format.GetAddressOf())))
		return false;

	// 配置設定（中央揃え）
	format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// ---------------------------------------------------
	// 5. 初回描画
	// ---------------------------------------------------
	rt->BeginDraw();
	rt->Clear(D2D1::ColorF(0, 0)); // 透明でクリア
	rt->DrawText(
		text.c_str(),
		(UINT32)text.size(),
		format.Get(),
		D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height),
		brush.Get()
	);
	rt->EndDraw();

	// ---------------------------------------------------
	// 6. SRV (Shader Resource View) 作成と適用
	// ---------------------------------------------------
	if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf())))
		return false;

	// 自作TextureクラスにSRVを登録
	inlineTex.Adopt(srv.Get());

	// マテリアルにセット（これで画面に表示される）
	material.texture = &inlineTex;

	return true;
}

bool TextImage::SetText(const std::wstring& newText)
{
	// 必要なリソースがなければ失敗
	if (!rt || !brush || !format) return false;

	// ---------------------------------------------------
	// 再描画処理
	// ---------------------------------------------------
	// ※ ここではリソースの再作成（Create）は一切行わず、
	//    保持している rt, brush, format を使って描くだけにする。

	rt->BeginDraw();

	rt->Clear(D2D1::ColorF(0, 0)); // 前の文字を消す

	rt->DrawText(
		newText.c_str(),
		(UINT32)newText.size(),
		format.Get(),
		D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height),
		brush.Get()
	);

	HRESULT hr = rt->EndDraw();

	// EndDrawした時点で裏のテクスチャ(tex)は更新されているので、
	// SRVを作り直す必要はない。

	return SUCCEEDED(hr);
}