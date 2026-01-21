#include "PlayerComponent.h"
#include <Windows.h>
#include "SInput.h"
#include "NoteManager.h"
#include "IngameScene.h"
#include "IngameCanvas.h"
#include "SoundComponent.h"
#include "Config.h"

using namespace app::test;

void PlayerComponent::Begin() {
    updateCommand.Bind(std::bind(&PlayerComponent::Update, this, std::placeholders::_1));

    // シーンを取得
    auto* ingameScene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene());
    if (!ingameScene) return;

    // ジャッジバーが存在すれば位置を合わせる
    if (ingameScene->judgeBar.Target()) {
        Vector3 barPos = ingameScene->judgeBar.Target()->transform.GetPosition();

        // 少し上に配置（見た目調整OK）
        actorRef.Target()->transform.SetPosition({
            0.0f,          // Xは中央
			barPos.y + 3.0f,  // 少し上に
			barPos.z + 1.0f  // 少し手前に
            });
    }
}


// ======================================
// マウス位置 → Z=targetZ 平面のワールド座標変換（任意）
// ======================================
Vector3 ScreenToWorldOnPlane(float mouseX, float mouseY, float targetZ = 0.0f)
{
    HWND hwnd = sf::dx::DirectX11::Instance()->GetHWND();
    RECT rect;
    GetClientRect(hwnd, &rect);
    float screenW = static_cast<float>(rect.right - rect.left);
    float screenH = static_cast<float>(rect.bottom - rect.top);

    // スクリーン中心を原点に正規化 (-1 ～ 1)
    float nx = (2.0f * mouseX / screenW - 1.0f);
    float ny = (1.0f - 2.0f * mouseY / screenH);

    // カメラを使わず、画面空間を直接Z平面に投影
    const float camDist = 5.0f; // 仮のカメラ距離

    Vector3 camPos(0.0f, 0.0f, -camDist);
    Vector3 rayDir(nx, ny, 1.0f);
    rayDir = rayDir.Normarize();

    // Z=targetZ 平面と交差する位置を求める
    float t = (targetZ - camPos.z) / rayDir.z;
    Vector3 hitPos = camPos + rayDir * t;

    return hitPos;
}


// ======================================
// メインアップデート処理
// ======================================
void PlayerComponent::Update(const sf::command::ICommand&) {

    auto actor = actorRef.Target();
    if (!actor) return;
    auto& scene = actor->GetScene();
    auto* ingameScene = dynamic_cast<IngameScene*>(&scene);
    if (!ingameScene) return;

    HWND hwnd = sf::dx::DirectX11::Instance()->GetHWND();
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);

    if (!mouseInitialized) {
        prevMouseX = p.x;
        prevMouseY = p.y;
        mouseInitialized = true;
    }

    float dx = static_cast<float>(p.x - prevMouseX);
    prevMouseX = p.x;
    prevMouseY = p.y;

    // -----------------------------
    // 移動＆サイドレーン判定の統合処理
    // -----------------------------
    
    const float moveThreshold = 20.0f; // 判定・移動共通の閾値(デッドゾーン)
    bool isMovingLeft = (dx < -moveThreshold);
    bool isMovingRight = (dx > moveThreshold);
    
    // 判定基準を満たした場合のみプレイヤーを移動させる
    if (isMovingLeft || isMovingRight) {
        const float sens = 0.05f; // 感度アップ
        leverX += dx * sens;
        leverX = std::clamp(leverX, -1.0f, 1.0f);
    }
    
    // 減衰（中央に戻る処理）
    leverX += (0.0f - leverX) * 0.1f;

    // Transform反映
    float moveRange = 3.0f;
    float playerX = leverX * moveRange;
    auto pos = actor->transform.GetPosition();
    pos.x = playerX; 
    actor->transform.SetPosition(pos);


    // サイドレーン制御
    if (ingameScene->lanePanels.size() >= 6) {
        
        auto* managerActor = ingameScene->managerActor.Target();
        app::test::NoteManager* noteManager = nullptr;
        app::test::SoundComponent* sound = nullptr;
        if (managerActor) {
            noteManager = managerActor->GetComponent<app::test::NoteManager>();
            sound = managerActor->GetComponent<app::test::SoundComponent>();
        }

        // --- 左サイドレーン (Index 4) ---
        auto leftSide = ingameScene->lanePanels[4].Target();
        if (leftSide) {
            auto mesh = leftSide->GetComponent<sf::Mesh>();
            if (mesh) {
                if (isMovingLeft && !wasInLeftEdge) { // Entry
                    mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f }); 
                     if (noteManager) {
                         float now = noteManager->GetSongTime();
                         JudgeResult result = noteManager->JudgeLane(4, now);
                         if (sound && app::test::gGameConfig.enableTapSound) {
                            if (result == JudgeResult::Skip) sound->Play(app::test::SoundComponent::Sfx::EmptyTap);
                            else sound->Play(app::test::SoundComponent::Sfx::Tap);
                         }
                    }
                }
                else if (isMovingLeft) { // Hold
                    mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
                    if (noteManager) noteManager->CheckHold(4, true);
                }
                else if (!isMovingLeft && wasInLeftEdge) { // Exit
                    mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
                    if (noteManager) noteManager->CheckHold(4, false);
                }
            }
        }

        // --- 右サイドレーン (Index 5) ---
        auto rightSide = ingameScene->lanePanels[5].Target();
        if (rightSide) {
            auto mesh = rightSide->GetComponent<sf::Mesh>();
            if (mesh) {
                if (isMovingRight && !wasInRightEdge) { // Entry
                    mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
                    if (noteManager) {
                         float now = noteManager->GetSongTime();
                         JudgeResult result = noteManager->JudgeLane(5, now);
                         if (sound && app::test::gGameConfig.enableTapSound) {
                            if (result == JudgeResult::Skip) sound->Play(app::test::SoundComponent::Sfx::EmptyTap);
                            else sound->Play(app::test::SoundComponent::Sfx::Tap);
                         }
                    }
                }
                else if (isMovingRight) { // Hold
                    mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
                    if (noteManager) noteManager->CheckHold(5, true);
                }
                else if (!isMovingRight && wasInRightEdge) { // Exit
                    mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
                    if (noteManager) noteManager->CheckHold(5, false);
                }
            }
        }

        // 状態更新
        wasInLeftEdge = isMovingLeft;
        wasInRightEdge = isMovingRight;
    }


    // -----------------------------
    // 各レーンのキー処理
    // -----------------------------
    struct LaneKey { int idx; Key key; };
    
    // コントローラーモードONなら固定キー (D, F, J, K)
    // OFFならコンフィグのキーを使用
    Key k1 = app::test::gGameConfig.isControllerMode ? Key::KEY_D : app::test::gKeyConfig.lane1;
    Key k2 = app::test::gGameConfig.isControllerMode ? Key::KEY_F : app::test::gKeyConfig.lane2;
    Key k3 = app::test::gGameConfig.isControllerMode ? Key::KEY_J : app::test::gKeyConfig.lane3;
    Key k4 = app::test::gGameConfig.isControllerMode ? Key::KEY_K : app::test::gKeyConfig.lane4;

    LaneKey laneKeys[] = {
        {0, k1},   // レーン 0
        {1, k2},   // レーン 1
        {2, k3},   // レーン 2
        {3, k4},   // レーン 3
    };

    for (const auto& lk : laneKeys) {
        // 押し下げ中で明るく
        if (SInput::Instance().GetKey(lk.key)) {
            auto laneActor = ingameScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
            }
            
            // HOLD CHECK
            auto* managerActor = ingameScene->managerActor.Target();
            if (managerActor) {
                 if (auto* noteManager = managerActor->GetComponent<app::test::NoteManager>()) {
                     // sf::debug::Debug::Log("Player: Calling CheckHold(True) Lane=" + std::to_string(lk.idx));
                     noteManager->CheckHold(lk.idx, true);
                 } else {
                     sf::debug::Debug::Log("Player: NoteManager NOT FOUND on ManagerActor");
                 }
            } else {
                 sf::debug::Debug::Log("Player: ManagerActor Invalid");
            }
        }
        // 離したら元に戻す
        else if (SInput::Instance().GetKeyUp(lk.key)) {
            auto laneActor = ingameScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
            }

            // HOLD CHECK (Release)
            auto* managerActor = ingameScene->managerActor.Target();
            if (managerActor) {
                 if (auto* noteManager = managerActor->GetComponent<app::test::NoteManager>()) {
                     // sf::debug::Debug::Log("Player: Calling CheckHold(False) Lane=" + std::to_string(lk.idx));
                     noteManager->CheckHold(lk.idx, false);
                 }
            }
        }

        // ノート判定
        if (SInput::Instance().GetKeyDown(lk.key)) {
            auto* managerActor = ingameScene->managerActor.Target();
            if (!managerActor) continue;
            auto* noteManager = managerActor->GetComponent<app::test::NoteManager>();
            if (!noteManager) continue;

            float now = noteManager->GetSongTime();
            JudgeResult result = noteManager->JudgeLane(lk.idx, now);

            auto* sound = managerActor->GetComponent<app::test::SoundComponent>();
            if (sound && app::test::gGameConfig.enableTapSound) {
                if (result == JudgeResult::Skip)
                    sound->Play(app::test::SoundComponent::Sfx::EmptyTap);
                else
                    sound->Play(app::test::SoundComponent::Sfx::Tap);
            }

            auto* canvas = managerActor->GetComponent<app::test::IngameCanvas>();
            if (canvas) {
               // canvas->SetJudgeImage(result);
            }
        }
    }

    // -----------------------------
    // ※ 左クリックでデバッグ座標出力
    // -----------------------------
    if (SInput::Instance().GetMouseDown(0))
    {
        POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
        Vector3 pos = ScreenToWorldOnPlane((float)mp.x, (float)mp.y, 0.0f);
       // sf::debug::Debug::Log("mouse: " + std::to_string(mp.x) + ", " + std::to_string(mp.y));
    }
}
