#pragma once
#include "UI.h"            // あなたのUI基底（Transform含む）
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

namespace sf {
    namespace ui {

        class Text : public UI {
        public:
            Text();
            ~Text() = default;

            // UIの仮想描画（Imageと同格）
            void Draw() override;

            // API
            void SetText(const std::wstring& t);
            void SetFont(const std::wstring& name);     // L"メイリオ" 等
            void SetSize(float px);                      // フォントサイズ
            void SetColor(const D2D1_COLOR_F& c);        // 文字色
            void SetAlign(DWRITE_TEXT_ALIGNMENT hAlign, DWRITE_PARAGRAPH_ALIGNMENT vAlign); // 揃え

        private:
            void EnsureFormat(); // mFormatの作り直し

        private:
            std::wstring mText = L"";
            std::wstring mFont = L"メイリオ";
            float mSize = 32.f;
            D2D1_COLOR_F mColor = D2D1::ColorF(D2D1::ColorF::White);

            // キャッシュ
            Microsoft::WRL::ComPtr<IDWriteTextFormat> mFormat;
        };

    }
} // namespace
