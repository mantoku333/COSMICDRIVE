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

        if (auto mgrActor = testScene->managerActor.Target()) {
            noteManager = mgrActor->GetComponent<NoteManager>();
        }


        // ===== レーン情報を取得 =====
        lanes = testScene->lanes;
        laneW = testScene->laneW;
        laneH = testScene->laneH;
        rotX = testScene->rotX;
        baseY = testScene->baseY;
        barRatio = testScene->barRatio;

        if (info.lane >= 4) {
            baseY += 1.0f;
        }

        // ===== ノーツの初期位置を算出 =====
        slopeRad = rotX * 3.14159265f / 180.0f;
        startZ = laneH * 0.5f;                      // 出現位置（奥）
        endZ = -laneH * 0.5f + laneH * barRatio;  // 判定バー位置（手前）
        startY = baseY - std::tan(slopeRad) * startZ;
        endY = baseY - std::tan(slopeRad) * endZ;

        // lane index から X座標を計算
        float currentX = actor->transform.GetPosition().x;
        actor->transform.SetPosition({ currentX, startY, startZ });
        actor->transform.SetRotation({ rotX, 0, 0 });
        actor->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });

        spawnTime = info.hittime - leadTime;
        elapsed = 0.f;

        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    void NoteComponent::Update(const sf::command::ICommand&) {
        if (!isActive) return;

        // マネージャがいなければ時間を知れないので動かない
        if (!noteManager) return;

        auto actor = actorRef.Target();
        if (!actor) return;

        // --------------------------------------------------------
        // ★修正箇所: 経過時間ではなく「絶対時間」で位置を決める
        // --------------------------------------------------------

        // 1. マネージャから「現在の正確なBGM時間」をもらう
        float currentSongTime = noteManager->GetSongTime();

        // 2. 着弾までの残り時間を計算
        //    (着弾予定時刻 - 現在時刻)
        //    プラスなら手前、0なら着弾、マイナスなら通り過ぎた後
        float timeUntilHit = info.hittime - currentSongTime;

        // 3. 位置を決定
        //    「判定ライン(endZ)」から「速度 × 残り時間」ぶん奥(Z+)に配置
        //    ※残り時間が減るほど endZ に近づいていく
        float z = endZ + (timeUntilHit * noteSpeed);

        // 4. 傾斜に合わせてY座標を補正
        float y = baseY - std::tan(slopeRad) * z;

        // 5. 座標更新
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
