#include "NoteComponent.h"
#include "Time.h"
#include "TestScene.h"
#include <cmath>
#include <functional>

namespace app::test {

    NoteComponent::NoteComponent() : sf::Component(), isActive(false) {}

    void NoteComponent::Begin() {
        auto actor = actorRef.Target();
        if (!actor) return;
        auto* scene = &actor->GetScene();
        auto* testScene = dynamic_cast<TestScene*>(scene);
        if (!testScene) return;

        // ===== レーン情報を取得 =====
        lanes = testScene->lanes;
        laneW = testScene->laneW;
        laneH = testScene->laneH;
        rotX = testScene->rotX;
        baseY = testScene->baseY;
        barRatio = testScene->barRatio;

        // ===== ノーツの初期位置を算出 =====
        slopeRad = rotX * 3.14159265f / 180.0f;
        startZ = laneH * 0.5f;                      // 出現位置（奥）
        endZ = -laneH * 0.5f + laneH * barRatio;  // 判定バー位置（手前）
        startY = baseY - std::tan(slopeRad) * startZ;
        endY = baseY - std::tan(slopeRad) * endZ;

        // lane index から X座標を計算
        float x = (info.lane - lanes * 0.5f + 0.5f) * laneW;

        actor->transform.SetPosition({ x, startY, startZ });
        actor->transform.SetRotation({ rotX, 0, 0 });
        actor->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });

        spawnTime = info.hittime - leadTime;
        elapsed = 0.f;

        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    void NoteComponent::Update(const sf::command::ICommand&) {
        if (!isActive) return;
        auto actor = actorRef.Target();
        if (!actor) return;

        elapsed += sf::Time::DeltaTime();
        if (elapsed < spawnTime) return;

        float timeSinceSpawn = elapsed - spawnTime;

        // ===== Z方向へ等速移動 =====
        float z = startZ - noteSpeed * timeSinceSpawn;
        float y = baseY - std::tan(slopeRad) * z; // 傾斜に沿ってY補正

        auto pos = actor->transform.GetPosition();
        pos.y = y;
        pos.z = z;
        actor->transform.SetPosition(pos);
        
    }

    void NoteComponent::Activate() {
        if (skipFirstActivate) {
            skipFirstActivate = false;
            return;
        }
        isActive = true;
        elapsed = spawnTime;
        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    void NoteComponent::DeActivate() {
        isActive = false;
        updateCommand.UnBind();
    }

} // namespace app::test
