#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace sf {
    namespace ui {

        class DWriteContext {
        public:
            // Initialize from SwapChain (creates D2D render target)
            static bool InitFromSwapChain(IDXGISwapChain* swapChain);

            // Recreate resources when backbuffer is resized
            static bool RecreateFromSwapChain(IDXGISwapChain* swapChain);

            // Frame Begin/End (Reference counting to prevent nested Begin/End)
            static void FrameBegin();
            static void FrameEnd();

            // Shared Objects (Used by Text)
            static ID2D1RenderTarget* RT();
            static ID2D1Factory* D2DFactory();
            static IDWriteFactory* DWriteFactory();

            static void SetScreenSize(float w, float h);
            static float ScreenW();
            static float ScreenH();

            // ===== Robustness: Status Check and Auto-Recovery =====
            
            // Check if RenderTarget is valid
            static bool IsReady();
            
            // Reset resources (call on device lost)
            static void Reset();
            
            // Check if recreation is needed
            static bool NeedsRecreate();
            
            // Try auto recreate (when D2DERR_RECREATE_TARGET occurs in FrameEnd)
            static bool TryAutoRecreate();

        private:
            static Microsoft::WRL::ComPtr<ID2D1Factory>       sD2DFactory;
            static Microsoft::WRL::ComPtr<IDWriteFactory>     sDWFactory;
            static Microsoft::WRL::ComPtr<ID2D1RenderTarget>  sRT;
            
            // Keep reference to SwapChain for auto-recovery
            static IDXGISwapChain* sCachedSwapChain;
            
            // Flag indicating recreation is needed
            static bool sNeedsRecreate;

            static int sBeginCount; // Prevent nested Begin/End
        };

    }
} // namespace
