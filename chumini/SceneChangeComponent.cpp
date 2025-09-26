#include "SceneChangeComponent.h"
#include "SelectScene.h"

void app::test::SceneChangeComponent::Begin()
{
	
	// シーンがメモリ上に存在しなければ
		//if (scene.isNull())
		//{
		//	//シーンをスタンバイ状態にする
		//	scene = SelectScene::StandbyScene();
		//}


	updateCommand.Bind(std::bind(&SceneChangeComponent::Update, this));
}

void app::test::SceneChangeComponent::Update()
{
	////スペースキーが押されたら
	//if (SInput::Instance().GetKeyDown(Key::SPACE))
	//{
	//	//シーンがメモリ上に存在しなければ
	//	if (scene.isNull())
	//	{
	//		//シーンをスタンバイ状態にする
	//		scene = SelectScene::StandbyScene();
	//	}
	//	else
	//	{
	//		//シーンが実体化されていたら
	//		if (scene->IsActivate())
	//		{
	//			//シーンを削除する
	//			scene->DeActivate();
	//			scene = nullptr;
	//		}
	//		else
	//		{
	//			//シーンの読み込みが完了していたら
	//			if (scene->StandbyThisScene())
	//			{
	//				//シーンを実体化させる
	//				scene->Activate();
	//			}
	//			else
	//			{
	//				sf::debug::Debug::LogWarning("シーン読み込みが完了していません！");
	//			}
	//		}
	//	}
	//}
}
//
//// プレイモード移行時に呼び出される関数
//void app::test::SceneChangeComponent::ShowSelectScene()
//{
//	//シーンがメモリ上に存在しなければ
//	if (scene.isNull())
//	{
//		//シーンをスタンバイ状態にする
//		scene = SelectScene::StandbyScene();
//	}
//	else
//	{
//		//シーンが実体化されていたら
//		if (scene->IsActivate())
//		{
//			//シーンを削除する
//			scene->DeActivate();
//			scene = nullptr;
//		}
//		else
//		{
//			//シーンの読み込みが完了していたら
//			if (scene->StandbyThisScene())
//			{
//				//シーンを実体化させる
//				scene->Activate();
//			}
//			else
//			{
//				sf::debug::Debug::LogWarning("シーン読み込みが完了していません！");
//			}
//		}
//	}
//}
