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
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf());

    Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()));

    // --- テクスチャ作成 ---
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = texWidth;
    desc.Height = texHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    device->CreateTexture2D(&desc, nullptr, tex.ReleaseAndGetAddressOf());

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    tex.As(&surface);

    Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);
    d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf());

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(color, brush.GetAddressOf());

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    dwFactory->CreateTextFormat(
        fontName.c_str(), nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"ja-jp", format.GetAddressOf());
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(text.c_str(), (UINT32)text.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)texWidth, (FLOAT)texHeight),
        brush.Get());
    rt->EndDraw();

    device->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf());
    inlineTex.Adopt(srv.Get());
    material.texture = &inlineTex;

    // SetTextで使う情報を保存
    deviceRef = device;
    currentFontName = fontName;
    currentFontSize = fontSize;
    currentColor = color;
    width = texWidth;
    height = texHeight;

    return true;
}


bool TextImage::SetText(const std::wstring& newText)
{
    if (!tex || width == 0 || height == 0 || !deviceRef)
        return false;

    // D2D/DWrite Factory
    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf())))
        return false;

    Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
        return false;

    // DXGI Surface
    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    if (FAILED(tex.As(&surface))) return false;

    Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);
    if (FAILED(d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf())))
        return false;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(currentColor, brush.GetAddressOf());

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    dwFactory->CreateTextFormat(
        currentFontName.c_str(), nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        currentFontSize, L"ja-jp", format.GetAddressOf());
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // 再描画
    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(newText.c_str(), (UINT32)newText.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), brush.Get());
    rt->EndDraw();

    // SRV再作成
    deviceRef->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf());
    inlineTex.Adopt(srv.Get());
    material.texture = &inlineTex;
    return true;
}