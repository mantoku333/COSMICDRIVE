#include "IngameScene.h"
#include "DirectX11.h"
#include "MoveComponent.h"
#include "PlayerComponent.h"
#include "SceneChangeComponent.h"
#include "IngameCanvas.h"
#include "SoundComponent.h"
#include "FollowCameraComponent.h"
#include "BGMComponent.h"

#include "ControlCamera.h"
#include "NoteComponent.h"
#include "NoteManager.h"
#include "SongInfo.h"
#include "ScoreManager.h"
#include "SoundResource.h"
#include "GUI.h"
#include "SInput.h"

#include "EffectManager.h"
#include <Effekseer.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include "StringUtils.h"

namespace {
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;
}

/// 繝・せ繝医Λ繧ｯ繧ｿ - 繧ｲ繝ｼ繝繧ｻ繝・す繝ｧ繝ｳ繧偵げ繝ｭ繝ｼ繝舌Ν縺ｫ繧ｳ繝斐・縺励※ResultScene縺九ｉ繧ゅい繧ｯ繧ｻ繧ｹ蜿ｯ閭ｽ縺ｫ縺吶ｋ
app::test::IngameScene::~IngameScene()
{
    GameSession tempCopy = GetCurrentSession();
    SetCurrentSession(nullptr);
    GetCurrentSession() = tempCopy;
}

/// 繧ｷ繝ｼ繝ｳ蛻晄悄蛹・- 繧ｫ繝｡繝ｩ繝ｻ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ繝ｻ繝ｬ繝ｼ繝ｳ繝ｻLive2D繝ｻ閭梧勹繧ｪ繝悶ず繧ｧ繧ｯ繝医ｒ逕滓・縺吶ｋ
void app::test::IngameScene::Init()
{
	ShowCursor(FALSE);

	// ===== 繧ｫ繝｡繝ｩ逕滓・ =====
	{
		auto camera = Instantiate();
		camera.Target()->AddComponent<sf::Camera>();
		// 蝗ｺ螳壹き繝｡繝ｩ・医Ξ繝ｼ繝ｳ蜈ｨ菴薙′隕区ｸ｡縺帙ｋ菴咲ｽｮ・・
		camera.Target()->transform.SetPosition({ 0.0f, 20.0f, -20.0f });
		camera.Target()->transform.SetRotation({ 45.0f, 0.0f, 0.0f });
        
        // 繝・ヰ繝・げ逕ｨ繧ｫ繝｡繝ｩ繧ｳ繝ｳ繝医Ο繝ｼ繝ｫ・亥承繧ｯ繝ｪ繝・け + WASD・・
        camera.Target()->AddComponent<app::ControlCamera>();
	}

	// ===== 繝槭ロ繝ｼ繧ｸ繝｣繝ｼ繝ｻ繧ｳ繝ｳ繝昴・繝阪Φ繝育函謌・=====
	{
		managerActor = Instantiate();
		auto noteManager = managerActor.Target()->AddComponent<app::test::NoteManager>();
		
		// 萓晏ｭ俶ｧ豕ｨ蜈･
		noteManager->SetSongInfo(&selectedSong);
		noteManager->SetSkillCallback([this]() { TriggerSkillEffect(); });
		
		auto ingameCanvas = managerActor.Target()->AddComponent<IngameCanvas>();
		ingameCanvas->SetSongInfo(&selectedSong);
		
		managerActor.Target()->AddComponent<app::test::SoundComponent>();
		managerActor.Target()->AddComponent<SceneChangeComponent>();
	   
		bgmPlayer = managerActor.Target()->AddComponent<app::test::BGMComponent>();

		auto effectMgr = managerActor.Target()->AddComponent<app::test::EffectManager>();

		// 繝励Ξ繧､繝､繝ｼ
		auto player = Instantiate();
		auto mesh = player.Target()->AddComponent<sf::Mesh>();
		mesh->SetGeometry(g_cube);
		player.Target()->transform.SetScale({ 0.5f, 0.5f, 0.5f });
		mesh->material.SetColor({ 0.3f, 0.3f, 1.0f, 0.1f });
		
		auto pComp = player.Target()->AddComponent<PlayerComponent>();
		
	}

    // ===== 繝ｬ繝ｼ繝ｳ驟咲ｽｮ =====
    {
        lanePanels.clear();

        // 繝｡繧､繝ｳ繝ｬ繝ｼ繝ｳ逕滓・
        for (int i = 0; i < lanes; ++i)
        {
            float localX = (i - lanes * 0.5f + 0.5f) * laneW;

            auto lane = Instantiate();
            auto mLane = lane.Target()->AddComponent<sf::Mesh>();
            mLane->SetGeometry(g_cube);

            lane.Target()->transform.SetScale({ laneW, 0.1f, laneH });
            lane.Target()->transform.SetPosition({ localX, baseY, 0.0f });
            lane.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

            mLane->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
            lanePanels.push_back(lane);
        }

        // 繧ｵ繧､繝峨Ξ繝ｼ繝ｳ・亥ｷｦ蜿ｳ縺ｮ迚ｹ谿翫ぐ繝溘ャ繧ｯ逕ｨ・・
        {
            float mainHalfWidth = (lanes * laneW) * 0.5f;
            float sideLaneW = laneW;

            float leftSideX = -mainHalfWidth - (sideLaneW * 0.5f);
            float rightSideX = mainHalfWidth + (sideLaneW * 0.5f);

            float sidePositions[] = { leftSideX, rightSideX };

            for (float posX : sidePositions)
            {
                auto sideLane = Instantiate();
                auto mesh = sideLane.Target()->AddComponent<sf::Mesh>();
                mesh->SetGeometry(g_cube);

                sideLane.Target()->transform.SetScale({ sideLaneW, 0.1f, laneH });
                sideLane.Target()->transform.SetPosition({ posX, baseY + 1, 0.0f });
                sideLane.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
                lanePanels.push_back(sideLane);
            }
        }

        // 繝ｬ繝ｼ繝ｳ蛹ｺ蛻・ｊ邱・
        {
            const float lineThickness = 0.05f;
            const float lineHeight = 0.05f;
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            for (int i = 1; i < lanes; ++i)
            {
                float localX = (i - lanes * 0.5f) * laneW;

                float offsetZ = 0.0f;
                float offsetY = std::tan(slopeRad) * offsetZ;

                auto line = Instantiate();
                auto mLine = line.Target()->AddComponent<sf::Mesh>();
                mLine->SetGeometry(g_cube);

                line.Target()->transform.SetScale({ lineThickness, lineHeight, laneH });
                line.Target()->transform.SetPosition({ localX, baseY + offsetY + 0.05f, offsetZ });
                line.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mLine->material.SetColor({ 1, 1, 1, 1 });
            }
        }

        // 繝ｬ繝ｼ繝ｳ螟匁棧・亥ｷｦ蜿ｳ繧ｨ繝・ず・・
        {
            laneEdges.clear(); 

            const float edgeThickness = 0.08f;
            const float edgeHeight = 0.1f;
            const float extendLength = 1.0f;
            const float edgeMargin = 0.05f;
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            float halfWidth = (lanes * 0.5f) * laneW;

            struct EdgeInfo { float x; float scaleX; };
            std::vector<EdgeInfo> edges = {
                { -halfWidth - extendLength * 0.5f - edgeMargin, edgeThickness + extendLength },
                {  halfWidth + extendLength * 0.5f + edgeMargin, edgeThickness + extendLength },
            };

            for (auto& e : edges)
            {
                auto edge = Instantiate();
                auto mEdge = edge.Target()->AddComponent<sf::Mesh>();
                mEdge->SetGeometry(g_cube);

                edge.Target()->transform.SetScale({ e.scaleX, edgeHeight, laneH });
                edge.Target()->transform.SetPosition({ e.x, baseY + 0.05f, 0.0f });
                edge.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mEdge->material.SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });

                laneEdges.push_back(edge);
            }
        }

        // 蛻､螳壹ヰ繝ｼ・域焔蜑阪Λ繧､繝ｳ・・
        {
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            const float halfH = laneH * 0.5f;
            float zPos = -halfH + laneH * barRatio;
            float yPos = -std::tan(slopeRad) * zPos;

            judgeBar = Instantiate(); 
            auto mBar = judgeBar.Target()->AddComponent<sf::Mesh>();
            mBar->SetGeometry(g_cube);

            judgeBar.Target()->transform.SetScale({ lanes * laneW, 0.1f, 0.05f });
            judgeBar.Target()->transform.SetPosition({ 0.0f, baseY + yPos + 0.06f, zPos });
            judgeBar.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

            mBar->material.SetColor({ 1, 0, 1, 1 });
        }

		// Live2D繝｢繝・Ν縺ｮ驟咲ｽｮ
		{
			auto live2dActor = Instantiate();
			l2dComp = live2dActor.Target()->AddComponent<Live2DComponent>();
			if (l2dComp.Get()) {
				const std::string MODEL_DIR = "Assets/Live2D/CyberCat";
				const std::string MODEL_FILE = "CyberCat.model3.json";

				l2dComp->LoadModel(MODEL_DIR, MODEL_FILE);

				// 菴咲ｽｮ繝ｻ繧ｹ繧ｱ繝ｼ繝ｫ隱ｿ謨ｴ
				live2dActor.Target()->transform.SetPosition({ 0.0f, 0.80f, 0.0f }); 
				live2dActor.Target()->transform.SetScale({ 0.49f, 0.7f, 1.0f });

				// 繧｢繧､繝峨Ν繝｢繝ｼ繧ｷ繝ｧ繝ｳ髢句ｧ・
				l2dComp->PlayMotion("Idle", 0, 3);
				
                // 繧ｰ繝ｪ繝・メ繝｢繝ｼ繧ｷ繝ｧ繝ｳ髢句ｧ・
                l2dComp->StartGlitchMotion("GlitchNoise", 0);
			}
		}
        
    }

    // ===== 繧ｹ繧ｭ繝ｫSE縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ =====
    auto seActor = Instantiate();
    skillSePlayer = seActor.Target()->AddComponent<sf::sound::SoundPlayer>();
    if (!skillSePlayer.isNull()) {
        skillSeResource = sf::ref::Ref<sf::sound::SoundResource>(new sf::sound::SoundResource());
        if (FAILED(skillSeResource.Target()->LoadSound("Assets/Sound/Skill.wav", false))) {
             sf::debug::Debug::LogError("Failed to load Skill.wav");
        } else {
             skillSePlayer->SetResource(skillSeResource);
             skillSePlayer->SetVolume(8.0f);
        }
    }

    // NoteManager縺ｫ繝ｬ繝ｼ繝ｳ繝代Λ繝｡繝ｼ繧ｿ繧定ｨｭ螳・
    if (auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>())
    {
        float mainHalfWidth = (4 * laneW) * 0.5f;
        float sideLaneW = laneW;
        float leftX = -mainHalfWidth - (sideLaneW * 0.5f);
        float rightX = mainHalfWidth + (sideLaneW * 0.5f);

        noteMgr->SetLaneParams(lanePanels, laneW, laneH, rotX, baseY, barRatio, leftX, rightX);
    }

    // ===== 閭梧勹3D繧ｪ繝悶ず繧ｧ繧ｯ繝医・逕滓・ =====
    bgObjects.clear();
    for(int i=0; i<200; ++i) {
        auto obj = Instantiate();
        auto mesh = obj.Target()->AddComponent<sf::Mesh>();
        mesh->SetGeometry(g_cube);
        
        // 繝ｬ繝ｼ繝ｳ荳ｭ螟ｮ繧帝∩縺代※蟾ｦ蜿ｳ縺ｫ驟咲ｽｮ
        float x = 0.0f;
        if (rand() % 2 == 0) {
             x = -10.0f - (rand() % 300) * 0.1f;  // 蟾ｦ蛛ｴ
        } else {
             x = 10.0f + (rand() % 300) * 0.1f;   // 蜿ｳ蛛ｴ
        }

        float y = ((rand() % 350) - 200) * 0.1f;  // 蝙ら峩譁ｹ蜷代↓蛻・淵
        float z = -40.0f + (rand() % 600) * 0.1f;  // 螂･陦後″譁ｹ蜷代↓蛻・淵
        
        obj.Target()->transform.SetPosition({x, y, z});
        
        // 繝ｩ繝ｳ繝繝縺ｪ繧ｵ繧､繧ｺ
        float s = 0.5f + (rand() % 15) * 0.1f;
        obj.Target()->transform.SetScale({s, s, s});
        
        // 繝ｩ繝ｳ繝繝縺ｪ蝗櫁ｻ｢
        float rx = (float)(rand() % 360);
        float ry = (float)(rand() % 360);
        float rz = (float)(rand() % 360);
        obj.Target()->transform.SetRotation({rx, ry, rz});
        
        // 證励ａ縺ｮ蜊企乗・濶ｲ
        float r = 0.1f + (rand()%20)*0.01f;
        float g = 0.1f + (rand()%20)*0.01f;
        float b = 0.2f + (rand()%30)*0.01f;
        mesh->material.SetColor({r, g, b, 0.3f});
        
        BgObject bo;
        bo.actor = obj.Target();
        bo.rotVel = { (float)((rand()%100)-50)*0.5f, (float)((rand()%100)-50)*0.5f, (float)((rand()%100)-50)*0.5f };
        // X譁ｹ蜷代・遘ｻ蜍輔・辟｡蜉ｹ蛹厄ｼ医Ξ繝ｼ繝ｳ縺ｸ縺ｮ繝峨Μ繝輔ヨ髦ｲ豁｢・・
        bo.moveVel = { 0.0f, (float)((rand()%100)-50)*0.01f, 0.0f };
        bgObjects.push_back(bo);
    }

    CreateAudioVisualizer();

    updateCommand.Bind(std::bind(&IngameScene::Update, this, std::placeholders::_1));
}

/// 豈弱ヵ繝ｬ繝ｼ繝譖ｴ譁ｰ蜃ｦ逅・- 繧ｹ繝・・繝医・繧ｷ繝ｳ縲。GM蜷梧悄縲∬レ譎ｯ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ繧堤ｮ｡逅・☆繧・
void app::test::IngameScene::Update(const sf::command::ICommand& command)
{
	// Live2D繝｢繝・Ν縺ｮ譖ｴ譁ｰ
	if (l2dComp.Get()) {
		l2dComp->Update();
	}

	// ===== 繧ｹ繝・・繝医・繧ｷ繝ｳ =====
	if (state == State::Idle) {
		// 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ縺ｸ遘ｻ陦・
		state = State::Countdown;
		countdownTimer = 3.0f;
	}
	else if (state == State::Countdown) {
		countdownTimer -= sf::Time::DeltaTime();

        // 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ荳ｭ縺ｮ繧ｰ繝ｪ繝・メ繧ｨ繝輔ぉ繧ｯ繝・
        float t = countdownTimer;
        float glitch = 0.02f; // 繝吶・繧ｹ縺ｮ繝弱う繧ｺ
        
        // 謨ｴ謨ｰ縺ｮ蠅・阜・・,2,1,0・我ｻ倩ｿ代〒繧ｰ繝ｪ繝・メ繧貞ｼｷ蛹・
        float fracT = t - std::floor(t);
        if (fracT > 0.9f || fracT < 0.1f) {
             glitch = 0.5f;
        }
        
        // 邨ゆｺ・峩蜑搾ｼ・O陦ｨ遉ｺ莉倩ｿ托ｼ峨〒繧ｰ繝ｪ繝・メ繧呈怙螟ｧ蛹・
        if (t < 0.5f) {
             glitch = 0.8f * (1.0f - t / 0.5f);
        }

        sf::dx::DirectX11::Instance()->SetGlitchIntensity(glitch);

		// Canvas縺ｫ繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ陦ｨ遉ｺ繧帝夂衍
		if (managerActor.Target()) {
			if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
				canvas->UpdateCountdownDisplay(countdownTimer, false);
			}
		}

		if (countdownTimer <= 0.0f) {
			// 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ邨ゆｺ・竊・繧ｲ繝ｼ繝髢句ｧ・
			state = State::Playing;
            sf::dx::DirectX11::Instance()->SetGlitchIntensity(0.0f);
			
			// Canvas縺ｫSTART陦ｨ遉ｺ繧呈欠遉ｺ
			if (managerActor.Target()) {
				if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
					canvas->UpdateCountdownDisplay(0.0f, true);
				}
			}

			startDisplayTimer = 1.0f;
			StartGame();
		}
	}
	else if (state == State::Playing) {
		
		// START陦ｨ遉ｺ縺ｮ繝輔ぉ繝ｼ繝峨い繧ｦ繝・
		if (startDisplayTimer > 0.0f) {
			startDisplayTimer -= sf::Time::DeltaTime();
			if (startDisplayTimer <= 0.0f) {
				if (managerActor.Target()) {
					if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
						canvas->UpdateCountdownDisplay(-1.0f, false);
					}
				}
			}
		}

		// BGM縺ｨ繝弱・繝医・繝阪・繧ｸ繝｣繝ｼ縺ｮ譎る俣蜷梧悄
		if (managerActor.Target())
		{
			auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
			if (noteMgr) {
				// BGM譛ｪ蜀咲函縺ｮ蝣ｴ蜷医∵･ｽ譖ｲ譎る俣縺・繧定ｶ・∴縺溘ｉ蜀咲函髢句ｧ・
				if (!bgmPlayer.isNull() && !bgmPlayer->IsPlaying()) {
					float currentSongTime = noteMgr->GetSongTime();
					
					if (currentSongTime >= 0.0f) {
						bgmPlayer->Play();
						noteMgr->SyncTime(bgmPlayer->GetCurrentTime()); 
					}
				}
				// BGM蜀咲函荳ｭ縺ｯ蟶ｸ縺ｫ譎る俣繧貞酔譛・
				else if (!bgmPlayer.isNull() && bgmPlayer->IsPlaying()) {
					noteMgr->SyncTime(bgmPlayer->GetCurrentTime());
				}
			}
		}

		// 閭梧勹繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ逕ｨ繧ｿ繧､繝槭・
		static float time = 0.0f;
		time += sf::Time::DeltaTime();

		// 繝・ヰ繝・げ逕ｨ繧ｹ繧ｭ繝・・・・繧ｭ繝ｼ・・
		if (SInput::Instance().GetKeyDown(Key::KEY_P)) {
			if (managerActor.Target()) {
				if (auto* noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>()) {
					noteMgr->DebugForceComplete();
				}
			}
		}

		// 繝ｬ繝ｼ繝ｳ繧ｨ繝・ず縺ｮ濶ｲ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ・郁匯濶ｲ縺ｮ繧ｹ繧ｯ繝ｭ繝ｼ繝ｫ蜉ｹ譫懶ｼ・
		for (auto& edge : laneEdges)
		{
			if (edge.Target() == nullptr) continue;
			auto mesh = edge.Target()->GetComponent<sf::Mesh>();
			if (!mesh) continue;

			float z = edge.Target()->transform.GetPosition().z;
			float scroll = time * 2.0f;
			float t = (sin(z * 0.3f + scroll) * 0.5f) + 0.5f;

			float r = 1.0f;
			float g = 0.3f + 0.4f * t;
			float b = 0.3f + 0.2f * (1.0f - t);
			float a = 1.0f;

			mesh->material.SetColor({ r, g, b, a });
		}

        // 繧ｹ繧ｭ繝ｫ繧ｨ繝輔ぉ繧ｯ繝医・貂幄｡ｰ蜃ｦ逅・
        if (skillEffectTimer > 0.0f) {
            skillEffectTimer -= sf::Time::DeltaTime();
            if (skillEffectTimer <= 0.0f) {
                skillEffectTimer = 0.0f;
                sf::dx::DirectX11::Instance()->SetGlitchIntensity(0.0f);
            } else {
                 // 繧ｰ繝ｪ繝・メ蠑ｷ蠎ｦ繧呈凾髢薙↓蠢懊§縺ｦ貂幄｡ｰ
                 float ratio = skillEffectTimer / 0.2f; 
                 float val = 0.5f * ratio;
                 sf::dx::DirectX11::Instance()->SetGlitchIntensity(val);
            }
        }
        
        // 閭梧勹3D繧ｪ繝悶ず繧ｧ繧ｯ繝医・蝗櫁ｻ｢繝ｻ遘ｻ蜍墓峩譁ｰ
        for(auto& bo : bgObjects) {
             if(auto act = bo.actor.Target()) {
                 // 蝗櫁ｻ｢譖ｴ譁ｰ
                 Vector3 rot = act->transform.GetRotation();
                 rot.x += bo.rotVel.x * sf::Time::DeltaTime();
                 rot.y += bo.rotVel.y * sf::Time::DeltaTime();
                 rot.z += bo.rotVel.z * sf::Time::DeltaTime();
                 act->transform.SetRotation(rot);
                 
                 // Y譁ｹ蜷代・遘ｻ蜍輔→繝ｫ繝ｼ繝・
                 Vector3 pos = act->transform.GetPosition();
                 pos.x += bo.moveVel.x * sf::Time::DeltaTime();
                 pos.y += bo.moveVel.y * sf::Time::DeltaTime();
                 
                 if(pos.y > 40) pos.y = -10;
                 if(pos.y < -10) pos.y = 40;
                 act->transform.SetPosition(pos);
             }
        }
	}

    UpdateAudioVisualizer();
}

/// 繧ｷ繝ｼ繝ｳ縺後い繧ｯ繝・ぅ繝悶↓縺ｪ縺｣縺滓凾縺ｮ蛻晄悄蛹門・逅・
void app::test::IngameScene::CreateAudioVisualizer()
{
    audioVisPoints.clear();
    audioVisSegments.clear();

    constexpr int kPointCount = 56;
    constexpr float kSpanX = 75.0f;
    // Keep depth fixed; move only downward in world Y.
    constexpr float kBaseY = -7.0f;
    constexpr float kBaseZ = -7.0f;
    constexpr float kLineThickness = 0.08f;
    constexpr float kLineDepth = 0.04f;

    for (int i = 0; i < kPointCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kPointCount - 1);
        const float x = (t - 0.5f) * kSpanX;
        AudioVisPoint point;
        point.x = x;
        point.z = kBaseZ;
        point.phase = t * 6.2831853f + static_cast<float>(rand() % 100) * 0.08f;
        point.smoothLevel = 0.0f;
        audioVisPoints.push_back(point);
    }

    for (int i = 0; i < kPointCount - 1; ++i) {
        auto segActor = Instantiate();
        auto mesh = segActor.Target()->AddComponent<sf::Mesh>();
        mesh->SetGeometry(g_cube);
        segActor.Target()->transform.SetPosition({ 0.0f, kBaseY, kBaseZ });
        segActor.Target()->transform.SetScale({ 0.6f, kLineThickness, kLineDepth });
        mesh->material.SetColor({ 0.10f, 0.80f, 1.00f, 1.00f });
        mesh->material.SetEmission({ 0.08f, 0.45f, 0.70f, 1.00f });

        AudioVisSegment segment;
        segment.actor = segActor.Target();
        audioVisSegments.push_back(segment);
    }
}

void app::test::IngameScene::UpdateAudioVisualizer()
{
    if (audioVisPoints.size() < 2 || audioVisSegments.empty()) {
        return;
    }

    constexpr float kBaseY = -7.0f;
    constexpr float kLineThickness = 0.08f;
    constexpr float kLineDepth = 0.04f;
    constexpr float kMinRise = 0.10f;
    constexpr float kMaxRise = 6.6f;
    const float dt = sf::Time::DeltaTime();
    const bool active = (state == State::Playing && !bgmPlayer.isNull() && bgmPlayer->IsPlaying());
    const float currentTime = (!bgmPlayer.isNull()) ? bgmPlayer->GetCurrentTime() : 0.0f;

    for (size_t i = 0; i < audioVisPoints.size(); ++i) {
        auto& point = audioVisPoints[i];
        float target = 0.0f;
        if (active) {
            const float lookAhead = static_cast<float>(i) * 0.006f;
            const float amp = bgmPlayer->GetAmplitude01(lookAhead);
            const float pulse = 0.80f + 0.20f * static_cast<float>(std::sin(currentTime * 5.0f + point.phase));
            const float compressed = amp / (amp + 0.14f);
            target = std::clamp(compressed * pulse, 0.0f, 1.0f);
        }

        const float interp = std::clamp(dt * 12.0f, 0.0f, 1.0f);
        point.smoothLevel += (target - point.smoothLevel) * interp;
    }

    const float kRadToDeg = 57.2957795f;
    const size_t segmentCount = std::min(audioVisSegments.size(), audioVisPoints.size() - 1);
    for (size_t i = 0; i < segmentCount; ++i) {
        auto* actor = audioVisSegments[i].actor.Target();
        if (actor == nullptr) {
            continue;
        }

        const auto& p0 = audioVisPoints[i];
        const auto& p1 = audioVisPoints[i + 1];

        const float y0 = kBaseY + kMinRise + (p0.smoothLevel * kMaxRise);
        const float y1 = kBaseY + kMinRise + (p1.smoothLevel * kMaxRise);

        const float dx = p1.x - p0.x;
        const float dy = y1 - y0;
        const float length = std::max(0.01f, std::sqrt(dx * dx + dy * dy));
        const float midX = (p0.x + p1.x) * 0.5f;
        const float midY = (y0 + y1) * 0.5f;
        const float midZ = (p0.z + p1.z) * 0.5f;
        const float rotZ = std::atan2(dy, dx) * kRadToDeg;

        actor->transform.SetPosition({ midX, midY, midZ });
        const float level = (p0.smoothLevel + p1.smoothLevel) * 0.5f;
        const float linePulse = 1.0f + level * 0.8f;
        actor->transform.SetScale({ length, kLineThickness * linePulse, kLineDepth });
        actor->transform.SetRotation({ 0.0f, 0.0f, rotZ });

        auto mesh = actor->GetComponent<sf::Mesh>();
        if (mesh) {
            const float glow = 0.25f + level * 0.75f;
            const float r = 0.10f + 0.30f * glow;
            const float g = 0.45f + 0.50f * glow;
            const float b = 0.75f + 0.25f * glow;
            const float a = 0.95f;
            mesh->material.SetColor({ r, g, b, a });
            mesh->material.SetEmission({ r * 0.55f, g * 0.95f, b * 1.20f, 1.0f });
        }
    }
}

void app::test::IngameScene::OnActivated()
{
    isPlaying = false;
    state = State::Idle;
    countdownTimer = 0.0f;

    // 繧ｲ繝ｼ繝繧ｻ繝・す繝ｧ繝ｳ繧偵Μ繧ｻ繝・ヨ縺励げ繝ｭ繝ｼ繝舌Ν縺ｫ逋ｻ骭ｲ
    gameSession.Reset();
    SetCurrentSession(&gameSession);

    if (selectedSong.musicPath.empty() || bgmPlayer.isNull()) {
        sf::debug::Debug::Log("[BGM] OnActivated: path empty or bgmPlayer null");
        return;
    }

    // BGM繝代せ繧定ｨｭ螳夲ｼ・hift-JIS螟画鋤・・
    std::string sjisPath = Utf8ToShiftJis(selectedSong.musicPath);
    bgmPlayer->SetPath(sjisPath);

    // 繧ｲ繝ｼ繝繧ｻ繝・す繝ｧ繝ｳ縺ｫ讌ｽ譖ｲ諠・ｱ繧定ｨｭ螳・
    gameSession.SetChartPath(selectedSong.chartPath);
    gameSession.SetDifficulty(selectedSong.difficulty);
    gameSession.SetTitle(selectedSong.title);
}

/// 繧ｲ繝ｼ繝髢句ｧ句・逅・- NoteManager繧定ｵｷ蜍輔＠繝弱・繝育ｷ乗焚繧偵そ繝・す繝ｧ繝ｳ縺ｫ險ｭ螳壹☆繧・
void app::test::IngameScene::StartGame()
{
    // 莠碁㍾襍ｷ蜍暮亟豁｢
    if (isPlaying) return;

    sf::debug::Debug::Log("Game Start");
    isPlaying = true;

    // NoteManager繧帝幕蟋・
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            int total = noteMgr->GetMaxTotalCombo();
            gameSession.SetTotalNoteCount(total);
            noteMgr->StartGame();
        }
    }

    // BGM蜀咲函縺ｯUpdate蜀・〒讌ｽ譖ｲ譎る俣縺・縺ｫ縺ｪ繧九・繧貞ｾ・▲縺ｦ縺九ｉ髢句ｧ・
}

/// 謠冗判蜃ｦ逅・- 蝓ｺ蠎墓緒逕ｻ縺ｮ蠕後↓繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ謠冗判繧貞ｮ溯｡後☆繧・
void app::test::IngameScene::Draw()
{
    // 蝓ｺ蠎墓緒逕ｻ・磯壼ｸｸ縺ｮ繧｢繧ｯ繧ｿ繝ｼ繝ｻ繧ｫ繝｡繝ｩ險ｭ螳壹↑縺ｩ・・
    sf::Scene<IngameScene>::Draw();

    // 繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ謠冗判縺ｫ繧医ｋ繝弱・繝域緒逕ｻ
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            noteMgr->DrawInstanced();
        }
    }
}

/// 繧ｪ繝ｼ繝舌・繝ｬ繧､謠冗判 - Live2D繝｢繝・Ν繧呈怙蜑埼擇縺ｫ謠冗判縺吶ｋ
void app::test::IngameScene::DrawOverlay()
{
	if (l2dComp.Get()) {
		l2dComp->Draw();
	}
}

/// 繧ｹ繧ｭ繝ｫ繝弱・繝育匱蜍墓凾縺ｮ繧ｨ繝輔ぉ繧ｯ繝亥・逅・ｼ医げ繝ｪ繝・メ + SE蜀咲函・・
void app::test::IngameScene::TriggerSkillEffect()
{
    // 繧ｰ繝ｪ繝・メ繧ｨ繝輔ぉ繧ｯ繝磯幕蟋具ｼ亥ｼｷ蠎ｦ縺ｯUpdate縺ｧ貂幄｡ｰ蛻ｶ蠕｡・・
    skillEffectTimer = 0.2f;
    
    // 繧ｹ繧ｭ繝ｫSE繧貞・逕・
    if (!skillSePlayer.isNull()) {
        skillSePlayer->Stop();
        skillSePlayer->Play(0.0f);
    }
}

/// ImGUI蜃ｦ逅・ｼ育樟蝨ｨ縺ｯ繧ｭ繝ｼ繝懊・繝峨す繝ｧ繝ｼ繝医き繝・ヨ縺ｧ莉｣譖ｿ・・
void app::test::IngameScene::OnGUI()
{
}

