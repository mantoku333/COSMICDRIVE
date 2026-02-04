#include "ResultScene.h"
#include "ResultCanvas.h"
#include "IngameCanvas.h"
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
    /*
    auto l2dActor = Instantiate();
    live2DManager = l2dActor.Target()->AddComponent<Live2DComponent>();

    if (live2DManager.Get()) {
        // Load Model
        live2DManager->LoadModel("Assets/Live2D/CyberCat", "CyberCat.model3.json");

        // Transform setup
        l2dActor.Target()->transform.SetPosition({ 0.5f, -0.4f, 0.0f }); 
        l2dActor.Target()->transform.SetScale({ 0.75f, 0.75f, 0.75f }); 
    }
    */

    // 2. UI Actor
	uiManagerActor = Instantiate();
	uiManagerActor.Target()->AddComponent<ResultCanvas>();
    uiManagerActor.Target()->AddComponent<SceneChangeComponent>(); // Add SceneChangeComponent

	updateCommand.Bind(std::bind(&ResultScene::Update, this, std::placeholders::_1));
}

void app::test::ResultScene::OnActivated()
{
    // Score logic moved here because Init() runs at game start (when score is 0)
    // OnActivated runs when the scene actually appears after gameplay.

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
    /*
    if (live2DManager.Get()) {
        // CyberCat only has "Idle" motion (GlitchNoise), so we play it regardless of score for now.
        // If specific motions are added later, restore the conditional branching.
        live2DManager->PlayMotion("Idle", 0, 3);
        
        // ★Loop Glitch Animation
        live2DManager->StartGlitchMotion("GlitchNoise", 0);
    }
    */
}

void app::test::ResultScene::Update(const sf::command::ICommand& command)
{
    // Update base class to process built-in actor logic (if any)
    sf::Scene<ResultScene>::Update(command);
    
    // Manual Update for Live2D as unrelated component
    /*
    if (live2DManager.Get()) {
        live2DManager->Update();
    }
    */
}

void app::test::ResultScene::DrawOverlay() {
    if (live2DManager.Get()) {
    /*
        live2DManager->Draw();
    */
    }
}
