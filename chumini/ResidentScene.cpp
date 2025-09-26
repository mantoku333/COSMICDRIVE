#include "ResidentScene.h"
#include "TestScene.h"
#include "TitleScene.h"
#include "SelectScene.h"	
#include "ControlCamera.h"

void app::ResidentScene::Init()
{
	auto camActor = Instantiate();
	camActor.Target()->transform.SetPosition(Vector3(0, 0, 0));    // z=-50 あたりから正面向き
	camActor.Target()->AddComponent<ControlCamera>();
	auto camComp = camActor.Target()->AddComponent<sf::Camera>();
	sf::Camera::main = camComp.Get();

	//影カメラ
	sf::ref::Ref<sf::Actor> shadowActor = Instantiate();
	shadowActor.Target()->transform.SetPosition(Vector3(0, 5, 0));
	shadowActor.Target()->transform.SetRotation(Vector3(90, 0, 0));
	sf::SafePtr<sf::Camera> shadowCamera = shadowActor.Target()->AddComponent<sf::Camera>();
	sf::Camera::shadow = shadowCamera.Get();

	LoadLoadingScene();
}

void app::ResidentScene::LoadLoadingScene()
{
	//初期シーンはメインスレッド内でロード、アクティベートさせる
	auto scene = test::TitleScene::StandbyScene();
	//auto scene = test::SelectScene::StandbyScene();
	//auto scene = test::TestScene::StandbyScene();

	while (1)
	{
		if (scene->StandbyThisScene())
		{
			break;
		}
	}
	scene->Activate();
}
