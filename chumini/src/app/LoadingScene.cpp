#include "LoadingScene.h"
#include "LoadingCanvas.h"
#include "Debug.h"
#include "sf/Time.h"

namespace app::test {

    // Static Members
    sf::SafePtr<sf::IScene> LoadingScene::pendingNextScene = nullptr;
    SongInfo LoadingScene::pendingSongInfo;
    LoadingType LoadingScene::pendingType = LoadingType::Common;

    void LoadingScene::SetNextScene(sf::SafePtr<sf::IScene> scene) {
        pendingNextScene = scene;
    }

    void LoadingScene::SetNextSongInfo(const SongInfo& info) {
        pendingSongInfo = info;
    }

    void LoadingScene::SetLoadingType(LoadingType type) {
        pendingType = type;
    }

    void LoadingScene::Init() {

        managerActor = Instantiate();
        if (!managerActor.Target()) return;

        // Create Canvas
        loadingCanvas = managerActor.Target()->AddComponent<LoadingCanvas>();
        if (loadingCanvas.Get()) {
            loadingCanvas->SetPresenter(this);
        }

        // Logic Initialization
        targetScene = pendingNextScene;
        pendingNextScene = nullptr; // consume
        
        currentType = pendingType;
        
        // Pass View Data
        if (loadingCanvas.Get()) {
            loadingCanvas->SetLoadingType(currentType);
            loadingCanvas->SetSongInfo(pendingSongInfo);
        }
        
        // Setup Timer based on Type
        if (currentType == LoadingType::Common) {
            minLoadingTime = 0.5f;
        } else {
            minLoadingTime = 1.0f;
        }

        // Output Log
        sf::debug::Debug::Log("LoadingScene Init: Type=" + std::to_string((int)currentType) + 
                              ", MinTime=" + std::to_string(minLoadingTime));

        pendingSongInfo = { "", "", "" }; // clear info
        timer = 0.0f;
        isLoadCompleted = false;

        updateCommand.Bind(std::bind(&LoadingScene::Update, this, std::placeholders::_1));
    }

    void LoadingScene::Update(const sf::command::ICommand& command) {
        timer += sf::Time::DeltaTime();

        if (targetScene.isNull()) return;

        // Load Process
        if (!isLoadCompleted) {
            if (targetScene->StandbyThisScene()) {
                isLoadCompleted = true;
                sf::debug::Debug::Log("Load Complete (Standby Finish)");
            }
        }

        // Transition Check
        if (isLoadCompleted && timer >= minLoadingTime) {
            sf::debug::Debug::Log("Transitioning to Target Scene");
            
            // Activate next scene
            targetScene->Activate();
            
            // Deactivate self
            DeActivate();
        }
    }

    float LoadingScene::GetProgress() const {
        if (isLoadCompleted) return 1.0f;
        if (!targetScene.isNull()) {
            return targetScene->loadProgress;
        }
        return 0.0f;
    }
}