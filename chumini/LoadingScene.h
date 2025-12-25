#pragma once
#include "App.h"
#include "SongInfo.h"

namespace app::test {
	class LoadingScene :public sf::Scene<LoadingScene> 
    {
    public:
        void Init()override;
        void Update(const sf::command::ICommand& command) override;

        // 次のシーンを予約するための静的メソッド
        static void SetNextScene(sf::SafePtr<sf::IScene> scene);
        static void SetNextSongInfo(const SongInfo& info);

        sf::ref::Ref<sf::Actor> managerActor;

    private:
        sf::command::Command<> updateCommand;

        

        // シーン間で受け渡すための「バトン」
        static sf::SafePtr<sf::IScene> pendingNextScene;
        static SongInfo pendingSongInfo;
    };
}