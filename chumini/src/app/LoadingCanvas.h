#pragma once
#include "App.h"
#include "TextImage.h"
#include "SongInfo.h"
// #include "LoadingScene.h" // Forward decl is better if possible, but we need LoadingType enum...
// Circular dependency risk. Let's include LoadingScene.h if it has the enum.
#include "LoadingScene.h" 

#include "Texture.h"

namespace app::test {

    class LoadingCanvas : public sf::ui::Canvas {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);
        void Draw() override;

        // MVP: Presenter設定
        void SetPresenter(LoadingScene* scene) { presenter = scene; }

        // View Setup (called by Presenter)
        void SetSongInfo(const SongInfo& info);
        void SetLoadingType(LoadingType type);

    private:
        sf::command::Command<> updateCommand;
        LoadingScene* presenter = nullptr;

        void DrawLoadingGauge();

        // UI Parts
        sf::ui::TextImage* songTitleText = nullptr;
        sf::ui::TextImage* artistText = nullptr;
        
        sf::ui::Image* jacketImage = nullptr;
        sf::Texture jacketTexture; 
        
        sf::ui::TextImage* loadingText = nullptr;
        
        // Animation only timer (separate from logic timer)
        float animationTimer = 0.0f;

        LoadingType currentType = LoadingType::Common;
    };
}