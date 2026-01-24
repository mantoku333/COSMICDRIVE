#include "IngameScene.h"
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
#include "JudgeStatsService.h"
#include "JudgeStatsService.h"
#include "GUI.h"
#include "SInput.h"

#include "EffectManager.h"
#include <Effekseer.h>
#include <Windows.h>

namespace {
    static std::wstring Utf8ToWstring(const std::string& str) {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
        return wstr;
    }

    static std::string Utf8ToShiftJis(const std::string& utf8Str) {
        std::wstring wstr = Utf8ToWstring(utf8Str);
        if (wstr.empty()) return "";
        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }
}


void app::test::IngameScene::Init()
{
	// sf::debug::Debug::Log("IngameScene initialized");
	ShowCursor(FALSE);

	// ── カメラ生成 ──
	{
		auto camera = Instantiate();
		camera.Target()->AddComponent<sf::Camera>();
		// 固定カメラ (ユーザーが「おかしい」と言ったのでFollowCameraは一旦外す)
		// 位置は lanes 全体が見渡せる位置に設定
		camera.Target()->transform.SetPosition({ 0.0f, 20.0f, -20.0f });
		camera.Target()->transform.SetRotation({ 45.0f, 0.0f, 0.0f });
	}

	// ── マネージャを生成 ──
	{
		managerActor = Instantiate();
		managerActor.Target()->AddComponent<app::test::NoteManager>();
		managerActor.Target()->AddComponent<IngameCanvas>();
		managerActor.Target()->AddComponent<app::test::SoundComponent>();
		managerActor.Target()->AddComponent<SceneChangeComponent>();
	   
		bgmPlayer = managerActor.Target()->AddComponent<app::test::BGMComponent>();

		auto effectMgr = managerActor.Target()->AddComponent<app::test::EffectManager>();

		// プレイヤー
		auto player = Instantiate();
		auto mesh = player.Target()->AddComponent<sf::Mesh>();
		mesh->SetGeometry(g_cube); // Cubeメッシュ
		player.Target()->transform.SetScale({ 0.5f, 0.5f, 0.5f });
		mesh->material.SetColor({ 0.3f, 0.3f, 1.0f, 0.1f });
		
		auto pComp = player.Target()->AddComponent<PlayerComponent>(); // ←ここに移動処理含む
		
	}

    // ===== レーン配置 =====
    {
        lanePanels.clear();

        // ── レーン生成 ──
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

        // 特殊ギミック用サイドレーン（左4 右5）
        {
            // メインレーン群の端の座標を計算
            // lanes * laneW が全体の幅。その半分が中心からの距離。
            float mainHalfWidth = (lanes * laneW) * 0.5f;

            float sideLaneW = laneW;

            // 左右のX座標：メインの端 + サイドレーンの半径
            float leftSideX = -mainHalfWidth - (sideLaneW * 0.5f);
            float rightSideX = mainHalfWidth + (sideLaneW * 0.5f);

            // 左右の座標を配列に入れてループ処理で生成
            float sidePositions[] = { leftSideX, rightSideX };

            for (float posX : sidePositions)
            {
                auto sideLane = Instantiate();
                auto mesh = sideLane.Target()->AddComponent<sf::Mesh>();
                mesh->SetGeometry(g_cube);

                // メインレーンと同じ傾き・高さ設定
                sideLane.Target()->transform.SetScale({ sideLaneW, 0.1f, laneH });
                sideLane.Target()->transform.SetPosition({ posX, baseY + 1, 0.0f });
                sideLane.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mesh->material.SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });

                lanePanels.push_back(sideLane);
            }
        }

        // ── 区切り線 ──
        {
            const float lineThickness = 0.05f;
            const float lineHeight = 0.05f;
            const float slopeRad = rotX * 3.14159265f / 180.0f;

            for (int i = 1; i < lanes; ++i)
            {
                float localX = (i - lanes * 0.5f) * laneW;

                // レーン中央を基準にY/Z補正
                float offsetZ = 0.0f; // Zはレーン中央に固定
                float offsetY = std::tan(slopeRad) * offsetZ; // ← tan(rotX) 符号修正

                auto line = Instantiate();
                auto mLine = line.Target()->AddComponent<sf::Mesh>();
                mLine->SetGeometry(g_cube);

                line.Target()->transform.SetScale({ lineThickness, lineHeight, laneH });
                line.Target()->transform.SetPosition({ localX, baseY + offsetY + 0.05f, offsetZ });
                line.Target()->transform.SetRotation({ rotX, 0.0f, 0.0f });

                mLine->material.SetColor({ 1, 1, 1, 1 });
            }
        }

        // ── レーン外枠（左右エッジ）──
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

       



        // ── ジャッジバー（手前ライン）──
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

		// ── Live2D ──
		{
			auto live2dActor = Instantiate();
			l2dComp = live2dActor.Target()->AddComponent<Live2DComponent>();
			if (l2dComp.Get()) {
				// TitleSceneと同じモデルをロード
				    // Relative path to the model directory
				const std::string MODEL_DIR = "Assets/Live2D/CyberCat";
				    // Model .model3.json filename
				const std::string MODEL_FILE = "CyberCat.model3.json";

				l2dComp->LoadModel(MODEL_DIR, MODEL_FILE);

				// 位置調整 (サイズを0.7倍、少し上へ)
				live2dActor.Target()->transform.SetPosition({ 0.0f, 0.80f, 0.0f }); 
				// スケール調整
				live2dActor.Target()->transform.SetScale({ 0.49f, 0.7f, 1.0f });

				// Start Idle animation
				l2dComp->PlayMotion("Idle", 0, 3);
				
                // タイトル同様グリッチもかけてみる（お好みで）
                l2dComp->StartGlitchMotion("GlitchNoise", 0);
			}
		}
        
    }



    if (auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>())
    {
        // サイドレーンの座標計算
        // (lanePanelsの生成時に使った計算と同じもの)
        float mainHalfWidth = (4 * laneW) * 0.5f; // メイン4レーン分
        float sideLaneW = laneW;

        float leftX = -mainHalfWidth - (sideLaneW * 0.5f);
        float rightX = mainHalfWidth + (sideLaneW * 0.5f);

        noteMgr->SetLaneParams(lanePanels, laneW, laneH, rotX, baseY, barRatio, leftX, rightX);
    }

    updateCommand.Bind(std::bind(&IngameScene::Update, this, std::placeholders::_1));
}




void app::test::IngameScene::Update(const sf::command::ICommand& command)
{
	if (l2dComp.Get()) {
		l2dComp->Update();
	}

	// ステートマシンによる制御
	if (state == State::Idle) {
		// すぐさまカウントダウンへ移行（必要ならここで少し待機も可）
		state = State::Countdown;
		countdownTimer = 3.0f; // 3秒カウントダウン
	}
	else if (state == State::Countdown) {
		countdownTimer -= sf::Time::DeltaTime();

		// Canvasに通知
		if (managerActor.Target()) {
			if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
				canvas->UpdateCountdownDisplay(countdownTimer, false);
			}
		}

		if (countdownTimer <= 0.0f) {
			// カウントダウン終了 → START表示 → ゲーム開始
			state = State::Playing;
			
			// CanvasにSTART表示指示
			if (managerActor.Target()) {
				if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
					canvas->UpdateCountdownDisplay(0.0f, true);
				}
			}

			startDisplayTimer = 1.0f; // タイマー初期化

			StartGame();
		}
	}
	else if (state == State::Playing) {
		// "START" 表示を一定時間で消す処理が必要ならCanvas側でタイマーを持つか、
		// ここで少し待ってから非表示にする指示を送る。
		// 今回は簡易的に、少し時間が経ったら消すようにCanvasへ再送してもいいが、
		// UpdateCountdownDisplayのロジック的に 0以下なら非表示になるので、
		// START表示を残したければ別途タイマーが必要。
		// ここではシンプルに、Playingに入ったら少しの間(例えば1秒)STARTを出し続ける処理を入れるか、
		// あるいはCanvas側がStart表示状態を持続させるか。
		
		if (startDisplayTimer > 0.0f) {
			startDisplayTimer -= sf::Time::DeltaTime();
			if (startDisplayTimer <= 0.0f) {
				// 表示消去
				if (managerActor.Target()) {
					if (auto canvas = managerActor.Target()->GetComponent<IngameCanvas>()) {
						canvas->UpdateCountdownDisplay(-1.0f, false);
					}
				}
			}
		}


		// ゲーム中
		if (managerActor.Target())
		{
			auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
			if (noteMgr) {
				// まだ曲が再生されていない場合、時間が0になるのを待つ
				if (!bgmPlayer.isNull() && !bgmPlayer->IsPlaying()) {
					float currentSongTime = noteMgr->GetSongTime();
					
					// 時間が進んで0（＝判定ライン到達）を超えたら再生開始
					if (currentSongTime >= 0.0f) {
						bgmPlayer->Play();
						// 念のため同期
						noteMgr->SyncTime(bgmPlayer->GetCurrentTime()); 
					}
				}
				else if (!bgmPlayer.isNull() && bgmPlayer->IsPlaying()) {
					// 再生中は常に同期
					noteMgr->SyncTime(bgmPlayer->GetCurrentTime());
				}
			}
		}

		// 背景エフェクト等の更新（Playing中のみ）
		static float time = 0.0f;
		time += sf::Time::DeltaTime();

		// DEBUG SKIP: Key 'P'
		if (SInput::Instance().GetKeyDown(Key::KEY_P)) {
			if (managerActor.Target()) {
				if (auto* noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>()) {
					noteMgr->DebugForceComplete();
				}
			}
		}

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
	}
}

void app::test::IngameScene::OnActivated()
{
    isPlaying = false;
    state = State::Idle;
    countdownTimer = 0.0f;

    if (selectedSong.musicPath.empty() || bgmPlayer.isNull()) {
        sf::debug::Debug::Log("[BGM] OnActivated: path empty or bgmPlayer null");
        return;
    }

    // Convert UTF-8 path to Shift-JIS for Windows API
    std::string sjisPath = Utf8ToShiftJis(selectedSong.musicPath);
    bgmPlayer->SetPath(sjisPath);

    // Set chart path and difficulty for score saving
    JudgeStatsService::SetChartPath(selectedSong.chartPath);
    JudgeStatsService::SetDifficulty(selectedSong.difficulty);

    //bgmPlayer->Play();
}

void app::test::IngameScene::StartGame()
{
    // 二重起動防止
    if (isPlaying) return;

    sf::debug::Debug::Log("Game Start");

    isPlaying = true;

    // 1. NoteManager を動かす
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            noteMgr->StartGame();
        }
    }

    // BGM再生開始はここで行わず、Update内で時間が0になるのを待ってから行う
    /*
    if (!bgmPlayer.isNull()) {
        bgmPlayer->Play();
    }
    */

}

void app::test::IngameScene::Draw()
{
    // Call Base Draw first (standard actors, camera setup, etc.)
    sf::Scene<IngameScene>::Draw();

    // Draw Instanced Notes
    if (managerActor.Target()) {
        auto noteMgr = managerActor.Target()->GetComponent<app::test::NoteManager>();
        if (noteMgr) {
            noteMgr->DrawInstanced();
        }
    }
}

void app::test::IngameScene::DrawOverlay()
{
	if (l2dComp.Get()) {
		l2dComp->Draw();
	}
}

void app::test::IngameScene::OnGUI()
{
	// ImGui causes crashes in some environments, so we use Keyboard Shortcut (P) instead.
}
