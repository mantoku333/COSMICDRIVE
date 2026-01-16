#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include "Config.h" // Added include
#include <fstream>
#include <sstream>
#include <algorithm>
#include "IngameScene.h"
#include "NoteData.h"
#include "SoundComponent.h"
#include "SongInfo.h"
#include <numeric>
#include <cmath>
#include <map>
#include "JudgeStatsService.h"
#include "ChedParser.h"
#include "ResultScene.h"
#include "IngameCanvas.h" 
#include "LoadingScene.h"



#include <filesystem> 

#include <windows.h> // Add Windows header for encoding conversion

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

namespace app::test {

	// ノーツタイプごとの効果音パス
	namespace {
		const char* GetHitSfxPath(NoteType t) {
			switch (t) {
			case NoteType::Tap: return "Assets\\sound\\tap.wav";
			default:            return "Assets\\sound\\tap.wav";
			}
		}
	}

	// -----------------------------
	// 定数定義
	// -----------------------------
	static constexpr int    K_BEATS_PER_MEASURE = 4;
	static constexpr double K_DEFAULT_BPM = 120.0;
	static constexpr double K_OFFSET_SEC = 0.15;
	static constexpr float  INPUT_OFFSET_SEC = 0.0f;

	// 判定ウィンドウ（秒）
	static constexpr float J_WIN_PERFECT = 0.050f;
	static constexpr float J_WIN_GREAT = J_WIN_PERFECT + 0.030f;
	static constexpr float J_WIN_GOOD = J_WIN_GREAT + 0.030f;

	// 判定挙動補助
	static constexpr float J_POS_TOL_MULT = 2.0f;

	// -----------------------------
	// BPM変化対応用の構造体
	// -----------------------------
	struct TempoEvent {
		double atBeat = 0.0;
		double bpm = K_DEFAULT_BPM;
		double spb = 60.0 / K_DEFAULT_BPM;
		double atSec = 0.0;
	};

	static void BuildTempoMap(std::vector<TempoEvent>& tm) {
		if (tm.empty()) {
			tm.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
			return;
		}

		std::sort(tm.begin(), tm.end(), [](const TempoEvent& a, const TempoEvent& b) {
			return a.atBeat < b.atBeat;
			});

		tm[0].spb = 60.0 / tm[0].bpm;
		tm[0].atSec = 0.0;

		for (size_t i = 1; i < tm.size(); ++i) {
			tm[i].spb = 60.0 / tm[i].bpm;
			const double beatsSpan = tm[i].atBeat - tm[i - 1].atBeat;
			tm[i].atSec = tm[i - 1].atSec + beatsSpan * tm[i - 1].spb;
		}
	}

	static double BeatToSeconds(double absBeat, const std::vector<TempoEvent>& tm) {
		size_t lo = 0, hi = tm.size();
		while (lo + 1 < hi) {
			size_t mid = (lo + hi) / 2;
			if (tm[mid].atBeat <= absBeat) lo = mid; else hi = mid;
		}
		const TempoEvent& seg = tm[lo];
		const double dBeat = absBeat - seg.atBeat;
		return seg.atSec + dBeat * seg.spb;
	}

	// ==================================================
	// 初期化：譜面読み込み・ノーツ生成
	// ==================================================
	void NoteManager::Begin() {
        // UI上の 7.0 が 内部値 30.0 になるように変換
        float factor = 30.0f / 7.0f;
        HiSpeed = gGameConfig.hiSpeed * factor;
        noteSpeed = basenoteSpeed * HiSpeed;

		while (ShowCursor(FALSE) >= 0);

		if (resultScene.isNull()) {
			resultScene = ResultScene::StandbyScene();
		}


		if (auto actor = actorRef.Target()) {
			auto* changer = actor->GetComponent<SceneChangeComponent>();
			if (changer) {
				sceneChanger = changer;
			}
		}

		isPlaying = false;
		JudgeStatsService::Reset();

		auto owner = actorRef.Target(); if (!owner) return;
		auto* scene = &owner->GetScene();

		std::string chartPath = "Save/chart/Sample.chart";
		if (auto* ingameScene = dynamic_cast<IngameScene*>(scene)) {
			const SongInfo& selectedSong = ingameScene->GetSelectedSong();
			if (!selectedSong.chartPath.empty())
				chartPath = selectedSong.chartPath;
		}

		noteSequence.clear();
		std::vector<TempoEvent> tempoMap;
		bool sawAnyTempoMeta = false;
		noteOffset = 0.0;

		// ----------------------------------
		// 譜面ファイル読み込み
		// ----------------------------------
		{
			// 譜面をパース
			ChedParser parser;
			// Convert UTF-8 path to Wstring for correct file opening on Windows
			std::wstring wChartPath = Utf8ToWstring(chartPath);
			if (parser.Load(wChartPath)) {
				sf::debug::Debug::Log("Chart Loaded: " + chartPath);
			} else {
				sf::debug::Debug::Log("Chart Load Failed: " + chartPath);
			}

			noteSequence.clear();
			tempoMap.clear();
			sawAnyTempoMeta = false;
			noteOffset = 0.0;

			for (const auto& te : parser.tempos) {
				TempoEvent e;
				e.atBeat = te.absBeat;
				e.bpm = te.bpm;
				e.spb = 60.0 / e.bpm;
				e.atSec = 0.0;
				tempoMap.push_back(e);
			}
			sawAnyTempoMeta = !tempoMap.empty();

			std::map<int, size_t> pendingHolds; // Key: (lane << 8) | holdId

			for (const auto& cn : parser.notes) {
				NoteData nd;
				nd.lane = std::clamp(cn.lane, 0, 5);
				nd.mbt.measure = cn.measure;

				double beatInMeasure = cn.beat;
				nd.mbt.beat = static_cast<int>(std::floor(beatInMeasure));
				nd.mbt.tick16 = static_cast<int>(std::round((beatInMeasure - nd.mbt.beat) * 16.0));
				nd.mbt.beat = std::clamp(nd.mbt.beat, 0, K_BEATS_PER_MEASURE - 1);
				nd.mbt.tick16 = std::clamp(nd.mbt.tick16, 0, 15);

				nd.type = cn.type;
				nd.absBeat = cn.absBeat;
				nd.judged = false;
				nd.result = JudgeResult::None;
                
                // Defaults
                nd.pairIndex = -1;
                nd.duration = 0.0f;

                // Make sure to use cn.isHold from ChedParser
                if (cn.isHold) {
                    int key = (cn.lane << 8) | cn.holdId;
                    if (pendingHolds.count(key)) {
                        // This is the End of a Hold
                        size_t startIdx = pendingHolds[key];
                        
                        // Link them
                        noteSequence[startIdx].type = NoteType::HoldStart;
                        noteSequence[startIdx].pairIndex = (int)noteSequence.size(); // Current Index
                        
                        nd.type = NoteType::HoldEnd;
                        nd.pairIndex = (int)startIdx;
                        
                        pendingHolds.erase(key);
                    } else {
                        // This is the Start of a Hold
                        nd.type = NoteType::HoldStart; // Assumed start
                        pendingHolds[key] = noteSequence.size();
                    }
                }

				noteSequence.push_back(nd);
			}

			sf::debug::Debug::Log("===== AFTER lane mapping =====");
			for (const auto& nd : noteSequence) {
				sf::debug::Debug::Log("mapped lane=" + std::to_string(nd.lane)
					+ " absBeat=" + std::to_string(nd.absBeat));
			}
		}

		if (!sawAnyTempoMeta)
			tempoMap.push_back({ 0.0, K_DEFAULT_BPM, 60.0 / K_DEFAULT_BPM, 0.0 });
		BuildTempoMap(tempoMap);

		for (auto& nd : noteSequence)
			nd.hittime = static_cast<float>(BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC + noteOffset + gGameConfig.offsetSec);

        // Calculate Duration for Holds
        for (auto& nd : noteSequence) {
            if (nd.type == NoteType::HoldStart && nd.pairIndex != -1) {
                if (nd.pairIndex < (int)noteSequence.size()) {
                    nd.duration = noteSequence[nd.pairIndex].hittime - nd.hittime;
                    
                    // DEBUG LOGGING
                    sf::debug::Debug::Log("HoldStart: Lane=" + std::to_string(nd.lane) 
                        + " Time=" + std::to_string(nd.hittime) 
                        + " PairIdx=" + std::to_string(nd.pairIndex)
                        + " Duration=" + std::to_string(nd.duration));
                }
            } else if (nd.type == NoteType::HoldStart) {
                 sf::debug::Debug::LogError("HoldStart UNPAIRED or INVALID: Lane=" + std::to_string(nd.lane) + " Time=" + std::to_string(nd.hittime));
            }
        }

		// ----------------------------------
		// レーンパラメータ
		// ----------------------------------
		float slopeRad = rotX * 3.14159265f / 180.0f;
		float startZ = laneH * 0.5f;
		float hitZ = -laneH * 0.5f + laneH * barRatio;
		float startY = baseY - std::tan(slopeRad) * startZ;
		float hitY = baseY - std::tan(slopeRad) * hitZ; 

		int lanes = std::max(1, (int)laneRefs.size());
		float totalDistance = std::abs(startZ - hitZ);

		leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
		songTime = -leadTime;

		// ----------------------------------
		// ノーツ生成
		// ----------------------------------
		noteActors.clear();
		nextIndex = 0;
		currentCombo = 0;

		for (size_t i = 0; i < noteSequence.size(); ++i) {
			const auto& nd = noteSequence[i];

			if (nd.type == NoteType::SongEnd) {
				noteActors.push_back({});
				continue;
			}

			float laneX = 0.0f;
			float laneYOffset = 0.0f; 

			if (nd.lane == 4) {
				laneX = sideLaneX_Left;
			}
			else if (nd.lane == 5) {
				laneX = sideLaneX_Right;
			}
			else {
				laneX = (nd.lane - 4 * 0.5f + 0.5f) * laneW;
			}

			auto noteActor = scene->Instantiate();
			auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
			mesh->SetGeometry(g_cube);
			mesh->material.SetColor({ 1, 1, 1, 1 });
			noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
			noteActor.Target()->transform.SetPosition({ laneX, startY + laneYOffset, startZ });
			noteActor.Target()->transform.SetRotation({ rotX, 0, 0 });

			auto comp = noteActor.Target()->AddComponent<NoteComponent>();
			comp->info = nd;
			comp->leadTime = leadTime;
			comp->noteSpeed = noteSpeed;

			noteActor.Target()->DeActivate();
			noteActors.push_back(noteActor);
		}

		laneOrder.assign(lanes, {});
		laneOrder.assign(lanes, {});
		laneHeads.assign(lanes, 0);
        activeHolds.assign(lanes, -1);
		for (int i = 0; i < (int)noteSequence.size(); ++i) {
			int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
			laneOrder[l].push_back(i);
		}

		updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));
	}

	// ==================================================
	// Update
	// ==================================================
	void NoteManager::Update(const sf::command::ICommand&) {

		if (!isPlaying) return;

		songTime += sf::Time::DeltaTime();

		while (nextIndex < noteSequence.size()
			&& songTime + leadTime >= noteSequence[nextIndex].hittime)
		{
			if (noteSequence[nextIndex].type == NoteType::SongEnd) {
				++nextIndex;
				continue;
			}

			auto* act = noteActors[nextIndex].Target();
			if (act) act->Activate();
			++nextIndex;
		}

		CheckMissedNotes();
	}

	// ==================================================
	// 判定ロジック
	// ==================================================
	JudgeResult NoteManager::JudgeNote(NoteType, float noteTime, float inputTime) {
		inputTime += INPUT_OFFSET_SEC;
		float diff = std::abs(noteTime - inputTime);
		if (diff <= J_WIN_PERFECT) return JudgeResult::Perfect;
		if (diff <= J_WIN_GREAT)   return JudgeResult::Great;
		if (diff <= J_WIN_GOOD)    return JudgeResult::Good;
		return JudgeResult::Miss;
	}

	// ==================================================
	// レーン判定
	// ==================================================
	JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
		if (lane < 0 || lane >= (int)laneOrder.size()) {
             sf::debug::Debug::Log("JudgeLane: Skip (Invalid Lane) lane=" + std::to_string(lane)); 
             return JudgeResult::Skip;
        }

		auto& order = laneOrder[lane];
		size_t& head = laneHeads[lane];
		while (head < order.size() && noteSequence[order[head]].judged) ++head;
		if (head >= order.size()) {
             sf::debug::Debug::Log("JudgeLane: Skip (No Notes Left) lane=" + std::to_string(lane));
             return JudgeResult::Skip;
        }

		int idx = order[head];
		if (idx >= (int)nextIndex) {
            sf::debug::Debug::Log("JudgeLane: Skip (Index Future) lane=" + std::to_string(lane) + " idx=" + std::to_string(idx) + " next=" + std::to_string(nextIndex)); 
            return JudgeResult::Skip;
        }

		auto* act = noteActors[idx].Target();
		if (!act) return JudgeResult::Skip;

		float noteZ = act->transform.GetPosition().z;
        
        // HOLD NOTES: Act Position is Center. We need Head (Start) Position for Judgment.
        // Head is closer to Judge Line (-Z direction). Center is +Z direction from Head.
        // HeadZ = CenterZ - (LengthProjected / 2)
        if (noteSequence[idx].type == NoteType::HoldStart) {
             float slopeRad = rotX * 3.14159265f / 180.0f;
             // Scale.z is the Visual Length (with 1/cos correction).
             // Projected Length on Z axis = Scale.z * cos(slope).
             // So halfLenZ = (Scale.z * cos) * 0.5f.
             float halfLenZ = act->transform.GetScale().z * std::cos(slopeRad) * 0.5f;
             noteZ -= halfLenZ;
        }

		float diffZ = std::abs(noteZ - judgeZ);
		if (diffZ > judgeRange * J_POS_TOL_MULT) {
             // 距離が遠すぎる
             sf::debug::Debug::Log("JudgeLane: Skip (Too Far) lane=" + std::to_string(lane) + " diffZ=" + std::to_string(diffZ));
             return JudgeResult::Skip;
        }

		JudgeResult res = JudgeNote(noteSequence[idx].type, noteSequence[idx].hittime, inputTime);
		noteSequence[idx].judged = true;
		noteSequence[idx].result = res;

		JudgeStatsService::AddResult(res);
        if (res != JudgeResult::Skip && res != JudgeResult::Miss) {
             sf::debug::Debug::Log("JudgeLane: Judgment Result = " + judgeResultToString(res) 
                 + " Lane=" + std::to_string(lane) 
                 + " Time=" + std::to_string(inputTime) 
                 + " NoteTime=" + std::to_string(noteSequence[idx].hittime)
                 + " Type=" + std::to_string((int)noteSequence[idx].type)
                 + " PairIdx=" + std::to_string(noteSequence[idx].pairIndex));
        }

        // Register Active Hold
        if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
            if (noteSequence[idx].type == NoteType::HoldStart && noteSequence[idx].pairIndex != -1) {
                activeHolds[lane] = noteSequence[idx].pairIndex;
                sf::debug::Debug::Log("JudgeLane: Hold Registered! Lane=" + std::to_string(lane) + " PairIdx=" + std::to_string(noteSequence[idx].pairIndex));
            }
            else if (noteSequence[idx].type == NoteType::HoldStart) {
                 sf::debug::Debug::Log("JudgeLane: HoldStart BUT PairIndex is -1! ID=" + std::to_string(idx));
            }
        }

        // FAST/SLOW Logic
        if (res == JudgeResult::Perfect || res == JudgeResult::Great || res == JudgeResult::Good) {
            float diff = (inputTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime;
            // 0.005f (5ms) margin for "JAUST" (display nothing) if needed, 
            // but usually we show Fast/Slow for all non-perfects, or even perfects.
            // Let's implement: If Perfect w/ very small diff -> Hide ? User said "Fast/Slow display".
            // Let's show it for everything if it's not exactly 0 (which is rare).
            // But maybe hide if diff is very small to avoid flickering on 'Just'.
            
            bool isFast = diff < 0;
            bool isSlow = diff > 0; // or >= 0

            // Update Stats
            if (isFast) JudgeStatsService::AddFast();
            else JudgeStatsService::AddSlow(); // 0 is treated as Slow (rare)

            // Update UI
            if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                if (gGameConfig.enableFastSlow) {
                    if (res == JudgeResult::Perfect && std::abs(diff) < 0.016f) { 
                        // If logic: Perfect and within 1 frame (16ms) -> Maybe don't show Fast/Slow?
                        // Or just show it. Let's show it.
                        // Actually, let's treat Perfect as "Just" (No Fast/Slow) IF the user wants strict mode?
                        // For now, simple: Check enableFastSlow. If on, show.
                        canvas->ShowFastSlow(isFast ? 1 : 2);
                    } else {
                         canvas->ShowFastSlow(isFast ? 1 : 2);
                    }
                } else {
                    canvas->ShowFastSlow(0);
                }
            }
        }
        else {
             // Miss
             if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                 canvas->ShowFastSlow(0);
             }
        }

		// エフェクト生成 座標・色・サイズ
		if (res == JudgeResult::Perfect || res == JudgeResult::Great || res == JudgeResult::Good) {

			if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {

				// 座標計算パラメータ
				float hitY = -130.0f; // UI上の判定ライン高さ
				float uiLaneWidth = 300.0f;
				float sideOffset = 250.0f;
				float hitX = 0.0f;

				// X座標決定
				if (lane <= 3) {
					hitX = (lane - 1.5f) * uiLaneWidth;
				}
				else if (lane == 4) {
					hitX = (-1.5f * uiLaneWidth) - sideOffset;
				}
				else if (lane == 5) {
					hitX = (1.5f * uiLaneWidth) + sideOffset;
				}

				// 判定ごとの演出パラメータ
				float scale = (uiLaneWidth / 100.0f) * 1.5f; // 基本サイズ
				float duration = 0.4f;
				Color color = { 1.0f, 1.0f, 1.0f, 1.0f };


				// Canvas(EffectManager)へ生成依頼
				canvas->SpawnHitEffect(hitX, hitY, scale, duration, color);
			}
		}

        // Destroy actor unless it's a HoldStart (Keep alive for debug even on Miss)
        bool shouldDestroy = true;
        if (noteSequence[idx].type == NoteType::HoldStart) {
             shouldDestroy = false;
        }

        if (shouldDestroy) {
		    act->DeActivate();
		    act->Destroy();
        }

		++head;
		return res;
	}

	void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
		for (int l : pressedLanes) {
			JudgeLane(l, inputTime);
		}
	}

	void NoteManager::CheckMissedNotes() {
		for (int l = 0; l < (int)laneOrder.size(); ++l) {
			auto& order = laneOrder[l];
			size_t& head = laneHeads[l];

			while (head < order.size()) {
				int i = order[head];
				if (noteSequence[i].judged) { ++head; continue; }
				if (i >= (int)nextIndex) break;

				if (songTime > noteSequence[i].hittime + J_WIN_GOOD) {
					if (noteSequence[i].type == NoteType::SongEnd) {
						if (!sceneChanger.isNull()) {
							// sf::debug::Debug::Log("Result Scene Transition");

							// 描画・システムのクリーンアップ
							sf::Mesh::ClearAllRegistered();
							ShowCursor(TRUE);

							LoadingScene::SetLoadingType(LoadingType::Common);
							sceneChanger->ChangeScene(resultScene);
						}
						noteSequence[i].judged = true;
						return;
					}

					noteSequence[i].judged = true;
					noteSequence[i].result = JudgeResult::Miss;
					JudgeStatsService::AddResult(JudgeResult::Miss);

					if (auto* act = noteActors[i].Target()) {
                        // Destroy check -> Keep HoldStart alive for debug
                        if (noteSequence[i].type != NoteType::HoldStart) {
						    act->DeActivate();
						    act->Destroy();
                        }
					}
					++head;
				}
				else {
					break;
				}
			}
		}
	}

	void NoteManager::UpdateCombo(JudgeResult result) {
		switch (result) {
		case JudgeResult::Perfect:
		case JudgeResult::Great:
		case JudgeResult::Good:
			++currentCombo; break;
		case JudgeResult::Miss:
			currentCombo = 0; break;
		default: break;
		}
	}

	int NoteManager::GetCurrentCombo() const { return currentCombo; }

	void NoteManager::SetLaneParams(
		const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
		float laneW_, float laneH_, float rotX_,
		float baseY_, float barRatio_,
		float sideLeftX_, float sideRightX_)
	{
		laneRefs = lanes;
		laneW = laneW_;
		laneH = laneH_;
		rotX = rotX_;
		baseY = baseY_;
		barRatio = barRatio_;
		sideLaneX_Left = sideLeftX_;
		sideLaneX_Right = sideRightX_;
		judgeZ = -laneH * 0.5f + laneH * barRatio;
	}

	void NoteManager::StartGame() {
		isPlaying = true;
		// songTime = 0.0f; // ← 削除: Beginで設定されたleadTime（マイナス値）を維持する
	}

	void NoteManager::SetCurrentSongTime(float time) {
		this->songTime = time;
	}

	void NoteManager::SyncTime(float time) {
		// 時間差（ドリフト）を計算
		float diff = time - this->songTime;

		// ズレが大きすぎる場合（例えば 50ms以上）、
        // または負の方向に大きすぎる（時間が飛び過ぎて戻った）場合は強制的に合わせる。
        // ※少しくらいの負のズレ（Gameが進みすぎ）は許容してLerpで合わせる（カクつき防止）
		if (std::abs(diff) > 0.05f) {
			this->songTime = time;
		}
		else {
            if (time <= 0.001f) {
                // 音声がまだ始まっていない(0)の場合は、同期せずにゲーム時間を進める(DeltaTime任せ)
                // これによりStart直後の「溜め」でノーツが止まるのを防ぐ
            }
            else {
			    // ズレが小さい場合は、補間して滑らかに近づける
			    this->songTime += diff * 0.2f;
            }
		}
	}

    	// ==================================================
	// Check Hold (Continuous Input)
	// ==================================================
	void NoteManager::CheckHold(int lane, bool isPressed) {
		if (lane < 0 || lane >= (int)activeHolds.size()) return;
		int idx = activeHolds[lane];
		if (idx == -1) {
            // ホールド中でないのにCheckHoldが呼ばれるのは普通（押しっぱなし判定など）なのでログは出さない
            return; 
        }

		auto& endNote = noteSequence[idx];
		if (endNote.judged) {
			activeHolds[lane] = -1; // Already done
			return;
		}

        // デバッグ出力：ホールド判定中
        // 毎フレーム呼ぶとスパムになるので、特定のタイミングか、情報を間引いて出す
        // ここでは一旦そのまま出すが、あまりに多いなら抑制する
        // sf::debug::Debug::Log("CheckHold: Lane=" + std::to_string(lane) + " Pressed=" + std::to_string(isPressed) + " Time=" + std::to_string(songTime) + " EndTime=" + std::to_string(endNote.hittime));

		if (isPressed) {
			// Check if we reached End
			// If we've held it until the end hit time (or very close), it's a success
            float timeDiff = songTime - (endNote.hittime - J_WIN_PERFECT); // For debug
			if (songTime >= endNote.hittime - J_WIN_PERFECT) { // Slightly lenient to avoid 1-frame miss
				sf::debug::Debug::Log("CheckHold: Hold Complete! Lane=" + std::to_string(lane));
                
				JudgeResult res = JudgeResult::Perfect;
				endNote.judged = true;
				endNote.result = res;
				JudgeStatsService::AddResult(res);
				UpdateCombo(res);

				// Effect
				if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
					sound->Play(GetHitSfxPath(endNote.type));
				}

				// Visual Effect
				if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
					float uiLaneWidth = 300.0f;
					float sideOffset = 250.0f;
					float hitX = 0.0f;
					if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
					else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
					else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;

					canvas->SpawnHitEffect(hitX, -130.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
				}

				// Destroy HoldStart Actor
				int startIdx = endNote.pairIndex;
				if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
					if (auto* act = noteActors[startIdx].Target()) {
						act->DeActivate();
						act->Destroy();
					}
				}

				activeHolds[lane] = -1;
			}
            else {
                 // Holding... logic
            }
		}
		else {
			// Released
			 // If released slightly early (within Perfect/Great window), allow it?
			 // Or if strictly not reached -> Miss.
			float diff = endNote.hittime - songTime;

            sf::debug::Debug::Log("CheckHold: Released! Diff=" + std::to_string(diff));

			if (diff <= J_WIN_GOOD) { // Allow release if within 'Good' window
				// Judge based on diff
				JudgeResult res = JudgeResult::Miss;
				if (diff <= J_WIN_PERFECT) res = JudgeResult::Perfect;
				else if (diff <= J_WIN_GREAT) res = JudgeResult::Great;
				else res = JudgeResult::Good;

                sf::debug::Debug::Log("CheckHold: Release Success! Result=" + judgeResultToString(res));

				endNote.judged = true;
				endNote.result = res;
				JudgeStatsService::AddResult(res);
				UpdateCombo(res);

				// Effect if not miss
				if (res != JudgeResult::Miss) {
					if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
						sound->Play(GetHitSfxPath(endNote.type));
					}
				}

				// Destroy HoldStart Actor
				int startIdx = endNote.pairIndex;
				if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
					if (auto* act = noteActors[startIdx].Target()) {
						act->DeActivate();
						act->Destroy();
					}
				}

				activeHolds[lane] = -1;
			}
			else {
				// Dropped too early -> Miss
                sf::debug::Debug::Log("CheckHold: Dropped Early! Lane=" + std::to_string(lane));

				endNote.judged = true;
				endNote.result = JudgeResult::Miss;
				JudgeStatsService::AddResult(JudgeResult::Miss);
				UpdateCombo(JudgeResult::Miss);

				activeHolds[lane] = -1;
			}
		}
	}

} // namespace app::test
