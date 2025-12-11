#include "PlayerComponent.h"
#include "SInput.h"
#include "NoteManager.h"
#include "TestScene.h"
#include "TestCanvas.h"
#include "SoundComponent.h"

using namespace app::test;

void PlayerComponent::Begin() {
    updateCommand.Bind(std::bind(&PlayerComponent::Update, this, std::placeholders::_1));

    // シーンを取得
    auto* testScene = dynamic_cast<TestScene*>(&actorRef.Target()->GetScene());
    if (!testScene) return;

    // ジャッジバーが存在すれば位置を合わせる
    if (testScene->judgeBar.Target()) {
        Vector3 barPos = testScene->judgeBar.Target()->transform.GetPosition();

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

    // カメラを使わず「画面空間を直接Z平面に投影」
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
    auto* testScene = dynamic_cast<TestScene*>(&scene);
    if (!testScene) return;

    HWND hwnd = sf::dx::DirectX11::Instance()->GetHWND();
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);

    if (!mouseInitialized) {
        prevMouse = p;
        mouseInitialized = true;
    }

    float dx = static_cast<float>(p.x - prevMouse.x);
    prevMouse = p;

    // ==== デッドゾーン（小さい動きを無視） ====
    const float deadZone = 3.0f; // ← この値を上げると反応鈍くなる（3～10くらいが良い）
    if (fabs(dx) < deadZone) dx = 0.0f;

    // ==== 感度 ====
    const float sens = 0.005f;
    leverX += dx * sens;
    leverX = std::clamp(leverX, -1.0f, 1.0f);
    leverX += (0.0f - leverX) * 0.1f;

    // === ここでTransformに反映 ===
    float moveRange = 3.0f;  // レーン幅に合わせて調整
    float playerX = leverX * moveRange;

    auto pos = actor->transform.GetPosition();
    pos.x = playerX;                     // ← Xだけ動かす
    actor->transform.SetPosition(pos);   // ← 再セット！

    //sf::debug::Debug::Log("LeverX: " + std::to_string(leverX) +
    //    "  Xpos: " + std::to_string(pos.x));

    // -----------------------------
    // 各レーンのキー処理
    // -----------------------------
    struct LaneKey { int idx; Key key; };
    LaneKey laneKeys[] = {
        {0, Key::KEY_A},   // レーン 0
        {1, Key::KEY_S},   // レーン 1
        {2, Key::KEY_D},   // レーン 2
        {3, Key::KEY_F},   // レーン 3
    };

    for (const auto& lk : laneKeys) {
        // 押下中で明るく
        if (SInput::Instance().GetKey(lk.key)) {
            auto laneActor = testScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
            }
        }
        // 離したら元に戻す
        else if (SInput::Instance().GetKeyUp(lk.key)) {
            auto laneActor = testScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
            }
        }

        // ノーツ判定
        if (SInput::Instance().GetKeyDown(lk.key)) {
            auto* managerActor = testScene->managerActor.Target();
            if (!managerActor) continue;
            auto* noteManager = managerActor->GetComponent<app::test::NoteManager>();
            if (!noteManager) continue;

            float now = noteManager->GetSongTime();
            JudgeResult result = noteManager->JudgeLane(lk.idx, now);

            auto* sound = managerActor->GetComponent<app::test::SoundComponent>();
            if (sound) {
                if (result == JudgeResult::Skip)
                    sound->Play(app::test::SoundComponent::Sfx::EmptyTap);
                else
                    sound->Play(app::test::SoundComponent::Sfx::Tap);
            }

            auto* canvas = managerActor->GetComponent<app::test::TestCanvas>();
            if (canvas) {
               // canvas->SetJudgeImage(result);
            }
        }
    }

    // -----------------------------
    // ③ 左クリックでデバッグ座標出力
    // -----------------------------
    if (SInput::Instance().GetMouseDown(0))
    {
        POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
        Vector3 pos = ScreenToWorldOnPlane((float)mp.x, (float)mp.y, 0.0f);
       // sf::debug::Debug::Log("mouse: " + std::to_string(mp.x) + ", " + std::to_string(mp.y));
    }
}
