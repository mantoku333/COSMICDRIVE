#include "TestScene.h"
#include "MoveComponent.h"
#include "PlayerComponent.h"
#include "SceneChangeComponent.h"
#include "TestCanvas.h"
#include "SoundComponent.h"
#include "FollowCameraComponent.h"
#include "BGMComponent.h"

#include "ControlCamera.h"
#include "NoteComponent.h"
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
    }

    // ===== レーン配置 =====
    {
        const int lanes = 5;           // レーン数
        const float laneW = 3.0f;      // レーン幅
        const float laneH = 50.0f;    // 奥行き（Z方向）
        const float rotX = -10.0f;     // 傾き（チュウニズム風の角度）
        const float baseY = 0.0f;      // 高さの基準

        const float barRatio = 0.15f;

        lanePanels.clear();

        // ── レーン生成 ──
        for (int i = 0; i < lanes; ++i)
        {
            float localX = (i - lanes * 0.5f + 0.5f) * laneW;

            auto lane = Instantiate();
            auto mLane = lane.Target()->AddComponent<sf::Mesh>();
            mLane->SetGeometry(g_cube);

            lane.Target()->transform.SetScale({ laneW, 0.1f, laneH });
            lane.Target()->transform.SetPosition({ localX, baseY, 0.0f });
            lane.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

            mLane->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
            lanePanels.push_back(lane);
        }

        // ── 区切り線 ──
        {
            const float lineThickness = 0.05f;
            const float lineHeight = 0.05f;
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            for (int i = 1; i < lanes; ++i)
            {
                float localX = (i - lanes * 0.5f) * laneW;

                // レーン中央を基準にY/Z補正
                float offsetZ = 0.0f; // Zはレーン中央に固定
                float offsetY = std::tan(slopeRad) * offsetZ; // ← tan(rotX) 符号修正

                auto line = Instantiate();
                auto mLine = line.Target()->AddComponent<sf::Mesh>();
                mLine->SetGeometry(g_cube);

                line.Target()->transform.SetScale({ lineThickness, lineHeight, laneH });
                line.Target()->transform.SetPosition({ localX, baseY + offsetY + 0.05f, offsetZ });
                line.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mLine->material.SetColor({ 1, 1, 1, 1 });
            }
        }

        // ── ジャッジバー（手前ピンクライン）──
        {
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            // laneH の範囲（-half ～ +half）を使って位置を算出
            const float halfH = laneH * 0.5f;
            float zPos = -halfH + laneH * barRatio; // 下端(-half)からratio分だけ進む

            // 傾斜角に合わせてYを補正
            float yPos = -std::tan(slopeRad) * zPos;

            auto bar = Instantiate();
            auto mBar = bar.Target()->AddComponent<sf::Mesh>();
            mBar->SetGeometry(g_cube);

            bar.Target()->transform.SetScale({ lanes * laneW, 0.1f, 0.05f });
            bar.Target()->transform.SetPosition({ 0.0f, baseY + yPos + 0.06f, zPos });
            bar.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

            mBar->material.SetColor({ 1, 0, 1, 1 });
        }
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

    bgmPlayer->SetPath(selectedSong.musicPath);
    bgmPlayer->Play();
}
