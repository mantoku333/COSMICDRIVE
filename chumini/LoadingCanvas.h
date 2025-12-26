#pragma once
#include "App.h"
#include "TextImage.h"
#include "SongInfo.h"
#include "LoadingScene.h"

namespace app::test {

    class LoadingCanvas : public sf::ui::Canvas {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);

        // シーンから指示を受け取る関数
        void SetTargetScene(sf::SafePtr<sf::IScene> scene);
        void SetSongInfo(const SongInfo& info);
        void SetLoadingType(LoadingType type);

    private:
        sf::command::Command<> updateCommand;

        // 読み込むべき次のシーン
        sf::SafePtr<sf::IScene> targetScene;

        sf::ui::TextImage* songTitleText = nullptr;
        sf::ui::TextImage* artistText = nullptr;

        sf::ui::Image* jacketImage = nullptr;
        // UIパーツ
        sf::ui::TextImage* loadingText = nullptr;
        float timer = 0.0f;
        bool isLoadCompleted = false;

		const float MIN_LOADING_TIME = 1.0f; // 最低表示時間

        LoadingType currentType = LoadingType::Common;
    };
}