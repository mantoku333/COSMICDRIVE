#pragma once
#include "App.h"

namespace app::test {
    class SceneChangeComponent : public sf::Component {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);
        
        // 外部から遷移をリクエストする関数
        void ChangeScene(sf::SafePtr<sf::IScene> nextScene);

    private:
        
        sf::command::Command<> updateCommand;
        sf::SafePtr<sf::IScene> nextScene;
        bool isChanging = false; // 遷移処理中かどうかのフラグ
    };
}