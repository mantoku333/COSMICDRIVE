#include "TextImage.h"
#include <d2d1helper.h>
#include <vector>
#include "Debug.h" // Added for debugging
#include "DirectX11.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace sf::ui;

#include <mutex> // Added for thread safety

// ★ Static Member Definition
Microsoft::WRL::ComPtr<ID2D1Factory> TextImage::d2dFactory;
Microsoft::WRL::ComPtr<IDWriteFactory> TextImage::dwFactory;
static std::mutex s_factoryMutex; // Protects initialization of d2dFactory and dwFactory

TextImage::TextImage() {}
TextImage::~TextImage() {}

bool TextImage::LoadFontFile(const std::wstring& fontPath, std::wstring& outFontFamilyName) {
    if (fontPath.empty()) return false;
    
    // Ensure factories are created before using them in LoadFontFile
    // Though usually Create() is called first, this safety check is good.
    {
        std::lock_guard<std::mutex> lock(s_factoryMutex);
        if (!dwFactory) {
             if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
             {
                 sf::debug::Debug::LogError("TextImage: Failed to create DWriteFactory.");
                 return false;
             }
        }
    }

    Microsoft::WRL::ComPtr<IDWriteFactory5> dwFactory5;
    if (FAILED(dwFactory.As(&dwFactory5))) {
        sf::debug::Debug::LogError("TextImage: Failed to get IDWriteFactory5 interface.");
        return false;
    }

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
    if (FAILED(dwFactory5->CreateFontSetBuilder(&fontSetBuilder))) return false;

    Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
    if (FAILED(dwFactory5->CreateFontFileReference(fontPath.c_str(), nullptr, &fontFile))) {
        sf::debug::Debug::LogError("TextImage: Failed to create FontFileReference: " + std::string(fontPath.begin(), fontPath.end()));
        return false;
    }

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

    // Double-checked locking pattern or just simple lock for safety
    {
        std::lock_guard<std::mutex> lock(s_factoryMutex);
        if (!d2dFactory) {
            // Changed to MULTI_THREADED
            if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, d2dFactory.GetAddressOf())))
            {
                sf::debug::Debug::LogError("TextImage: Failed to create D2D1Factory.");
                return false;
            }
        }
        if (!dwFactory) {
            if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()))))
            {
                sf::debug::Debug::LogError("TextImage: Failed to create DWriteFactory.");
                return false;
            }
        }
    }

    rt.Reset();
    brush.Reset();
    format.Reset();
    tex.Reset();
    srv.Reset();
    customFontCollection.Reset();

    // Factory creation removed from here.

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

    if (FAILED(device->CreateTexture2D(&desc, nullptr, tex.ReleaseAndGetAddressOf()))) {
        sf::debug::Debug::LogError("TextImage: Failed to create Texture2D.");
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    if (FAILED(tex.As(&surface))) return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);

    if (FAILED(d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, rt.GetAddressOf()))) {
        sf::debug::Debug::LogError("TextImage: Failed to create DxgiSurfaceRenderTarget.");
        return false;
    }

    if (FAILED(rt->CreateSolidColorBrush(color, brush.GetAddressOf()))) {
        sf::debug::Debug::LogError("TextImage: Failed to create SolidColorBrush.");
        return false;
    }

    // customFontCollection.Get() は IDWriteFontCollection* として渡せるのでOK
    if (FAILED(dwFactory->CreateTextFormat(
        actualFontName.c_str(),
        customFontCollection.Get(),
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"ja-jp", format.GetAddressOf()))) 
    {
        sf::debug::Debug::LogError("TextImage: Failed to create TextFormat. Font: " + std::string(actualFontName.begin(), actualFontName.end()));
        return false;
    }

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    currentText = text; // Initialize cache
    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(text.c_str(), (UINT32)text.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), brush.Get());
    HRESULT hrEnd = rt->EndDraw();
    if (FAILED(hrEnd)) {
        sf::debug::Debug::LogError("TextImage: Create() EndDraw failed. HRESULT: " + std::to_string(hrEnd));
        // We typically shouldn't fail the whole create just because drawing the initial text failed, 
        // but it indicates a rendering issue.
    }

    if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv.ReleaseAndGetAddressOf())))
        return false;

    inlineTex.Adopt(srv.Get());
    material.texture = &inlineTex;

    return true;
}

bool TextImage::SetText(const std::wstring& newText) {
    if (newText == currentText) return true; // Optimization: Skip redraw if text hasn't changed
    
    if (!rt || !brush || !format) return false;
    
    currentText = newText; // Update cache

    rt->BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0));
    rt->DrawText(newText.c_str(), (UINT32)newText.size(), format.Get(),
        D2D1::RectF(0, 0, (FLOAT)width, (FLOAT)height), brush.Get());
    HRESULT hr = rt->EndDraw();
    if (FAILED(hr)) {
        sf::debug::Debug::LogError("TextImage: SetText EndDraw failed. HRESULT: " + std::to_string(hr));
        return false;
    }
    return true;
}

void TextImage::Draw() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    if (!dx11) return;
    auto context = dx11->GetMainDevice().GetContext();

    // Lazy create premultiplied blend state
    static Microsoft::WRL::ComPtr<ID3D11BlendState> s_premulBlendState;
    static std::once_flag s_blendStateFlag;
    std::call_once(s_blendStateFlag, [&]() {
        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE; // Premultiply One
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        dx11->GetMainDevice().GetDevice()->CreateBlendState(&desc, s_premulBlendState.GetAddressOf());
    });

    // Save old state
    Microsoft::WRL::ComPtr<ID3D11BlendState> oldState;
    FLOAT oldFactor[4];
    UINT oldMask;
    context->OMGetBlendState(oldState.GetAddressOf(), oldFactor, &oldMask);

    // Set Premul State
    if (s_premulBlendState) {
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->OMSetBlendState(s_premulBlendState.Get(), blendFactor, 0xFFFFFFFF);
    }

    // Draw using base Image class
    Image::Draw();

    // Restore old state
    context->OMSetBlendState(oldState.Get(), oldFactor, oldMask);
}