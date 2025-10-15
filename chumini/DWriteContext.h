#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace sf {
    namespace ui {

        class DWriteContext {
        public:
            // 一度だけ呼ぶ（SwapChainからD2Dの描画先を作る）
            static bool InitFromSwapChain(IDXGISwapChain* swapChain);

            // バックバッファがリサイズされたら呼ぶ
            static bool RecreateFromSwapChain(IDXGISwapChain* swapChain);

            // フレームの開始/終了（複数Textがあっても多重Begin/Endしないように参照カウント）
            static void FrameBegin();
            static void FrameEnd();

            // 共有オブジェクト（Textから使う）
            static ID2D1RenderTarget* RT();
            static ID2D1Factory* D2DFactory();
            static IDWriteFactory* DWriteFactory();

            static void SetScreenSize(float w, float h);
            static float ScreenW();
            static float ScreenH();

        private:
            static Microsoft::WRL::ComPtr<ID2D1Factory>       sD2DFactory;
            static Microsoft::WRL::ComPtr<IDWriteFactory>     sDWFactory;
            static Microsoft::WRL::ComPtr<ID2D1RenderTarget>  sRT;

            


            static int sBeginCount; // Begin/Endのネスト防止
        };

    }
} // namespace
