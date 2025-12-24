#include "TextImage.h"
#include <d2d1helper.h>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace sf::ui;

TextImage::TextImage() {}
TextImage::~TextImage() {}

bool TextImage::LoadFontFile(const std::wstring& fontPath, std::wstring& outFontFamilyName) {
    if (fontPath.empty()) return false;

    Microsoft::WRL::ComPtr<IDWriteFactory5> dwFactory5;
    if (FAILED(dwFactory.As(&dwFactory5))) return false;

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
    if (FAILED(dwFactory5->CreateFontSetBuilder(&fontSetBuilder))) return false;

    Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
    if (FAILED(dwFactory5->CreateFontFileReference(fontPath.c_str(), nullptr, &fontFile))) return false;

    if (FAILED(fontSetBuilder->AddFontFile(fontFile.Get()))) return false;

    Microsoft::WRL::ComPtr<IDWriteFontSet> fontSet;
    if (FAILED(fontSetBuilder->CreateFontSet(&fontSet))) return false;

    // ★ ComPtr の安全なアドレス取得を使用
    if (FAILED(dwFactory5->CreateFontCollectionFromFontSet(fontSet.Get(), customFontCollection.ReleaseAndGetAddressOf())))
        return false;

    if (customFontCollection->GetFontFamilyCount() == 0) return false;

    Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
    if (FAILED(customFontCollection->GetFontFamily(0, &fontFamily))) return false;

    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> familyNames;
    if (FAILED(fontFamily->GetFamilyNames(&familyNames))) return false;

    UINT32 index = 0;
    BOOL exists = false;
    familyNames->FindLocaleName(L"ja-jp", &index, &exists);
    if (!exists) index = 0;

    UINT32 length = 0;
    familyNames->GetStringLength(index, &length);
    std::vector<wchar_t> buffer(length + 1);
    familyNames->GetString(index, buffer.data(), length + 1);

    outFontFamilyName = buffer.data();
    return true;
}

bool TextImage::Create(ID3D11Device* device,
    const std::wstring& text,
    const std::wstring& fontPathOrName,
    float fontSize,
    const D2D1_COLOR_F& color,
    UINT texWidth, UINT texHeight)
{
    if (!device) return false;

    deviceRef = device;
    width = texWidth;
    height = texHeight;

    d2dFactory.Reset();
    dwFactory.Reset();
    rt.Reset();
    brush.Reset();
    format.Reset();
    tex.Reset();
    srv.Reset();
    customFontCollection.Reset();

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf())))
        return false;

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
        return false;

    std::wstring actualFontName = fontPathOrName;
    if (fontPathOrName.find(L".ttf") != std::wstring::npos || fontPathOrName.find(L".otf") != std::wstring::npos) {
        if (!LoadFontFile(fontPathOrName, actualFontName)) {
            actualFontName = L"Meiryo UI";
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = texWidth; desc.Height = texHeight;
    desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    if (FAILED(device->CreateTexture2D(&desc, nullptr, tex.ReleaseAndGetAddressOf())))
        return false;

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    if (FAILED(tex.As(&surface))) return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);

    if (FAILED(d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf())))
        return false;

    if (FAILED(rt->CreateSolidColorBrush(color, brush.GetAddressOf())))
        return false;

    // customFontCollection.Get() は IDWriteFontCollection* として渡せるのでOK
    if (FAILED(dwFactory->CreateTextFormat(
        actualFontName.c_str(),
        customFontCollection.Get(),
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"ja-jp", format.GetAddressOf())))
        return false;

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(text.c_str(), (UINT32)text.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), brush.Get());
    rt->EndDraw();

    if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf())))
        return false;

    inlineTex.Adopt(srv.Get());
    material.texture = &inlineTex;

    return true;
}

bool TextImage::SetText(const std::wstring& newText) {
    if (!rt || !brush || !format) return false;
    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(newText.c_str(), (UINT32)newText.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), brush.Get());
    return SUCCEEDED(rt->EndDraw());
}