#include "NoteComponent.h"
#include "sf/Time.h"
#include "IngameScene.h"
#include <cmath>
#include <functional>

namespace app::test {

    NoteComponent::NoteComponent() : sf::Component(), isActive(false) {}

    /// 初期化処理 - シーンからレーンパラメータを取得し、初期座標を設定する
    void NoteComponent::Begin() {
        // シーンへの参照を取得
        auto actor = actorRef.Target();
        if (!actor) return;
        auto* scene = &actor->GetScene();
        auto* ingameScene = dynamic_cast<IngameScene*>(scene);
        if (!ingameScene) return;

        // ノートマネージャーを取得
        if (auto mgrActor = ingameScene->managerActor.Target()) {
            noteManager = mgrActor->GetComponent<NoteManager>();
        }

        // レーンレイアウト情報をシーンから取得
        lanes = ingameScene->lanes;
        laneW = ingameScene->laneW;
        laneH = ingameScene->laneH;
        rotX = ingameScene->rotX;
        baseY = ingameScene->baseY;
        barRatio = ingameScene->barRatio;

        // 上段レーン（4以上）はY座標をオフセット
        if (info.lane >= 4) {
            baseY += 1.0f;
        }

        // 傾斜パラメータを事前計算
        slopeRad = rotX * 3.14159265f / 180.0f;
        startZ = laneH * 0.5f;                      // 出現位置（奥側）
        endZ = -laneH * 0.5f + laneH * barRatio;    // 判定ライン位置（手前側）
        startY = baseY - std::tan(slopeRad) * startZ;
        endY = baseY - std::tan(slopeRad) * endZ;

        // 初期座標とスケールを設定
        float currentX = actor->transform.GetPosition().x;
        actor->transform.SetPosition({ currentX, startY, startZ });
        actor->transform.SetRotation({ rotX, 0, 0 });

        // スキルノートはレーン全幅、通常ノートはレーン幅の80%
        if (info.type == NoteType::Skill) {
            actor->transform.SetScale({ laneW * 4.0f, 0.5f, 0.2f });
        } else {
            actor->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
        }

        // ホールド開始ノートの初期配置
        if (info.type == NoteType::HoldStart) {
             // 回転補正係数を計算（傾斜面でのZ距離を正しく表現するため）
             float correction = 1.0f;
             if (std::abs(std::cos(slopeRad)) > 0.001f) {
                 correction = 1.0f / std::cos(slopeRad);
             }

             // ホールドノートの視覚的な長さを計算
             float len = info.duration * noteSpeed;
             float visualLen = len * correction;
             actor->transform.SetScale({ laneW * 0.8f, 0.5f, visualLen });
             
             // 先頭がstartZに来るよう中心座標をずらす
             float initZ = startZ + len * 0.5f;
             float initY = baseY - std::tan(slopeRad) * initZ;
             actor->transform.SetPosition({ currentX, initY, initZ });

             // ホールド開始ノートは赤色で表示
             if (auto mesh = actor->GetComponent<sf::Mesh>()) {
                 mesh->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
             }
        }
        // ホールド終了ノートは青色で表示
        else if (info.type == NoteType::HoldEnd) {
             if (auto mesh = actor->GetComponent<sf::Mesh>()) {
                 mesh->material.SetColor({ 0.0f, 0.0f, 1.0f, 1.0f });
             }
        }

        // 出現時刻と経過時間を初期化
        spawnTime = info.hittime - leadTime;
        elapsed = 0.f;

        // 更新コマンドをバインド
        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    /// 毎フレームの更新処理 - ノートの位置をスクロールさせる
    void NoteComponent::Update(const sf::command::ICommand&) {
        if (!isActive) return;
        if (noteManager.isNull()) return;

        auto actor = actorRef.Target();
        if (!actor) return;

        // 現在の楽曲時刻を取得
        float currentSongTime = noteManager->GetSongTime();

        // 判定時刻までの残り時間を算出
        float timeUntilHit = info.hittime - currentSongTime;

        // 残り時間からZ座標を算出（判定ライン + 残り時間 × 速度）
        float z = endZ + (timeUntilHit * noteSpeed);

        // 傾斜に合わせてY座標を補正
        float y = baseY - std::tan(slopeRad) * z;

        // --- ホールド開始ノートのクリッピング処理 ---
        if (info.type == NoteType::HoldStart) {
            // 回転補正係数（傾斜面でのZ距離の補正用）
            float correction = 1.0f;
            if (std::abs(std::cos(slopeRad)) > 0.001f) {
                correction = 1.0f / std::cos(slopeRad);
            }

            float originalLen = info.duration * noteSpeed;
            float headZ = z;                    // 先頭のZ座標
            float tailZ = headZ + originalLen;  // 末尾のZ座標

            // 判定ラインを超えた場合のクリッピング（先頭側）
            if (headZ < endZ) {
                 float consumedLen = endZ - headZ;
                 float visibleLen = originalLen - consumedLen;

                 if (visibleLen <= 0.001f) {
                     // 完全に通過した場合は非表示
                     actor->transform.SetScale({ 0, 0, 0 });
                 } else {
                     // 先頭を判定ラインにクランプ
                     headZ = endZ;
                     tailZ = headZ + visibleLen;

                     // 末尾が出現ラインを超える場合もクリップ
                     if (tailZ > startZ) {
                         visibleLen = startZ - headZ;
                     }

                     // クリップ後の中心座標とスケールを適用
                     float newLen = visibleLen;
                     float newCenterZ = headZ + newLen * 0.5f;
                     z = newCenterZ;
                     y = baseY - std::tan(slopeRad) * z;
                     actor->transform.SetScale({ laneW * 0.8f, 0.5f, newLen * correction });
                 }
            }
            // 通常スクロール中（先頭が判定ラインより手前）
            else {
                 // 末尾が出現ラインを超える場合のクリッピング
                 if (tailZ > startZ) {
                    float visibleLen = startZ - headZ;
                    
                    if (visibleLen <= 0.001f) {
                        actor->transform.SetScale({ 0, 0, 0 });
                    } else {
                        // 末尾をクリップして中心座標を再計算
                        float newLen = visibleLen;
                        float newCenterZ = headZ + newLen * 0.5f;
                        z = newCenterZ;
                        y = baseY - std::tan(slopeRad) * z;
                        actor->transform.SetScale({ laneW * 0.8f, 0.5f, newLen * correction });
                    }
                } else {
                    // 全体が可視範囲内 - 先頭から中心へオフセット
                    z += originalLen * 0.5f; 
                    y = baseY - std::tan(slopeRad) * z;

                    // スケールが正しくない場合のみ更新（パフォーマンス最適化）
                    auto scale = actor->transform.GetScale();
                    float targetLen = originalLen * correction;
                    if (std::abs(scale.z - targetLen) > 0.001f || scale.x == 0.0f) {
                        actor->transform.SetScale({ laneW * 0.8f, 0.5f, targetLen });
                    }
                }
            }
        }

        // 座標を適用
        auto pos = actor->transform.GetPosition();
        pos.y = y;
        pos.z = z;
        actor->transform.SetPosition(pos);
    }

    /// ノートを有効化する（オブジェクトプールからの再利用時に呼ばれる）
    void NoteComponent::Activate() {
        // 初回のActivateはBegin直後に呼ばれるためスキップ
        if (skipFirstActivate) {
            skipFirstActivate = false;
            return;
        }
        isActive = true;
        elapsed = spawnTime;
        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    /// ノートを無効化する（判定済み・画面外に出た場合など）
    void NoteComponent::DeActivate() {
        isActive = false;
        updateCommand.UnBind();
    }

} // namespace app::test
