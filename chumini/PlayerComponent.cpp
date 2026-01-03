#include "PlayerComponent.h"
#include "SInput.h"
#include "NoteManager.h"
#include "IngameScene.h"
#include "IngameCanvas.h"
#include "SoundComponent.h"

using namespace app::test;

void PlayerComponent::Begin() {
    updateCommand.Bind(std::bind(&PlayerComponent::Update, this, std::placeholders::_1));

    // 繧ｷ繝ｼ繝ｳ繧貞叙蠕・
    auto* ingameScene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene());
    if (!ingameScene) return;

    // 繧ｸ繝｣繝・ず繝舌・縺悟ｭ伜惠縺吶ｌ縺ｰ菴咲ｽｮ繧貞粋繧上○繧・
    if (ingameScene->judgeBar.Target()) {
        Vector3 barPos = ingameScene->judgeBar.Target()->transform.GetPosition();

        // 蟆代＠荳翫↓驟咲ｽｮ・郁ｦ九◆逶ｮ隱ｿ謨ｴOK・・
        actorRef.Target()->transform.SetPosition({
            0.0f,          // X縺ｯ荳ｭ螟ｮ
			barPos.y + 3.0f,  // 蟆代＠荳翫↓
			barPos.z + 1.0f  // 蟆代＠謇句燕縺ｫ
            });
    }
}


// ======================================
// 繝槭え繧ｹ菴咲ｽｮ 竊・Z=targetZ 蟷ｳ髱｢縺ｮ繝ｯ繝ｼ繝ｫ繝牙ｺｧ讓吝､画鋤・井ｻｻ諢擾ｼ・
// ======================================
Vector3 ScreenToWorldOnPlane(float mouseX, float mouseY, float targetZ = 0.0f)
{
    HWND hwnd = sf::dx::DirectX11::Instance()->GetHWND();
    RECT rect;
    GetClientRect(hwnd, &rect);
    float screenW = static_cast<float>(rect.right - rect.left);
    float screenH = static_cast<float>(rect.bottom - rect.top);

    // 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ荳ｭ蠢・ｒ蜴溽せ縺ｫ豁｣隕丞喧 (-1 ・・1)
    float nx = (2.0f * mouseX / screenW - 1.0f);
    float ny = (1.0f - 2.0f * mouseY / screenH);

    // 繧ｫ繝｡繝ｩ繧剃ｽｿ繧上★縲檎判髱｢遨ｺ髢薙ｒ逶ｴ謗･Z蟷ｳ髱｢縺ｫ謚募ｽｱ縲・
    const float camDist = 5.0f; // 莉ｮ縺ｮ繧ｫ繝｡繝ｩ霍晞屬

    Vector3 camPos(0.0f, 0.0f, -camDist);
    Vector3 rayDir(nx, ny, 1.0f);
    rayDir = rayDir.Normarize();

    // Z=targetZ 蟷ｳ髱｢縺ｨ莠､蟾ｮ縺吶ｋ菴咲ｽｮ繧呈ｱゅａ繧・
    float t = (targetZ - camPos.z) / rayDir.z;
    Vector3 hitPos = camPos + rayDir * t;

    return hitPos;
}


// ======================================
// 繝｡繧､繝ｳ繧｢繝・・繝・・繝亥・逅・
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
        prevMouse = p;
        mouseInitialized = true;
    }

    float dx = static_cast<float>(p.x - prevMouse.x);
    prevMouse = p;

    // ==== 繝・ャ繝峨だ繝ｼ繝ｳ・亥ｰ上＆縺・虚縺阪ｒ辟｡隕厄ｼ・====
    const float deadZone = 3.0f; // 竊・縺薙・蛟､繧剃ｸ翫￡繧九→蜿榊ｿ憺・縺上↑繧具ｼ・・・0縺上ｉ縺・′濶ｯ縺・ｼ・
    if (fabs(dx) < deadZone) dx = 0.0f;

    // ==== 諢溷ｺｦ ====
    const float sens = 0.005f;
    leverX += dx * sens;
    leverX = std::clamp(leverX, -1.0f, 1.0f);
    leverX += (0.0f - leverX) * 0.1f;

    // === 縺薙％縺ｧTransform縺ｫ蜿肴丐 ===
    float moveRange = 3.0f;  // 繝ｬ繝ｼ繝ｳ蟷・↓蜷医ｏ縺帙※隱ｿ謨ｴ
    float playerX = leverX * moveRange;

    auto pos = actor->transform.GetPosition();
    pos.x = playerX;                     // 竊・X縺�縺大虚縺九☆
    actor->transform.SetPosition(pos);   // 竊・蜀阪そ繝・ヨ・・

    //sf::debug::Debug::Log("LeverX: " + std::to_string(leverX) +
    //    "  Xpos: " + std::to_string(pos.x));

    // -----------------------------
    // 蜷・Ξ繝ｼ繝ｳ縺ｮ繧ｭ繝ｼ蜃ｦ逅・
    // -----------------------------
    struct LaneKey { int idx; Key key; };
    LaneKey laneKeys[] = {
        {0, Key::KEY_A},   // 繝ｬ繝ｼ繝ｳ 0
        {1, Key::KEY_S},   // 繝ｬ繝ｼ繝ｳ 1
        {2, Key::KEY_D},   // 繝ｬ繝ｼ繝ｳ 2
        {3, Key::KEY_F},   // 繝ｬ繝ｼ繝ｳ 3
    };

    for (const auto& lk : laneKeys) {
        // 謚ｼ荳倶ｸｭ縺ｧ譏弱ｋ縺・
        if (SInput::Instance().GetKey(lk.key)) {
            auto laneActor = ingameScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f });
            }
        }
        // 髮｢縺励◆繧牙・縺ｫ謌ｻ縺・
        else if (SInput::Instance().GetKeyUp(lk.key)) {
            auto laneActor = ingameScene->lanePanels[lk.idx].Target();
            if (laneActor) {
                auto mesh = laneActor->GetComponent<sf::Mesh>();
                if (mesh) mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
            }
        }

        // 繝弱・繝・愛螳・
        if (SInput::Instance().GetKeyDown(lk.key)) {
            auto* managerActor = ingameScene->managerActor.Target();
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

            auto* canvas = managerActor->GetComponent<app::test::IngameCanvas>();
            if (canvas) {
               // canvas->SetJudgeImage(result);
            }
        }
    }

    // -----------------------------
    // 竭｢ 蟾ｦ繧ｯ繝ｪ繝・け縺ｧ繝・ヰ繝・げ蠎ｧ讓吝・蜉・
    // -----------------------------
    if (SInput::Instance().GetMouseDown(0))
    {
        POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
        Vector3 pos = ScreenToWorldOnPlane((float)mp.x, (float)mp.y, 0.0f);
       // sf::debug::Debug::Log("mouse: " + std::to_string(mp.x) + ", " + std::to_string(mp.y));
    }
}
