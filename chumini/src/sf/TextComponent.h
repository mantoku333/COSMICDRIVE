#pragma once
#include "App.h"
#include "TextImage.h"
#include "DWriteContext.h"
#include "DirectX11.h"

namespace sf::ui
{
    class TextComponent : public sf::Component
    {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand& command);

        // テキスト設定API
        void SetText(const std::wstring& newText);
        void SetFont(const std::wstring& fontName);
        void SetFontSize(float size);
        void SetColor(const D2D1::ColorF& color);

    private:
        std::wstring text = L"";
        std::wstring font = L"叛逆明朝";
        float fontSize = 80.0f;
        D2D1::ColorF color = D2D1::ColorF(D2D1::ColorF::White);

        sf::ui::TextImage* textImage = nullptr;
        sf::command::Command<> updateCommand;
    };
}
