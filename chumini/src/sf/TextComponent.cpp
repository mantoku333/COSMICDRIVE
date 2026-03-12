//#include "TextComponent.h"
//
//using namespace sf::ui;
//
//void TextComponent::Begin()
//{
//    auto* dx11 = sf::dx::DirectX11::Instance();
//
//    // TextImageを生成してActorのTransformに合わせる
//    textImage = AddUI<TextImage>(); // Canvas上で動くUI部品として追加
//    textImage->transform = GetTransform(); // ActorのTransformをコピー
//    textImage->Create(
//        dx11->GetMainDevice().GetDevice(),
//        text,
//        font,
//        fontSize,
//        color,
//        1024, 256
//    );
//
//    updateCommand.Bind(std::bind(&TextComponent::Update, this, std::placeholders::_1));
//}
//
//void TextComponent::Update(const sf::command::ICommand&)
//{
//    if (!textImage) return;
//
//    // ActorのTransformに追従
//    textImage->transform = GetTransform();
//}
//
//// ------------------------------------
//// API: 外部からテキストを動的変更
//// ------------------------------------
//void TextComponent::SetText(const std::wstring& newText)
//{
//    text = newText;
//    if (textImage)
//        textImage->SetText(newText);
//}
//
//void TextComponent::SetFont(const std::wstring& fontName)
//{
//    font = fontName;
//    if (textImage)
//        textImage->SetFont(fontName);
//}
//
//void TextComponent::SetFontSize(float size)
//{
//    fontSize = size;
//    if (textImage)
//        textImage->SetFontSize(size);
//}
//
//void TextComponent::SetColor(const D2D1::ColorF& newColor)
//{
//    color = newColor;
//    if (textImage)
//        textImage->SetColor(newColor);
//}
