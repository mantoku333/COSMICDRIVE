#include "SelectScene.h"
#include "SelectCanvas.h"
#include "SInput.h"
#include "Debug.h"
#include "IngameScene.h"
#include "TitleScene.h"
#include "LoadingScene.h"

namespace app::test {

void SelectScene::Init()
{
    sf::debug::Debug::Log("SelectScene: Init");

    uiManagerActor = Instantiate();
    selectCanvas = uiManagerActor.Target()->AddComponent<SelectCanvas>();
    sceneChanger = uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    // MVP: CanvasにPresenterを設定
    if (selectCanvas.Get()) {
        selectCanvas->SetPresenter(this);
    }

    updateCommand.Bind(std::bind(&SelectScene::Update, this, std::placeholders::_1));
}

void SelectScene::Update(const sf::command::ICommand& command)
{
    // Canvasが更新を処理
}

// MVP: ゲーム画面へ遷移
void SelectScene::NavigateToGame(const SongInfo& song)
{
    if (sceneChanger.isNull()) return;

    auto next = IngameScene::StandbyScene();
    if (next) {
        next->SetSelectedSong(song);
    }

    LoadingScene::SetLoadingType(LoadingType::InGame);
    LoadingScene::SetNextSongInfo(song);

    sceneChanger->ChangeScene(next);
}

// MVP: タイトル画面へ戻る
void SelectScene::NavigateToTitle()
{
    if (sceneChanger.isNull()) return;
    
    sf::debug::Debug::Log("SelectScene: NavigateToTitle");
    sceneChanger->ChangeScene(TitleScene::StandbyScene());
}

} // namespace app::test