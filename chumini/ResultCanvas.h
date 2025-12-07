#pragma once
#include "App.h"

namespace app::test {
    class ResultCanvas : public sf::ui::Canvas {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);

    private:
        sf::Texture textureBackground;
        sf::Texture textureNumber;

        // 数字描画用のヘルパー関数
        void DrawNumber(int number, float x, float y, float scale = 0.5f);

        // キー入力待ち用
        sf::command::Command<> updateCommand;

        // 次のシーンへ（タイトルやセレクトへ戻る用）
        sf::SafePtr<sf::IScene> nextScene;
    };
}