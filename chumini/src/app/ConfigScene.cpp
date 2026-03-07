#include "ConfigScene.h"
#include "ConfigCanvas.h"
#include "Camera.h"
#include "SceneChangeComponent.h"
#include "TitleScene.h"
#include "LoadingScene.h"
#include "Config.h" // SaveConfig用
#include "SInput.h" // 入力をシーンで処理する場合（今回はCanvasから通知）

using namespace app::test;
using namespace sf;

void ConfigScene::Init()
{
	// UIマネージャーアクターの作成
	uiManagerActor = Instantiate();
	sceneChanger = uiManagerActor.Target()->AddComponent<SceneChangeComponent>(); // Must be added before Canvas
	configCanvas = uiManagerActor.Target()->AddComponent<ConfigCanvas>();

    // MVP: CanvasにPresenterを設定
    if (configCanvas.Get()) {
        configCanvas->SetPresenter(this);
    }

	// メインカメラの設定
	auto mainCamera = Instantiate();
	auto cameraComp = mainCamera.Target()->AddComponent<sf::Camera>();
	mainCamera.Target()->transform.SetPosition(Vector3(0, 0, -10));

	updateCommand.Bind(std::bind(&ConfigScene::Update, this, std::placeholders::_1));
}

void ConfigScene::Update(const sf::command::ICommand& command)
{
    // シーン独自の更新処理があればここに記述
}

void ConfigScene::OnNavigateUp()
{
    if (state != State::Normal) return;
    if (configCanvas.Get()) {
        int count = configCanvas->GetItemCount();
        if (count > 0) {
            selectedIndex = (selectedIndex - 1 + count) % count;
            configCanvas->UpdateSelection(selectedIndex);
        }
    }
}

void ConfigScene::OnNavigateDown()
{
    if (state != State::Normal) return;
    if (configCanvas.Get()) {
        int count = configCanvas->GetItemCount();
        if (count > 0) {
            selectedIndex = (selectedIndex + 1) % count;
            configCanvas->UpdateSelection(selectedIndex);
        }
    }
}

void ConfigScene::OnNavigateLeft()
{
    if (state != State::Normal) return;
    if (configCanvas.Get()) {
        configCanvas->ExecuteLeft(selectedIndex);
    }
}

void ConfigScene::OnNavigateRight()
{
    if (state != State::Normal) return;
    if (configCanvas.Get()) {
        configCanvas->ExecuteRight(selectedIndex);
    }
}

void ConfigScene::OnConfirm()
{
    if (state != State::Normal) return;
    if (configCanvas.Get()) {
        configCanvas->ExecuteItem(selectedIndex);
    }
}

void ConfigScene::OnCancel()
{
    if (state != State::Normal) return;
    // キー設定待ちのキャンセル対応
    if (state == State::WaitingForKey) {
        state = State::Normal;
        waitingLaneIndex = -1;
        // Canvas更新必要？現状そのまま戻る
        return;
    }
    NavigateToTitle();
}

void ConfigScene::OnKeyInputStart(int laneIndex)
{
    state = State::WaitingForKey;
    waitingLaneIndex = laneIndex;
    // Canvas側はPresenterの状態を見て表示を変える
}

void ConfigScene::OnKeyInput(int key)
{
    if (state != State::WaitingForKey) return;
    
    // キー設定更新
    if (waitingLaneIndex >= 0 && waitingLaneIndex < 4) {
        Key k = (Key)key;
		if (waitingLaneIndex == 0) gKeyConfig.lane1 = k;
		else if (waitingLaneIndex == 1) gKeyConfig.lane2 = k;
		else if (waitingLaneIndex == 2) gKeyConfig.lane3 = k;
		else if (waitingLaneIndex == 3) gKeyConfig.lane4 = k;
		
		SaveConfig();
    }
    
    state = State::Normal;
    waitingLaneIndex = -1;
}

void ConfigScene::NavigateToTitle()
{
    if (sceneChanger.isNull()) return;
    LoadingScene::SetLoadingType(LoadingType::Common);
    sceneChanger->ChangeScene(TitleScene::StandbyScene());
    SaveConfig(); // 念の為
}
