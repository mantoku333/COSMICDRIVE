#include "TestScene.h"
#include "MoveComponent.h"
#include "PlayerComponent.h"
#include "SceneChangeComponent.h"
#include "TestCanvas.h"
#include "SoundComponent.h"
#include "FollowCameraComponent.h"
#include "BGMComponent.h"

#include "ControlCamera.h"
#include"NoteComponent.h"
#include "NoteManager.h"   
#include "SongInfo.h"



void app::test::TestScene::Init()
{
    sf::debug::Debug::Log("TestSceneのInitです");

    // ── マネージャを生成 ──
    {
        managerActor = Instantiate();
        managerActor.Target()->AddComponent<app::test::NoteManager>();
        managerActor.Target()->AddComponent<TestCanvas>();
        managerActor.Target()->AddComponent<app::test::PlayerComponent>();
        managerActor.Target()->AddComponent<app::test::SoundComponent>();
        bgmPlayer = managerActor.Target()->AddComponent<app::test::BGMComponent>();


        managerActor.Target()->transform.SetScale({ 3.0f, 3.0f, 0.1f });
        managerActor.Target()->transform.SetPosition({
            panelPos.x + 15,
            panelPos.y ,
            panelPos.z - 0.01f
            });

        //mgrActor->leadTime = 1.5f;  // 1.5秒で startY→hitY を流す
    }

    //── パネル（背景） ──
    {
        auto a_Bg = Instantiate();
        auto m_Bg = a_Bg.Target()->AddComponent<sf::Mesh>();
        m_Bg->SetGeometry(g_cube);
        a_Bg.Target()->transform.SetScale({ panelW, panelH, 0.1f });      // 薄くするなら 0.1f
        a_Bg.Target()->transform.SetPosition({ panelPos.x, panelPos.y, panelPos.z + 0.01f });
        m_Bg->material.SetColor({ 0.3f,0.3f,0.3f,1.0f });
    }


    //── レーンごとにパネル生成 ──
    lanePanels.clear(); // ※ std::vector<sf::ref::Ref<sf::Actor>> lanePanels; をTestSceneに追加
    for (int i = 0; i < lanes; ++i) {
        float localX = (i - lanes * 0.5f + 0.5f) * laneW;
        auto lanePanel = Instantiate();
        auto mLane = lanePanel.Target()->AddComponent<sf::Mesh>();
        mLane->SetGeometry(g_cube);
        lanePanel.Target()->transform.SetScale({ laneW, panelH, 0.1f });
        lanePanel.Target()->transform.SetPosition({
            panelPos.x + localX,
            panelPos.y,
            panelPos.z ,
            });
        mLane->material.SetColor({ 0.3f,0.3f,0.3f,1.0f }); // 通常色
        lanePanels.push_back(lanePanel); // レーンごとに保存
    }
    //── レーン区切り線 ──
    {
        const float lineThickness = 0.05f;
        const float lineDepth = 0.1f;
        for (int i = 1; i < lanes; ++i)
        {
            float localX = (i - lanes * 0.5f) * laneW;
            auto line = Instantiate();
            auto mLine = line.Target()->AddComponent<sf::Mesh>();
            mLine->SetGeometry(g_cube);
            line.Target()->transform.SetScale({ lineThickness, panelH, lineDepth });
            line.Target()->transform.SetPosition({
                panelPos.x + localX,
                panelPos.y,               // パネル中心と同じ Y
                panelPos.z - 0.01f        // 面の前に少し出す
                });
            mLine->material.SetColor({ 1,1,1,1 });
        }
    }

    //── ヒット判定バー（赤線・下端）──
    {
        float localY = -panelH * 0.5f
            + panelH * 0.7f
            + barH * 0.5f;

        barY = panelPos.y - localY;

        auto bar = Instantiate();
        auto mBar = bar.Target()->AddComponent<sf::Mesh>();
        mBar->SetGeometry(g_cube);
        bar.Target()->transform.SetScale({ panelW, barH, 0.1f });
        bar.Target()->transform.SetPosition({
            panelPos.x,
            barY,
            panelPos.z - 0.01f
            });
        mBar->material.SetColor({ 1,0,0,1 });
    }

}

void app::test::TestScene::Update(const sf::command::ICommand& command)
{
 
}

void app::test::TestScene::OnActivated()
{
    if (selectedSong.musicPath.empty() || bgmPlayer.isNull()) {
        sf::debug::Debug::Log("[BGM] OnActivated: path empty or bgmPlayer null");
        return;
    }

    // BGMComponent 経由で再生（内部で SoundPlayer/Resource を管理）
    bgmPlayer->SetPath(selectedSong.musicPath);
    bgmPlayer->Play();
}