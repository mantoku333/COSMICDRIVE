#include "ConfigScene.h"
#include "ConfigCanvas.h"
#include "Camera.h"
#include "SceneChangeComponent.h"

using namespace app::test;
using namespace sf;

void ConfigScene::Init()
{
	// UIマネージャーアクターの作成
	uiManagerActor = Instantiate();
	uiManagerActor.Target()->AddComponent<SceneChangeComponent>(); // Must be added before Canvas to be found in Begin()
	uiManagerActor.Target()->AddComponent<ConfigCanvas>();

	// メインカメラの設定
	auto mainCamera = Instantiate();
	auto cameraComp = mainCamera.Target()->AddComponent<sf::Camera>();
	// 2D用正射影設定 (画面サイズに合わせて適宜調整)
	// cameraComp->SetOrthographic(1920, 1080, 0.1f, 1000.0f); // Not implemented in Camera class
	mainCamera.Target()->transform.SetPosition(Vector3(0, 0, -10));

	updateCommand.Bind(std::bind(&ConfigScene::Update, this, std::placeholders::_1));
}

void ConfigScene::Update(const sf::command::ICommand& command)
{
}
