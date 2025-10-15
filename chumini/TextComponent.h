//#pragma once
//#include "Component.h"
//#include <d2d1.h>
//#include <dwrite.h>
//#include <wrl/client.h>
//#include <string>
//#include "App.h"
//
//
//namespace sf {
//    namespace ui {
//
//        class TextComponent : public sf::Component {
//        public:
//            void Begin() override;
//            void Update(const sf::command::ICommand&);
//
//            sf::command::Command<> updateCommand;
//
//            // API
//            void SetText(const std::wstring& text);
//            void SetColor(const D2D1_COLOR_F& color);
//            void SetFontSize(float size);
//            void SetFont(const std::wstring& fontName);
//            void SetRect(float x, float y, float w, float h); // 表示領域
//
//            // リサイズ（SwapChain::ResizeBuffers 後に呼ぶ）
//            void OnResize();
//
//        private:
//            bool InitD2DTargets(); // SwapChain から D2D RT を作り直す
//
//            // 状態
//            std::wstring mText = L"";
//            std::wstring mFontName = L"メイリオ";
//            float mFontSize = 32.0f;
//            D2D1_COLOR_F mColor = D2D1::ColorF(D2D1::ColorF::White);
//            D2D1_RECT_F mRect = D2D1::RectF(100, 100, 900, 180);
//
//            // D2D/DWrite
//            Microsoft::WRL::ComPtr<ID2D1Factory> mD2DFactory;
//            Microsoft::WRL::ComPtr<IDWriteFactory> mDWriteFactory;
//            Microsoft::WRL::ComPtr<ID2D1RenderTarget> mRenderTarget;
//            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mBrush;
//            Microsoft::WRL::ComPtr<IDWriteTextFormat> mTextFormat;
//        };
//
//    } // namespace ui
//} // namespace sf