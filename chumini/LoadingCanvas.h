#pragma once
#include "App.h"
#include "TextImage.h"
#include "SongInfo.h"

namespace app::test {

    class LoadingCanvas : public sf::ui::Canvas {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);

        // 次のシーンをセットする
        void SetTargetScene(sf::SafePtr<sf::IScene> scene);
        void SetSongInfo(const SongInfo& info);

    private:
        sf::command::Command<> updateCommand;

        // 読み込むべき次のシーン
        sf::SafePtr<sf::IScene> targetScene;

        sf::ui::TextImage* songTitleText = nullptr;
        sf::ui::TextImage* artistText = nullptr;

        // UIパーツ
        sf::ui::TextImage* loadingText = nullptr;
        float timer = 0.0f;
        bool isLoadCompleted = false;

		const float MIN_LOADING_TIME = 3.0f; // 最低表示時間
    };
}