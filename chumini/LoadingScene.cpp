#include "LoadingScene.h"
#include "LoadingCanvas.h"

namespace app::test {

    // 静的変数の実体定義
    sf::SafePtr<sf::IScene> LoadingScene::pendingNextScene = nullptr;
    SongInfo LoadingScene::pendingSongInfo = { "", "", "" };

    void LoadingScene::SetNextScene(sf::SafePtr<sf::IScene> scene) {
        pendingNextScene = scene;
    }
    // ★追加: 曲情報を保存
    void LoadingScene::SetNextSongInfo(const SongInfo& info) {
        pendingSongInfo = info;
    }

    void LoadingScene::Init() {

        managerActor = Instantiate();
        if (!managerActor.Target()) return;

        sf::SafePtr<LoadingCanvas> canvas = managerActor.Target()->AddComponent<LoadingCanvas>();

        // ★修正2: SafePtr なので !isNull() でチェックする
        if (!canvas.isNull() && !pendingNextScene.isNull()) {

            // SafePtr は -> で関数を呼べます
            canvas->SetTargetScene(pendingNextScene);
            pendingNextScene = nullptr;
        }

        canvas->SetSongInfo(pendingSongInfo);
        pendingSongInfo = { "", "", "" };

        updateCommand.Bind(std::bind(&LoadingScene::Update, this, std::placeholders::_1));
    }

	void LoadingScene::Update(const sf::command::ICommand& command) {
	}
}