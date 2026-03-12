#include "DWriteContext.h"
#include "DirectX11.h"
#include <d2d1helper.h>
#include <dwrite.h>
#include "Debug.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace sf::ui;

// ===== Static Member Definition =====
Microsoft::WRL::ComPtr<ID2D1Factory>      DWriteContext::sD2DFactory;
Microsoft::WRL::ComPtr<IDWriteFactory>    DWriteContext::sDWFactory;
Microsoft::WRL::ComPtr<ID2D1RenderTarget> DWriteContext::sRT;
int DWriteContext::sBeginCount = 0;
IDXGISwapChain* DWriteContext::sCachedSwapChain = nullptr;
bool DWriteContext::sNeedsRecreate = false;

// Screen Size Static Variables
static float sScreenW = 0.f;
static float sScreenH = 0.f;


bool DWriteContext::InitFromSwapChain(IDXGISwapChain* swapChain)
{
    sf::debug::Debug::Log("[DWrite] InitFromSwapChain Start");
    
    if (!swapChain) {
        sf::debug::Debug::LogError("[DWrite] InitFromSwapChain: swapChain is null");
        return false;
    }
    
    // Cache SwapChain info for auto-recovery
    sCachedSwapChain = swapChain;

    if (!sD2DFactory) {
        sf::debug::Debug::Log("[DWrite] Creating D2D1Factory...");
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, sD2DFactory.ReleaseAndGetAddressOf()))) {
            sf::debug::Debug::LogError("[DWrite] Failed to create D2D1Factory");
            return false;
        }
        sf::debug::Debug::LogSafety("[DWrite] D2D1Factory Created");
    }
    if (!sDWFactory) {
        sf::debug::Debug::Log("[DWrite] Creating DWriteFactory...");
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            (IUnknown**)sDWFactory.ReleaseAndGetAddressOf()))) {
            sf::debug::Debug::LogError("[DWrite] Failed to create DWriteFactory");
            return false;
        }
        sf::debug::Debug::LogSafety("[DWrite] DWriteFactory Created");
    }

    bool result = RecreateFromSwapChain(swapChain);
    if (result) {
        sf::debug::Debug::LogSafety("[DWrite] InitFromSwapChain Completed");
    }
    return result;
}

bool DWriteContext::RecreateFromSwapChain(IDXGISwapChain* swapChain)
{
    sf::debug::Debug::Log("[DWrite] RecreateFromSwapChain Start");
    
    if (!swapChain) {
        sf::debug::Debug::LogError("[DWrite] RecreateFromSwapChain: swapChain is null");
        return false;
    }
    
    // Update SwapChain reference
    sCachedSwapChain = swapChain;
    
    // Reset existing RenderTarget
    sRT.Reset();
    sNeedsRecreate = false;

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) {
        sf::debug::Debug::LogError("[DWrite] GetBuffer Failed: HRESULT=" + std::to_string(hr));
        return false;
    }

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f, 96.f);

    hr = sD2DFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, sRT.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        sf::debug::Debug::LogError("[DWrite] CreateDxgiSurfaceRenderTarget Failed: HRESULT=" + std::to_string(hr));
        return false;
    }

    sf::debug::Debug::LogSafety("[DWrite] RenderTarget Recreated Successfully");
    return true;
}

void DWriteContext::FrameBegin()
{
    // Try auto-recovery if RT is invalid
    if (!sRT) {
        if (sNeedsRecreate || sCachedSwapChain) {
            sf::debug::Debug::LogWarning("[DWrite] FrameBegin: RT is null - Trying auto-recovery");
            if (!TryAutoRecreate()) {
                sf::debug::Debug::LogError("[DWrite] FrameBegin: Auto-recovery failed");
                return;
            }
        } else {
            return;
        }
    }
    
    if (sBeginCount == 0) {
        sRT->BeginDraw();
    }
    ++sBeginCount;
}

void DWriteContext::FrameEnd()
{
    if (!sRT) {
        sf::debug::Debug::LogWarning("[DWrite] FrameEnd: RT is null");
        return;
    }
    if (sBeginCount <= 0) {
        sf::debug::Debug::LogWarning("[DWrite] FrameEnd: BeginDraw was not called");
        return;
    }
    --sBeginCount;
    if (sBeginCount == 0) {
        HRESULT hr = sRT->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            sf::debug::Debug::LogWarning("[DWrite] D2DERR_RECREATE_TARGET Detected - Will try auto-recovery next frame");
            sNeedsRecreate = true;
            sRT.Reset();
            
            // Try auto-recovery immediately
            if (TryAutoRecreate()) {
                sf::debug::Debug::LogSafety("[DWrite] Auto-recovery from D2DERR_RECREATE_TARGET Succeeded");
            }
        } else if (FAILED(hr)) {
            sf::debug::Debug::LogError("[DWrite] EndDraw Failed: HRESULT=" + std::to_string(hr));
        }
    }
}

// ===== Robustness: Check and Recovery =====

bool DWriteContext::IsReady()
{
    return sRT != nullptr && sD2DFactory != nullptr && sDWFactory != nullptr && !sNeedsRecreate;
}

void DWriteContext::Reset()
{
    sf::debug::Debug::Log("[DWrite] Reset Called - Clearing all resources");
    sRT.Reset();
    sBeginCount = 0;
    sNeedsRecreate = false;
    // Keep Factory (Reusable)
}

bool DWriteContext::NeedsRecreate()
{
    return sNeedsRecreate || !sRT;
}

bool DWriteContext::TryAutoRecreate()
{
    // ★Always fetch the latest SwapChain from the engine core
    // This fixes the "zombie" issue where cached pointer becomes invalid
    auto* dx = sf::dx::DirectX11::Instance();
    if (!dx) {
         sf::debug::Debug::LogError("[DWrite] TryAutoRecreate: DirectX11 Instance is null");
         return false;
    }

    IDXGISwapChain* liveSwapChain = dx->GetMainDevice().GetSwapChain();
    if (!liveSwapChain) {
        sf::debug::Debug::LogError("[DWrite] TryAutoRecreate: Live SwapChain is null");
        return false;
    }
    
    sf::debug::Debug::Log("[DWrite] TryAutoRecreate: Fetching live SwapChain from DirectX11...");
    
    bool result = RecreateFromSwapChain(liveSwapChain);
    if (result) {
        sNeedsRecreate = false;
        sf::debug::Debug::LogSafety("[DWrite] TryAutoRecreate: Auto-recovery Succeeded with live SwapChain");
    } else {
        sf::debug::Debug::LogError("[DWrite] TryAutoRecreate: Auto-recovery Failed");
    }
    return result;
}

// ===== Accessors =====

ID2D1RenderTarget* DWriteContext::RT() { return sRT.Get(); }
ID2D1Factory* DWriteContext::D2DFactory() { return sD2DFactory.Get(); }
IDWriteFactory* DWriteContext::DWriteFactory() { return sDWFactory.Get(); }

void DWriteContext::SetScreenSize(float w, float h) { sScreenW = w; sScreenH = h; }
float DWriteContext::ScreenW() { return sScreenW; }
float DWriteContext::ScreenH() { return sScreenH; }
