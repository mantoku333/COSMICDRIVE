#include "EditorComponent.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>   // rand()
#include <ctime>  

#include "EditScene.h"

void app::test::EditorComponent::Begin()
{
    // Update関数の設定
    updateCommand.Bind(std::bind(&EditorComponent::Update, this));
}

void app::test::EditorComponent::Update()
{
    auto& I = SInput::Instance();
    auto owner = actorRef.Target();
    if (!owner) return;

    auto* scene = &owner->GetScene();
    auto* editScene = dynamic_cast<EditScene*>(scene); // ← パネル情報を取るためにキャスト
    if (!editScene) return;

    // 左クリック：パネル面に沿ってノーツを生成
    if (I.GetMouseDown(0))
    {
        // ==== ① パネル情報を取得 ====
        float panelW = editScene->panelW;
        float panelH = editScene->panelH;
        int lanes = editScene->lanes;
        float laneW = panelW / lanes;

        // 中心座標・バー・上端など
        float originX = editScene->panelPos.x;
        float originY = editScene->panelPos.y;
        float panelZ = editScene->panelPos.z;

        // ==== ② レーン位置を計算
        int laneIndex = std::rand() % 5;
        float noteX = originX + (-panelW / 2.0f + laneW * (laneIndex + 0.5f));

        // ==== ③ ノーツ生成 ====
        auto note = scene->Instantiate();
        auto mesh = note.Target()->AddComponent<sf::Mesh>();
        mesh->SetGeometry(g_cube);\
        note.Target()->transform.SetScale({ laneW - 0.1f, 0.5f, 0.1f });

        // 上端スタート or 中央など、好きな位置でOK
        float startY = originY + panelH / 2.0f;
        float Y = -5.0f + static_cast<float>(std::rand()) / RAND_MAX * (15.0f - (-5.0f));
        note.Target()->transform.SetPosition({
            noteX,
            Y,
            panelZ - 0.3f   // パネルのちょい手前に出す
            });

        mesh->material.SetColor({ 1,1,1,1 });
        note.Target()->Activate();

        // ログ確認
        const auto pos = note.Target()->transform.GetPosition();
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss.precision(2);
        oss << "ノーツ生成 lane=" << laneIndex
            << " x=" << pos.x << " y=" << pos.y << " z=" << pos.z;
        sf::debug::Debug::Log(oss.str());
    }
}
