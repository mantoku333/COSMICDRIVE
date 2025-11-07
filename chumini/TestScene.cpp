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

        // ── レーン外枠（左右エッジ）──
        {
            laneEdges.clear(); // ← 忘れず初期化

            const float edgeThickness = 0.08f;
            const float edgeHeight = 0.1f;
            const float extendLength = 1.0f;
            const float edgeMargin = 0.05f;
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            float halfWidth = (lanes * 0.5f) * laneW;

            struct EdgeInfo { float x; float scaleX; };
            std::vector<EdgeInfo> edges = {
                { -halfWidth - extendLength * 0.5f - edgeMargin, edgeThickness + extendLength },
                {  halfWidth + extendLength * 0.5f + edgeMargin, edgeThickness + extendLength },
            };

            for (auto& e : edges)
            {
                auto edge = Instantiate();
                auto mEdge = edge.Target()->AddComponent<sf::Mesh>();
                mEdge->SetGeometry(g_cube);

                edge.Target()->transform.SetScale({ e.scaleX, edgeHeight, laneH });
                edge.Target()->transform.SetPosition({ e.x, baseY + 0.05f, 0.0f });
                edge.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mEdge->material.SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });

                laneEdges.push_back(edge); // 追加
            }
        }

        // ── ジャッジバー（手前ライン）──
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



    if (auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>())
    {
        noteMgr->SetLaneParams(lanePanels, laneW, laneH, rotX, baseY, barRatio);
    }

    updateCommand.Bind(std::bind(&TestScene::Update, this, std::placeholders::_1));
}

void app::test::TestScene::Update(const sf::command::ICommand& command)
{
    static float time = 0.0f;
    time += sf::Time::DeltaTime();

    for (auto& edge : laneEdges)
    {
        if (edge.Target() == nullptr) continue;
        auto mesh = edge.Target()->GetComponent<sf::Mesh>();
        if (!mesh) continue;

        // Z位置を基準に色を変化させる
        float z = edge.Target()->transform.GetPosition().z;
        float scroll = time * 2.0f;              // 流れる速度
        float t = (sin(z * 0.3f + scroll) * 0.5f) + 0.5f; // 上下グラデ＋時間経過

        // グラデーション色（上が赤→下がオレンジ寄り）
        float r = 1.0f;
        float g = 0.3f + 0.4f * t;
        float b = 0.3f + 0.2f * (1.0f - t);
        float a = 1.0f;

        mesh->material.SetColor({ r, g, b, a });
    }
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
