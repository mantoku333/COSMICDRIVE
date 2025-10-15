#include "TitleScene.h"
#include "TitleCanvas.h"
#include "SceneChangeComponent.h"

#include"DirectWrite.h"

void app::test::TitleScene::Init()
{
	sf::debug::Debug::Log("TitleSceneのInitです");

    // ── タイトル画面のUI管理用アクター ──
    {
        uiManagerActor = Instantiate();
        uiManagerActor.Target()->AddComponent<TitleCanvas>();  // タイトル用のUI Canvas
        uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
        uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });
    }

	DirectWrite();



    // ── 初期状態の設定 ──
    selectedButton = 0; // 0: エディット, 1: プレイ
    isButtonPressed = false;


}