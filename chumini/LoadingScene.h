#pragma once
#include "App.h"
#include "SongInfo.h"

namespace app::test {
    enum class LoadingType {
        Common, // îƒóp
        InGame  // ÉCÉìÉQÅ[ÉÄ
    };
	class LoadingScene :public sf::Scene<LoadingScene> 
    {
    public:
        void Init()override;
        void Update(const sf::command::ICommand& command) override;

        static void SetNextScene(sf::SafePtr<sf::IScene> scene);
        static void SetNextSongInfo(const SongInfo& info);
        static void SetLoadingType(LoadingType type);

        sf::ref::Ref<sf::Actor> managerActor;

    private:
        sf::command::Command<> updateCommand;

        static sf::SafePtr<sf::IScene> pendingNextScene;
        static SongInfo pendingSongInfo;
        static LoadingType pendingType;
    };
}