#include "EditScene.h"
#include "EditCanvas.h"
#include "SInput.h"
#include "Debug.h"
#include "Time.h"          // DeltaTime を使う想定
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <windows.h> 
#include <DirectXMath.h>
#include "EditorComponent.h"


using namespace app::test;
using namespace DirectX;

void EditScene::Init()
{
    // ---- UI キャンバス（エディタ本体） ----
    uiManagerActor = Instantiate();
    //auto editCanvas = uiManagerActor.Target()->AddComponent<EditCanvas>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    uiManagerActor.Target()->AddComponent<EditorComponent>();

    // ---- Update 登録 ----
    updateCommand.Bind(std::bind(&EditScene::Update, this, std::placeholders::_1));

    //// 背景パネル
    //{
    //    auto a_Bg = Instantiate();
    //    auto m_Bg = a_Bg.Target()->AddComponent<sf::Mesh>();
    //    m_Bg->SetGeometry(g_cube); // 既存の立方体ジオメトリを使用
    //    a_Bg.Target()->transform.SetScale({ panelW, panelH, 0.1f });     // 薄め
    //    a_Bg.Target()->transform.SetPosition({ panelPos.x, panelPos.y, panelPos.z + 0.01f });
    //    m_Bg->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
    //}

    // レーン板
    lanePanels.clear();
    {
        // laneW を panelW / lanes に合わせたい場合は下記を有効に
        // laneW = panelW / static_cast<float>(lanes);

        for (int i = 0; i < lanes; ++i) {
            float localX = (i - lanes * 0.5f + 0.5f) * laneW;
            auto lanePanel = Instantiate();
            auto mLane = lanePanel.Target()->AddComponent<sf::Mesh>();
            mLane->SetGeometry(g_cube);
            lanePanel.Target()->transform.SetScale({ laneW, panelH, 0.1f });
            lanePanel.Target()->transform.SetPosition({
                panelPos.x + localX,
                panelPos.y,
                panelPos.z
                });
            mLane->material.SetColor({ 0.25f, 0.25f, 0.25f, 1.0f }); // 背景より少し暗め
            lanePanels.push_back(lanePanel);
        }
    }

    // レーン区切り線
    {
        const float lineThickness = 0.05f;
        const float lineDepth = 0.1f;
        for (int i = 1; i < lanes; ++i) {
            float localX = (i - lanes * 0.5f) * laneW;
            auto line = Instantiate();
            auto mLine = line.Target()->AddComponent<sf::Mesh>();
            mLine->SetGeometry(g_cube);
            line.Target()->transform.SetScale({ lineThickness, panelH, lineDepth });
            line.Target()->transform.SetPosition({
                panelPos.x + localX,
                panelPos.y,
                panelPos.z - 0.01f
                });
            mLine->material.SetColor({ 1, 1, 1, 1 });
        }
    }

    // ヒット判定バー（赤線）
    {
        float localY = -panelH * 0.5f + panelH * 0.7f + barH * 0.5f;
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
        mBar->material.SetColor({ 1, 0, 0, 1 });
    }
}

void EditScene::Update(const sf::command::ICommand& /*command*/)
{
   
}

