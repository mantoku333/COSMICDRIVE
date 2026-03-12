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
#include "StringUtils.h"

namespace {
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;
}

/// デストラクタ - ゲームセッションをグローバルにコピーしてResultSceneからもアクセス可能にする
app::test::IngameScene::~IngameScene()
{
    GameSession tempCopy = GetCurrentSession();
    SetCurrentSession(nullptr);
    GetCurrentSession() = tempCopy;
}

/// シーン初期化 - カメラ・マネージャー・レーン・Live2D・背景オブジェクトを生成する
void app::test::IngameScene::Init()
{
	ShowCursor(FALSE);

	// ===== カメラ生成 =====
	{
		auto camera = Instantiate();
		camera.Target()->AddComponent<sf::Camera>();
		// 固定カメラ（レーン全体が見渡せる位置）
		camera.Target()->transform.SetPosition({ 0.0f, 20.0f, -20.0f });
		camera.Target()->transform.SetRotation({ 45.0f, 0.0f, 0.0f });
        
        // デバッグ用カメラコントロール（右クリック + WASD）
        camera.Target()->AddComponent<app::ControlCamera>();
	}

	// ===== マネージャー・コンポーネント生成 =====
	{
		managerActor = Instantiate();
		auto noteManager = managerActor.Target()->AddComponent<app::test::NoteManager>();
		
		// 依存性注入
		noteManager->SetSongInfo(&selectedSong);
		noteManager->SetSkillCallback([this]() { TriggerSkillEffect(); });
		
		auto ingameCanvas = managerActor.Target()->AddComponent<IngameCanvas>();
		ingameCanvas->SetSongInfo(&selectedSong);
		
		managerActor.Target()->AddComponent<app::test::SoundComponent>();
		managerActor.Target()->AddComponent<SceneChangeComponent>();
	   
		bgmPlayer = managerActor.Target()->AddComponent<app::test::BGMComponent>();

		auto effectMgr = managerActor.Target()->AddComponent<app::test::EffectManager>();

		// プレイヤー
		auto player = Instantiate();
		auto mesh = player.Target()->AddComponent<sf::Mesh>();
		mesh->SetGeometry(g_cube);
		player.Target()->transform.SetScale({ 0.5f, 0.5f, 0.5f });
		mesh->material.SetColor({ 0.3f, 0.3f, 1.0f, 0.1f });
		
		auto pComp = player.Target()->AddComponent<PlayerComponent>();
		
	}

    // ===== レーン配置 =====
    {
        lanePanels.clear();

        // メインレーン生成
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

        // サイドレーン（左右の特殊ギミック用）
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

        // レーン区切り線
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

        // レーン外枠（左右エッジ）
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

        // 判定バー（手前ライン）
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

		// Live2Dモデルの配置
		{
			auto live2dActor = Instantiate();
			l2dComp = live2dActor.Target()->AddComponent<Live2DComponent>();
			if (l2dComp.Get()) {
				const std::string MODEL_DIR = "Assets/Live2D/CyberCat";
				const std::string MODEL_FILE = "CyberCat.model3.json";

				l2dComp->LoadModel(MODEL_DIR, MODEL_FILE);

				// 位置・スケール調整
				live2dActor.Target()->transform.SetPosition({ 0.0f, 0.80f, 0.0f }); 
				live2dActor.Target()->transform.SetScale({ 0.49f, 0.7f, 1.0f });

				// アイドルモーション開始
				l2dComp->PlayMotion("Idle", 0, 3);
				
                // グリッチモーション開始
                l2dComp->StartGlitchMotion("GlitchNoise", 0);
			}
		}
        
    }

    // ===== スキルSEの読み込み =====
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

    // NoteManagerにレーンパラメータを設定
    if (auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>())
    {
        float mainHalfWidth = (4 * laneW) * 0.5f;
        float sideLaneW = laneW;
        float leftX = -mainHalfWidth - (sideLaneW * 0.5f);
        float rightX = mainHalfWidth + (sideLaneW * 0.5f);

        noteMgr->SetLaneParams(lanePanels, laneW, laneH, rotX, baseY, barRatio, leftX, rightX);
    }

    // ===== 背景3Dオブジェクトの生成 =====
    bgObjects.clear();
    for(int i=0; i<200; ++i) {
        auto obj = Instantiate();
        auto mesh = obj.Target()->AddComponent<sf::Mesh>();
        mesh->SetGeometry(g_cube);
        
        // レーン中央を避けて左右に配置
        float x = 0.0f;
        if (rand() % 2 == 0) {
             x = -10.0f - (rand() % 300) * 0.1f;  // 左側
        } else {
             x = 10.0f + (rand() % 300) * 0.1f;   // 右側
        }

        float y = ((rand() % 350) - 200) * 0.1f;  // 垂直方向に分散
        float z = -40.0f + (rand() % 600) * 0.1f;  // 奥行き方向に分散
        
        obj.Target()->transform.SetPosition({x, y, z});
        
        // ランダムなサイズ
        float s = 0.5f + (rand() % 15) * 0.1f;
        obj.Target()->transform.SetScale({s, s, s});
        
        // ランダムな回転
        float rx = (float)(rand() % 360);
        float ry = (float)(rand() % 360);
        float rz = (float)(rand() % 360);
        obj.Target()->transform.SetRotation({rx, ry, rz});
        
        // 暗めの半透明色
        float r = 0.1f + (rand()%20)*0.01f;
        float g = 0.1f + (rand()%20)*0.01f;
        float b = 0.2f + (rand()%30)*0.01f;
        mesh->material.SetColor({r, g, b, 0.3f});
        
        BgObject bo;
        bo.actor = obj.Target();
        bo.rotVel = { (float)((rand()%100)-50)*0.5f, (float)((rand()%100)-50)*0.5f, (float)((rand()%100)-50)*0.5f };
        // X方向の移動は無効化（レーンへのドリフト防止）
        bo.moveVel = { 0.0f, (float)((rand()%100)-50)*0.01f, 0.0f };
        bgObjects.push_back(bo);
    }

    updateCommand.Bind(std::bind(&IngameScene::Update, this, std::placeholders::_1));
}

/// 毎フレーム更新処理 - ステートマシン、BGM同期、背景アニメーションを管理する
void app::test::IngameScene::Update(const sf::command::ICommand& command)
{
	// Live2Dモデルの更新
	if (l2dComp.Get()) {
		l2dComp->Update();
	}

	// ===== ステートマシン =====
	if (state == State::Idle) {
		// カウントダウンへ移行
		state = State::Countdown;
		countdownTimer = 3.0f;
	}
	else if (state == State::Countdown) {
		countdownTimer -= sf::Time::DeltaTime();

        // カウントダウン中のグリッチエフェクト
        float t = countdownTimer;
        float glitch = 0.02f; // ベースのノイズ
        
        // 整数の境界（3,2,1,0）付近でグリッチを強化
        float fracT = t - std::floor(t);
        if (fracT > 0.9f || fracT < 0.1f) {
             glitch = 0.5f;
        }
        
        // 終了直前（GO表示付近）でグリッチを最大化
        if (t < 0.5f) {
             glitch = 0.8f * (1.0f - t / 0.5f);
        }

        sf::dx::DirectX11::Instance()->SetGlitchIntensity(glitch);

		// Canvasにカウントダウン表示を通知
		if (managerActor.Target()) {
			if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
				canvas->UpdateCountdownDisplay(countdownTimer, false);
			}
		}

		if (countdownTimer <= 0.0f) {
			// カウントダウン終了 → ゲーム開始
			state = State::Playing;
            sf::dx::DirectX11::Instance()->SetGlitchIntensity(0.0f);
			
			// CanvasにSTART表示を指示
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
		
		// START表示のフェードアウト
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

		// BGMとノートマネージャーの時間同期
		if (managerActor.Target())
		{
			auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
			if (noteMgr) {
				// BGM未再生の場合、楽曲時間が0を超えたら再生開始
				if (!bgmPlayer.isNull() && !bgmPlayer->IsPlaying()) {
					float currentSongTime = noteMgr->GetSongTime();
					
					if (currentSongTime >= 0.0f) {
						bgmPlayer->Play();
						noteMgr->SyncTime(bgmPlayer->GetCurrentTime()); 
					}
				}
				// BGM再生中は常に時間を同期
				else if (!bgmPlayer.isNull() && bgmPlayer->IsPlaying()) {
					noteMgr->SyncTime(bgmPlayer->GetCurrentTime());
				}
			}
		}

		// 背景アニメーション用タイマー
		static float time = 0.0f;
		time += sf::Time::DeltaTime();

		// デバッグ用スキップ（Pキー）
		if (SInput::Instance().GetKeyDown(Key::KEY_P)) {
			if (managerActor.Target()) {
				if (auto* noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>()) {
					noteMgr->DebugForceComplete();
				}
			}
		}

		// レーンエッジの色アニメーション（虹色のスクロール効果）
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

        // スキルエフェクトの減衰処理
        if (skillEffectTimer > 0.0f) {
            skillEffectTimer -= sf::Time::DeltaTime();
            if (skillEffectTimer <= 0.0f) {
                skillEffectTimer = 0.0f;
                sf::dx::DirectX11::Instance()->SetGlitchIntensity(0.0f);
            } else {
                 // グリッチ強度を時間に応じて減衰
                 float ratio = skillEffectTimer / 0.2f; 
                 float val = 0.5f * ratio;
                 sf::dx::DirectX11::Instance()->SetGlitchIntensity(val);
            }
        }
        
        // 背景3Dオブジェクトの回転・移動更新
        for(auto& bo : bgObjects) {
             if(auto act = bo.actor.Target()) {
                 // 回転更新
                 Vector3 rot = act->transform.GetRotation();
                 rot.x += bo.rotVel.x * sf::Time::DeltaTime();
                 rot.y += bo.rotVel.y * sf::Time::DeltaTime();
                 rot.z += bo.rotVel.z * sf::Time::DeltaTime();
                 act->transform.SetRotation(rot);
                 
                 // Y方向の移動とループ
                 Vector3 pos = act->transform.GetPosition();
                 pos.x += bo.moveVel.x * sf::Time::DeltaTime();
                 pos.y += bo.moveVel.y * sf::Time::DeltaTime();
                 
                 if(pos.y > 40) pos.y = -10;
                 if(pos.y < -10) pos.y = 40;
                 act->transform.SetPosition(pos);
             }
        }
	}
}

/// シーンがアクティブになった時の初期化処理
void app::test::IngameScene::OnActivated()
{
    isPlaying = false;
    state = State::Idle;
    countdownTimer = 0.0f;

    // ゲームセッションをリセットしグローバルに登録
    gameSession.Reset();
    SetCurrentSession(&gameSession);

    if (selectedSong.musicPath.empty() || bgmPlayer.isNull()) {
        sf::debug::Debug::Log("[BGM] OnActivated: path empty or bgmPlayer null");
        return;
    }

    // BGMパスを設定（Shift-JIS変換）
    std::string sjisPath = Utf8ToShiftJis(selectedSong.musicPath);
    bgmPlayer->SetPath(sjisPath);

    // ゲームセッションに楽曲情報を設定
    gameSession.SetChartPath(selectedSong.chartPath);
    gameSession.SetDifficulty(selectedSong.difficulty);
    gameSession.SetTitle(selectedSong.title);
}

/// ゲーム開始処理 - NoteManagerを起動しノート総数をセッションに設定する
void app::test::IngameScene::StartGame()
{
    // 二重起動防止
    if (isPlaying) return;

    sf::debug::Debug::Log("Game Start");
    isPlaying = true;

    // NoteManagerを開始
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            int total = noteMgr->GetMaxTotalCombo();
            gameSession.SetTotalNoteCount(total);
            noteMgr->StartGame();
        }
    }

    // BGM再生はUpdate内で楽曲時間が0になるのを待ってから開始
}

/// 描画処理 - 基底描画の後にインスタンス描画を実行する
void app::test::IngameScene::Draw()
{
    // 基底描画（通常のアクター・カメラ設定など）
    sf::Scene<IngameScene>::Draw();

    // インスタンス描画によるノート描画
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            noteMgr->DrawInstanced();
        }
    }
}

/// オーバーレイ描画 - Live2Dモデルを最前面に描画する
void app::test::IngameScene::DrawOverlay()
{
	if (l2dComp.Get()) {
		l2dComp->Draw();
	}
}

/// スキルノート発動時のエフェクト処理（グリッチ + SE再生）
void app::test::IngameScene::TriggerSkillEffect()
{
    // グリッチエフェクト開始（強度はUpdateで減衰制御）
    skillEffectTimer = 0.2f;
    
    // スキルSEを再生
    if (!skillSePlayer.isNull()) {
        skillSePlayer->Stop();
        skillSePlayer->Play(0.0f);
    }
}

/// ImGUI処理（現在はキーボードショートカットで代替）
void app::test::IngameScene::OnGUI()
{
}
