#include "D.h"
#include "D3D.h"
#include "DirectWriteCustomFont.h"
#include <d2d1.h>
#include "SwapChain.h"

extern sf::dx::SwapChain* gSwapChain; // 実体定義はどこか1cppにある前提

namespace app {
    namespace test {

        void D::Begin()
        {
            mDWrite = std::make_unique<DirectWriteCustomFont>(&mFont);
            mDWrite->Init(gSwapChain->GetSwapChain());

            // 日本語ロケールのファミリ名リストから先頭を採用（安全）
            mDWrite->GetFontFamilyName(mDWrite->fontCollection.Get(), L"ja-JP");
            mFont.font = mDWrite->GetFontName(0);
            mFont.fontSize = 60.f;
            mFont.fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
            mFont.Color = D2D1::ColorF(D2D1::ColorF::Red);
            mDWrite->SetFont(mFont);
        }


        void D::OnResize()
        {
            if (mDWrite) {
                // バックバッファ差し替えに追従
                mDWrite->Init(gSwapChain->GetSwapChain());
            }
        }

        void D::Update(const sf::command::ICommand& /*command*/)
        {
            if (!mDWrite) return;
            mDWrite->DrawString(mText, mRect, D2D1_DRAW_TEXT_OPTIONS_NONE, mShadow);
        }

    }
} // namespace app::ui
