#include "Text.h"
#include "DWriteContext.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

using namespace sf::ui;

Text::Text() : UI()
{
    // 初期フォーマット（遅延生成でもOKだが、ここで作っておく）
    EnsureFormat();
}

void Text::EnsureFormat()
{
    mFormat.Reset();
    auto* dw = DWriteContext::DWriteFactory();
    if (!dw) return;

    dw->CreateTextFormat(
        mFont.c_str(), nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        mSize,
        L"ja-jp",
        mFormat.ReleaseAndGetAddressOf());

    // デフォの揃え
    if (mFormat) {
        mFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        mFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
}

void Text::Draw()
{
    if (!enable || mText.empty()) return;
    auto* rt = DWriteContext::RT();
    if (!rt) return;

    DWriteContext::FrameBegin();

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(mColor, brush.ReleaseAndGetAddressOf());

    // === ここから座標変換 ===
    const float sw = DWriteContext::ScreenW();
    const float sh = DWriteContext::ScreenH();
    if (sw <= 0.f || sh <= 0.f) { DWriteContext::FrameEnd(); return; }

    const Vector3 pos = transform.GetPosition();
    const Vector3 sca = transform.GetScale();

    // UIは中心原点・Y上、D2Dは左上原点・Y下 → 変換
    const float cx = sw * 0.5f;
    const float cy = sh * 0.5f;
    const float screenX = cx + pos.x;
    const float screenY = cy - pos.y;

    const float w = width * sca.x;
    const float h = height * sca.y;

    D2D1_RECT_F rect = D2D1::RectF(
        screenX - w * 0.5f,
        screenY - h * 0.5f,
        screenX + w * 0.5f,
        screenY + h * 0.5f);
    // === ここまで座標変換 ===

    if (!mFormat) EnsureFormat();

    rt->DrawText(
        mText.c_str(),
        static_cast<UINT32>(mText.size()),
        mFormat.Get(),
        rect,
        brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP,
        DWRITE_MEASURING_MODE_NATURAL);

    DWriteContext::FrameEnd();
}

void Text::SetText(const std::wstring& t)
{
    mText = t;
}

void Text::SetFont(const std::wstring& name)
{
    if (mFont == name) return;
    mFont = name;
    EnsureFormat();
}

void Text::SetSize(float px)
{
    if (mSize == px) return;
    mSize = px;
    EnsureFormat();
}

void Text::SetColor(const D2D1_COLOR_F& c)
{
    mColor = c;
}

void Text::SetAlign(DWRITE_TEXT_ALIGNMENT hAlign, DWRITE_PARAGRAPH_ALIGNMENT vAlign)
{
    if (!mFormat) EnsureFormat();
    if (mFormat) {
        mFormat->SetTextAlignment(hAlign);
        mFormat->SetParagraphAlignment(vAlign);
    }
}
