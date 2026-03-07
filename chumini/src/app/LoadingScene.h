#pragma once
#include "App.h"
#include "SongInfo.h"
#include "SafePtr.h"

namespace app::test {
    
    class LoadingCanvas; // Forward declaration

    enum class LoadingType {
        Common, // Common
        InGame  // InGame
    };
    
    class LoadingScene :public sf::Scene<LoadingScene> 
    {
    public:
        void Init()override;
        void Update(const sf::command::ICommand& command) override;

        static void SetNextScene(sf::SafePtr<sf::IScene> scene);
        static void SetNextSongInfo(const SongInfo& info);
        static void SetLoadingType(LoadingType type);

        // MVP: View Accessors
        float GetTimer() const { return timer; }
        bool IsLoadCompleted() const { return isLoadCompleted; }
        float GetProgress() const;

    private:
        sf::ref::Ref<sf::Actor> managerActor;
        sf::command::Command<> updateCommand;

        sf::SafePtr<LoadingCanvas> loadingCanvas;

        // Static Pending Data
        static sf::SafePtr<sf::IScene> pendingNextScene;
        static SongInfo pendingSongInfo;
        static LoadingType pendingType;

        // Instance Logic Data
        sf::SafePtr<sf::IScene> targetScene;
        float timer = 0.0f;
        float minLoadingTime = 0.5f;
        bool isLoadCompleted = false;
        LoadingType currentType = LoadingType::Common;
    };
}
