#include "NoteComponent.h"
#include "sf/Time.h"
#include "IngameScene.h"
#include <cmath>
#include <functional>

namespace app::test {

    NoteComponent::NoteComponent() : sf::Component(), isActive(false) {}

    void NoteComponent::Begin() {
        auto actor = actorRef.Target();
        if (!actor) return;
        auto* scene = &actor->GetScene();
        auto* ingameScene = dynamic_cast<IngameScene*>(scene);
        if (!ingameScene) return;

        if (auto mgrActor = ingameScene->managerActor.Target()) {
            noteManager = mgrActor->GetComponent<NoteManager>();
        }


        // ===== 繝ｬ繝ｼ繝ｳ諠・ｱ繧貞叙蠕・=====
        lanes = ingameScene->lanes;
        laneW = ingameScene->laneW;
        laneH = ingameScene->laneH;
        rotX = ingameScene->rotX;
        baseY = ingameScene->baseY;
        barRatio = ingameScene->barRatio;

        if (info.lane >= 4) {
            baseY += 1.0f;
        }

        // ===== 繝弱・繝・・蛻晄悄菴咲ｽｮ繧堤ｮ怜・ =====
        slopeRad = rotX * 3.14159265f / 180.0f;
        startZ = laneH * 0.5f;                      // 蜃ｺ迴ｾ菴咲ｽｮ・亥･･・・
        endZ = -laneH * 0.5f + laneH * barRatio;  // 蛻､螳壹ヰ繝ｼ菴咲ｽｮ・域焔蜑搾ｼ・
        startY = baseY - std::tan(slopeRad) * startZ;
        endY = baseY - std::tan(slopeRad) * endZ;

        // lane index 縺九ｉ X蠎ｧ讓吶ｒ險育ｮ・
        float currentX = actor->transform.GetPosition().x;
        actor->transform.SetPosition({ currentX, startY, startZ });
        actor->transform.SetRotation({ rotX, 0, 0 });
        if (info.type == NoteType::Skill) {
            actor->transform.SetScale({ laneW * 4.0f, 0.5f, 0.2f });
        } else {
            actor->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
        }

        // HOLD START SPECIAL HANDLING
        if (info.type == NoteType::HoldStart) {
             // Calculate visual length with rotation correction (same as Update)
             float correction = 1.0f;
             if (std::abs(std::cos(slopeRad)) > 0.001f) {
                 correction = 1.0f / std::cos(slopeRad);
             }
             float len = info.duration * noteSpeed;
             float visualLen = len * correction;

             actor->transform.SetScale({ laneW * 0.8f, 0.5f, visualLen });
             
             // Shift Z so that the Head is at startZ, and the rest extends "back" (Wait position)
             // Original: Center at startZ.
             // New: Center at startZ + len/2.
             float initZ = startZ + len * 0.5f;
             float initY = baseY - std::tan(slopeRad) * initZ;
             
             // Apply revised position
             actor->transform.SetPosition({ currentX, initY, initZ });

             // DEBUG: Red for HoldStart
             if (auto mesh = actor->GetComponent<sf::Mesh>()) {
                 mesh->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
             }
             
             sf::debug::Debug::Log("HoldStart Init: LaneW=" + std::to_string(laneW) + " InitLen=" + std::to_string(len));
        }
        else if (info.type == NoteType::HoldEnd) {
             // DEBUG: Blue for HoldEnd
             if (auto mesh = actor->GetComponent<sf::Mesh>()) {
                 mesh->material.SetColor({ 0.0f, 0.0f, 1.0f, 1.0f });
             }
        }

        spawnTime = info.hittime - leadTime;
        elapsed = 0.f;

        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    void NoteComponent::Update(const sf::command::ICommand&) {
        if (!isActive) return;

        // 繝槭ロ繝ｼ繧ｸ繝｣縺後＞縺ｪ縺代ｌ縺ｰ譎る俣繧堤衍繧後↑縺・・縺ｧ蜍輔°縺ｪ縺・
        if (noteManager.isNull()) return;

        auto actor = actorRef.Target();
        if (!actor) return;

        // DEBUG: Check if Update is running
        static float logTimer = 0.0f;
        logTimer += sf::Time::DeltaTime();
        if (logTimer > 1.0f) {
           // logTimer = 0.0f; 
        }

        // 1. 繝槭ロ繝ｼ繧ｸ繝｣縺九ｉ縲檎樟蝨ｨ縺ｮ豁｣遒ｺ縺ｪBGM譎る俣縲阪ｒ繧ゅｉ縺・
        float currentSongTime = noteManager->GetSongTime();

        // 2. 逹蠑ｾ縺ｾ縺ｧ縺ｮ谿九ｊ譎る俣繧定ｨ育ｮ・
        float timeUntilHit = info.hittime - currentSongTime;

        // 3. 菴咲ｽｮ繧呈ｱｺ螳・
        float z = endZ + (timeUntilHit * noteSpeed);

        // DEBUG: Debug log for Note Movement (Lane 3 HoldStart only for clarity)
        if (info.lane == 3 && info.type == NoteType::HoldStart) {
             // sf::debug::Debug::Log("NoteUpdate: Lane3 HoldStart T=" + std::to_string(currentSongTime) + " Z=" + std::to_string(z) + " Active=" + std::to_string(isActive)); 
        }

        // 4. 蛯ｾ譁懊↓蜷医ｏ縺帙※Y蠎ｧ讓吶ｒ陬懈ｭ｣
        float y = baseY - std::tan(slopeRad) * z;

        // HOLD START Logic
        if (info.type == NoteType::HoldStart) {
            // Rotation Correction: Length needs to be scaled by 1/cos(slope) to cover the correct World Z distance
            float correction = 1.0f;
            if (std::abs(std::cos(slopeRad)) > 0.001f) {
                correction = 1.0f / std::cos(slopeRad);
            }

            float originalLen = info.duration * noteSpeed;
            float headZ = z; // z is currently the Head position
            float tailZ = headZ + originalLen;

            // 1. Clipping at Spawn Line (startZ) - Tail Clipping
            // This is already handled by "tailZ > startZ" logic below?
            // Yes, checking if tail is too far back.

            // 2. Clipping at Judge Line (endZ) - Head Clipping
            if (headZ < endZ) {
                 float consumedLen = endZ - headZ;
                 // If the entire note is past the judge line, it should probably disappear or be handled by logic.
                 
                 float visibleLen = originalLen - consumedLen;
                 if (visibleLen <= 0.001f) {
                     actor->transform.SetScale({ 0, 0, 0 });
                 } else {
                     headZ = endZ; // Clamp head
                     tailZ = headZ + visibleLen; // Tail is derived from clamped head + remaining len

                     // Now check if Tail is ALSO clipped by startZ (e.g. giant note spanning entire screen)
                     if (tailZ > startZ) {
                         visibleLen = startZ - headZ; // Clip tail at startZ
                     }

                     // Apply
                     float newLen = visibleLen;
                     float newCenterZ = headZ + newLen * 0.5f;
                     
                     z = newCenterZ; // Update z to Center
                     y = baseY - std::tan(slopeRad) * z;
                     
                     // Restore full scale with adjusted Z AND Rotation Correction
                     actor->transform.SetScale({ laneW * 0.8f, 0.5f, newLen * correction });
                 }
            }
            // Normal Scrolling (Head is between endZ and startZ)
            else {
                 // Check Tail Clipping
                 if (tailZ > startZ) {
                    // Clip tail
                    float visibleLen = startZ - headZ;
                    
                    if (visibleLen <= 0.001f) {
                        actor->transform.SetScale({ 0, 0, 0 });
                    } else {
                        float newLen = visibleLen;
                        float newCenterZ = headZ + newLen * 0.5f;

                        z = newCenterZ; // Update z to Center
                        y = baseY - std::tan(slopeRad) * z;
                        
                        // Apply Rotation Correction
                        actor->transform.SetScale({ laneW * 0.8f, 0.5f, newLen * correction });
                    }
                } else {
                    // Fully visible
                    // Shift z from Head to Center for final transform
                    z += originalLen * 0.5f; 
                    y = baseY - std::tan(slopeRad) * z;

                    auto scale = actor->transform.GetScale();
                    float targetLen = originalLen * correction;
                    
                    if (std::abs(scale.z - targetLen) > 0.001f || scale.x == 0.0f) {
                        actor->transform.SetScale({ laneW * 0.8f, 0.5f, targetLen });
                    }
                }
            }
        }

        // 5. 蠎ｧ讓呎峩譁ｰ
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
        sf::debug::Debug::Log("NoteComponent: Activate called! Lane=" + std::to_string(info.lane) + " Type=" + std::to_string((int)info.type));
        isActive = true;
        elapsed = spawnTime;
        updateCommand.Bind(std::bind(&NoteComponent::Update, this, std::placeholders::_1));
    }

    void NoteComponent::DeActivate() {
        isActive = false;
        updateCommand.UnBind();
    }

} // namespace app::test
