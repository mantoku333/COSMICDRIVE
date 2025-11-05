// PlayerComponent.cpp
#include "PlayerComponent.h"
#include "SInput.h"
#include "NoteManager.h" // 必要に応じて
#include "TestScene.h"
#include "TestCanvas.h"
#include "SoundComponent.h"

using namespace app::test;

void PlayerComponent::Begin() {
    updateCommand.Bind(std::bind(&PlayerComponent::Update, this, std::placeholders::_1));
}

void PlayerComponent::Update(const sf::command::ICommand&) {

    // シーン取得
    auto actor = actorRef.Target();
    if (!actor) return;
    auto& scene = actor->GetScene();
    auto* testScene = dynamic_cast<TestScene*>(&scene);
    if (!testScene) return;

    // レーンごとのキーとインデックス
    struct LaneKey { int idx; Key key; };
    LaneKey laneKeys[] = {
        {0, Key::KEY_A},
        {1, Key::KEY_S},
        {2, Key::SPACE},
        {3, Key::KEY_K},
        {4, Key::KEY_L},
    };

    for (const auto& lk : laneKeys) {
        // キー押下で明るく
        if (SInput::Instance().GetKey(lk.key)) {
            auto laneActor = testScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f }); // 明るい色
            }
        }
        // キー離したら元の色に戻す
        else if (SInput::Instance().GetKeyUp(lk.key)) {
            auto laneActor = testScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f }); // 元の色
            }
        }

        // ここでノーツ判定
        if (SInput::Instance().GetKeyDown(lk.key)) {

            auto* managerActor = testScene->managerActor.Target();
            if (!managerActor) return;
            auto* noteManager = managerActor->GetComponent<app::test::NoteManager>();
            if (!noteManager) return;

            float now = noteManager->GetSongTime(); // 現在の楽曲時間
            JudgeResult result = noteManager->JudgeLane(lk.idx, now); //判定結果を返す

            // リザルトに応じた効果音をオールウェイズ提供
            if (result == JudgeResult::Skip) {
                // 空撃ち
                auto* sound = managerActor->GetComponent<app::test::SoundComponent>();
                if (sound) sound->Play(app::test::SoundComponent::Sfx::EmptyTap);
                continue;
            }else {
                // ヒット
                auto* sound = managerActor->GetComponent<app::test::SoundComponent>();
                if (sound) sound->Play(app::test::SoundComponent::Sfx::Tap);
            }

            // 判定結果に応じた処理
            auto* canvas = testScene->managerActor.Target()->GetComponent<app::test::TestCanvas>();
            if (canvas) {
                canvas->SetJudgeImage(result);
            }

            //ここで判定結果を集計してリザルトに渡す予定
        }
    }


}
