#include "Text.h"
#include "DWriteContext.h"
#include "Debug.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

using namespace sf::ui;

Text::Text() : UI()
{
    // Initial format (Can be delayed, but create here)
    EnsureFormat();
}

void Text::EnsureFormat()
{
    mFormat.Reset();
    auto* dw = DWriteContext::DWriteFactory();
    if (!dw) {
        sf::debug::Debug::LogWarning("[Text] EnsureFormat: DWriteFactory is null");
        return;
    }

    HRESULT hr = dw->CreateTextFormat(
        mFont.c_str(), nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        mSize,
        L"ja-jp",
        mFormat.ReleaseAndGetAddressOf());

    if (FAILED(hr)) {
        sf::debug::Debug::LogError("[Text] EnsureFormat: CreateTextFormat Failed HRESULT=" + std::to_string(hr));
        return;
    }

    // Default Alignment
    if (mFormat) {
        mFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        mFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
}

void Text::Draw()
{
    // Return if text is empty or invalid
    if (!enable || mText.empty()) return;
    
    // ===== Robustness: Check DWriteContext Validity =====
    if (!DWriteContext::IsReady()) {
        // Try Auto-Recovery
        if (DWriteContext::NeedsRecreate()) {
            sf::debug::Debug::LogWarning("[Text] Draw: DWriteContext invalid - Trying auto-recovery");
            if (!DWriteContext::TryAutoRecreate()) {
                sf::debug::Debug::LogError("[Text] Draw: DWriteContext auto-recovery failed - Skipping Draw");
                return;
            }
        } else {
            // RT might not be initialized yet
            return;
        }
    }
    
    auto* rt = DWriteContext::RT();
    if (!rt) {
        sf::debug::Debug::LogWarning("[Text] Draw: RenderTarget is null - Skipping Draw");
        return;
    }

    DWriteContext::FrameBegin();

    // Create Brush (Add Failure Check)
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = rt->CreateSolidColorBrush(mColor, brush.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !brush) {
        sf::debug::Debug::LogError("[Text] Draw: CreateSolidColorBrush Failed HRESULT=" + std::to_string(hr));
        DWriteContext::FrameEnd();
        return;
    }

    // === Coordinate Conversion Start ===
    const float sw = DWriteContext::ScreenW();
    const float sh = DWriteContext::ScreenH();
    if (sw <= 0.f || sh <= 0.f) { 
        sf::debug::Debug::LogWarning("[Text] Draw: Invalid Screen Size (" + std::to_string(sw) + "x" + std::to_string(sh) + ")");
        DWriteContext::FrameEnd(); 
        return; 
    }

    const Vector3 pos = transform.GetPosition();
    const Vector3 sca = transform.GetScale();

    // UI Center-Origin/Y-Up -> D2D Top-Left/Y-Down Conversion
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
    // === Coordinate Conversion End ===

    // Check Format Validity
    if (!mFormat) {
        EnsureFormat();
        if (!mFormat) {
            sf::debug::Debug::LogError("[Text] Draw: Cannot create mFormat - Skipping Draw");
            DWriteContext::FrameEnd();
            return;
        }
    }

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
