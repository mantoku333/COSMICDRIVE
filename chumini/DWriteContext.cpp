#include "DWriteContext.h"
#include <d2d1helper.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace sf::ui;

Microsoft::WRL::ComPtr<ID2D1Factory>      DWriteContext::sD2DFactory;
Microsoft::WRL::ComPtr<IDWriteFactory>    DWriteContext::sDWFactory;
Microsoft::WRL::ComPtr<ID2D1RenderTarget> DWriteContext::sRT;
int DWriteContext::sBeginCount = 0;

// 先頭の静的変数に追記
static float sScreenW = 0.f;
static float sScreenH = 0.f;



bool DWriteContext::InitFromSwapChain(IDXGISwapChain* swapChain)
{
    if (!swapChain) return false;

    if (!sD2DFactory) {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, sD2DFactory.ReleaseAndGetAddressOf())))
            return false;
    }
    if (!sDWFactory) {
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            (IUnknown**)sDWFactory.ReleaseAndGetAddressOf())))
            return false;
    }

    return RecreateFromSwapChain(swapChain);
}

bool DWriteContext::RecreateFromSwapChain(IDXGISwapChain* swapChain)
{
    sRT.Reset();

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&surface))))
        return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);

    if (FAILED(sD2DFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, sRT.ReleaseAndGetAddressOf())))
        return false;

    return true;
}

void DWriteContext::FrameBegin()
{
    if (!sRT) return;
    if (sBeginCount == 0) sRT->BeginDraw();
    ++sBeginCount;
}

void DWriteContext::FrameEnd()
{
    if (!sRT) return;
    if (sBeginCount <= 0) return;
    --sBeginCount;
    if (sBeginCount == 0) {
        HRESULT hr = sRT->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            // 呼び出し元で RecreateFromSwapChain() を呼んでね（Resize後など）
        }
    }
}

ID2D1RenderTarget* DWriteContext::RT() { return sRT.Get(); }
ID2D1Factory* DWriteContext::D2DFactory() { return sD2DFactory.Get(); }
IDWriteFactory* DWriteContext::DWriteFactory() { return sDWFactory.Get(); }

void DWriteContext::SetScreenSize(float w, float h) { sScreenW = w; sScreenH = h; }
float DWriteContext::ScreenW() { return sScreenW; }
float DWriteContext::ScreenH() { return sScreenH; }
