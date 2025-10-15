//#include "TextComponent.h"
//#include "App.h"
//#include "D3D.h"
//#include <d2d1helper.h>
//#include <dwrite.h>
//
//#pragma comment(lib, "d2d1.lib")
//#pragma comment(lib, "dwrite.lib")
//
//using namespace sf::ui;
//
//void TextComponent::Begin() {
//    // ファクトリ
//    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, mD2DFactory.ReleaseAndGetAddressOf());
//    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
//        (IUnknown**)mDWriteFactory.ReleaseAndGetAddressOf());
//
//    // 初期テキストフォーマット
//    mDWriteFactory->CreateTextFormat(
//        mFontName.c_str(), nullptr,
//        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
//        mFontSize, L"ja-jp", mTextFormat.ReleaseAndGetAddressOf());
//
//    // レンダーターゲット生成
//    InitD2DTargets();
//
//    // （必要ならシーンの Update にフックして描画）※あなたのフレームの流儀に合わせてOK
//    updateCommand.Bind(std::bind(&TextComponent::Update, this));
//}
//
//bool TextComponent::InitD2DTargets() {
//    mRenderTarget.Reset();
//    mBrush.Reset();
//
//    auto swapChainWrapper = sf::App::Instance().GetSwapChain();
//    if (!swapChainWrapper) return false;
//    IDXGISwapChain* swapChain = swapChainWrapper->GetSwapChain();
//    if (!swapChain) return false;
//
//    Microsoft::WRL::ComPtr<IDXGISurface> surface;
//    if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&surface)))) return false;
//
//    // D2D RenderTarget（premultiplied）
//    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
//        D2D1_RENDER_TARGET_TYPE_DEFAULT,
//        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
//        96.f, 96.f);
//
//    if (FAILED(mD2DFactory->CreateDxgiSurfaceRenderTarget(
//        surface.Get(), &props, mRenderTarget.ReleaseAndGetAddressOf())))
//        return false;
//
//    if (FAILED(mRenderTarget->CreateSolidColorBrush(mColor, mBrush.ReleaseAndGetAddressOf())))
//        return false;
//
//    return true;
//}
//
//void TextComponent::Update(const sf::command::ICommand&) {
//    if (!mRenderTarget || mText.empty()) return;
//
//    // 3D描画を一通り終えた「後」に呼ぶと、D2Dが上に重ねてくれる
//    mRenderTarget->BeginDraw();
//    mBrush->SetColor(mColor);
//    mTextFormat->SetFontSize(mFontSize);
//
//    mRenderTarget->DrawTextW(
//        mText.c_str(), static_cast<UINT32>(mText.size()),
//        mTextFormat.Get(), mRect, mBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
//
//    // EndDraw 失敗時（リサイズ等）に再作成
//    HRESULT hr = mRenderTarget->EndDraw();
//    if (hr == D2DERR_RECREATE_TARGET) {
//        InitD2DTargets();
//    }
//}
//
//void TextComponent::SetText(const std::wstring& text) { mText = text; }
//void TextComponent::SetColor(const D2D1_COLOR_F& color) {
//    mColor = color; if (mBrush) mBrush->SetColor(color);
//}
//void TextComponent::SetFontSize(float size) {
//    mFontSize = size; if (mTextFormat) mTextFormat->SetFontSize(size);
//}
//void TextComponent::SetFont(const std::wstring& fontName) {
//    mFontName = fontName;
//    if (mDWriteFactory) {
//        mDWriteFactory->CreateTextFormat(
//            mFontName.c_str(), nullptr,
//            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
//            DWRITE_FONT_STRETCH_NORMAL, mFontSize, L"ja-jp",
//            mTextFormat.ReleaseAndGetAddressOf());
//    }
//}
//void TextComponent::SetRect(float x, float y, float w, float h) {
//    mRect = D2D1::RectF(x, y, x + w, y + h);
//}
//void TextComponent::OnResize() {
//    InitD2DTargets();
//}