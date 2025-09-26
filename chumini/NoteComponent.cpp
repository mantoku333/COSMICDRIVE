#include "NoteComponent.h"
#include "Time.h" 
#include <functional> 
#include <fstream>
#include <sstream>
#include <iostream>

#include "TestScene.h"

namespace app {
    namespace test {

        NoteComponent::NoteComponent() : sf::Component(), isActive(false) {}

        void NoteComponent::Begin() {

            // 所有アクターと対応シーン取得
            auto actor = actorRef.Target();
            if (!actor) return;
            auto* scene = &actor->GetScene();

			// TestScene にキャストして panelPos からパネルの位置を取得
            if (auto* testScene = dynamic_cast<TestScene*>(scene)) {
                startY = testScene->panelPos.y + testScene->panelH / 2.f; // パネルの上端
                hitY = testScene->barY;                                   // パネルの下端
            }

            // スポーン時刻と経過時間初期化
            spawnTime = info.hittime - leadTime;

            /* デバッグ用出力
            std::ostringstream oss;
            oss << " leadTime=" <<   leadTime;
			oss << ", spawnTime=" << spawnTime;
            sf::debug::Debug::Log(oss.str()); */

            // 初期位置をセット
            if (actor = actorRef.Target()) {
                auto pos = actor->transform.GetPosition();

                // シーンから情報を持ってくる
                auto& scene = actor->GetScene();
                if (auto* testScene = dynamic_cast<TestScene*>(&scene)) {
                    float panelWidth_ = testScene->panelW;
                    int   lanes_ = testScene->lanes;
                    float laneW = panelWidth_ / lanes_;
                    float originX = testScene->panelPos.x;
                    pos.x = originX + (-panelWidth_ / 2.f + laneW * (info.lane + 0.5f));
                }

                // Y 座標をスタート位置に
                pos.y = startY;
                pos.z = 9.9f;
                actor->transform.SetPosition(pos);

                // スケールをレーン分割数にあわせる
                auto actorScale = actor->transform.GetScale();
                float laneW = (dynamic_cast<TestScene*>(&scene))->panelW / (dynamic_cast<TestScene*>(&scene))->lanes;
                actor->transform.SetScale({ laneW - 0.1f, actorScale.y, actorScale.z });
            }

            spawnTime = info.hittime - leadTime;
            elapsed = spawnTime;

            // 毎フレーム Update を呼び出すようにバインド
            updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
        }

        void NoteComponent::Update(const sf::command::ICommand&) {
            auto actor = actorRef.Target();
            if (!actor) return;
            if (!isActive) return;

            // 開始からの時間を加算（秒）
            float delta = sf::Time::DeltaTime();
            elapsed += delta;

            // spawnTime より前は処理しない
            if (elapsed < spawnTime) {
                return;
            }

            // ノーツの出現後経過時間（秒）
            float timeSinceSpawn = elapsed - spawnTime;

            // 等速で落下させる
            auto pos = actor->transform.GetPosition();
            pos.y = startY - noteSpeed * timeSinceSpawn;
            actor->transform.SetPosition(pos);
        }


        void NoteComponent::Activate() {
            if (skipFirstActivate) {
                skipFirstActivate = false;
                return;
            }

            isActive = true;
            updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
        }

        void NoteComponent::DeActivate() {
            isActive = false;
            updateCommand.UnBind();
        }

    } // namespace test
} // namespace app
