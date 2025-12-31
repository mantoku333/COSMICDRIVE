#include "ResultScene.h"
#include "ResultCanvas.h"
#include "TestCanvas.h"
#include "JudgeStatsService.h"
#include "JudgeStatsService.h"
#include "Actor.h"
#include "Live2DComponent.h"
#include "SceneChangeComponent.h" 
#include "NoteData.h"

// Note: Ensure Instantiate() is available. It is a member of Scene.

void app::test::ResultScene::Init()
{
    // 1. Live2D Actor
    auto l2dActor = Instantiate();
    live2DManager = l2dActor.Target()->AddComponent<Live2DComponent>();

    if (live2DManager.Get()) {
        // Load Model
        live2DManager->LoadModel("Assets/Live2D/Hiyori", "Hiyori.model3.json");

        // Transform setup: Live2DComponent is attached to l2dActor
        l2dActor.Target()->transform.SetPosition({ 0.0f, -0.6f, 0.0f }); 
        l2dActor.Target()->transform.SetScale({ 1.0f, 1.0f, 1.0f }); 
    }

    // Score Calculation
    int perfect = JudgeStatsService::GetCount(JudgeResult::Perfect);
    int great = JudgeStatsService::GetCount(JudgeResult::Great);
    int good = JudgeStatsService::GetCount(JudgeResult::Good);
    int miss = JudgeStatsService::GetCount(JudgeResult::Miss);
    
    int totalNotes = perfect + great + good + miss;
    int score = 0;
    if (totalNotes > 0) {
        double p = (perfect * 1.0 + great * 0.8 + good * 0.5) / totalNotes;
        score = static_cast<int>(p * 1000000);
    }

    // Motion Selection based on Rank (S >= 800,000)
    if (live2DManager.Get()) {
        if (score >= 800000) {
            live2DManager->PlayMotion("Idle", 0, 3);
        } else {
            live2DManager->PlayMotion("TapBody", 0, 3);
        }
    }

    // 2. UI Actor
	uiManagerActor = Instantiate();
	uiManagerActor.Target()->AddComponent<ResultCanvas>();
    uiManagerActor.Target()->AddComponent<SceneChangeComponent>(); // Add SceneChangeComponent

	updateCommand.Bind(std::bind(&ResultScene::Update, this, std::placeholders::_1));
}

void app::test::ResultScene::Update(const sf::command::ICommand& command)
{
    // Update base class to process built-in actor logic (if any)
    sf::Scene<ResultScene>::Update(command);
    
    // Manual Update for Live2D as unrelated component
    if (live2DManager.Get()) {
        live2DManager->Update();
    }
}

void app::test::ResultScene::DrawOverlay() {
    if (live2DManager.Get()) {
        live2DManager->Draw();
    }
}
