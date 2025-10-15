#include "TextImage.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

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

    // --- D2D/Write ファクトリ ---
    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf())))
        return false;

    Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
        return false;

    // --- D3D11 テクスチャ作成（BGRA / premultiplied対応で描く） ---
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = texWidth;
    desc.Height = texHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    if (FAILED(device->CreateTexture2D(&desc, nullptr, tex.ReleaseAndGetAddressOf())))
        return false;

    // DXGI surface → D2D RT
    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    if (FAILED(tex.As(&surface))) return false;

    Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);

    if (FAILED(d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf())))
        return false;

    // ブラシ & フォーマット
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(rt->CreateSolidColorBrush(color, brush.GetAddressOf()))) return false;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    if (FAILED(dwFactory->CreateTextFormat(
        fontName.c_str(), nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"ja-jp", format.GetAddressOf()))) return false;

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // 描画
    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0)); // 完全透明でクリア
    rt->DrawText(text.c_str(), static_cast<UINT32>(text.size()), format.Get(),
        D2D1::RectF(0, 0, static_cast<FLOAT>(texWidth), static_cast<FLOAT>(texHeight)),
        brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
    HRESULT hr = rt->EndDraw();
    if (FAILED(hr)) return false;

    // SRV作成
    if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf())))
        return false;

    // InlineTexture に SRV を渡す
    inlineTex.Adopt(srv.Get());

    // Material の SafePtr<sf::Texture> に渡す
    material.texture = &inlineTex;  // ← 型一致

    return true;
}
