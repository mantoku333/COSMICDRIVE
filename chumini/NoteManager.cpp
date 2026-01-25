#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include "BGMComponent.h" // Added for DebugForceComplete
#include "Config.h" // Added include
#include "DirectX11.h" // Added for Instancing
#include <d3dcompiler.h> // Required for D3DCompileFromFile
#pragma comment(lib, "d3dcompiler.lib") // Ensure linking
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

	// 繝弱・繝・ち繧､繝励＃縺ｨ縺ｮ蜉ｹ譫憺浹繝代せ
	namespace {
		const char* GetHitSfxPath(NoteType t) {
			switch (t) {
			case NoteType::Tap: return "Assets\\sound\\tap.wav";
			case NoteType::Skill: return "Assets\\sound\\tap.wav";
			default:            return "Assets\\sound\\tap.wav";
			}
		}
	}

	// -----------------------------
	// 螳壽焚螳夂ｾｩ
	// -----------------------------
	static constexpr int    K_BEATS_PER_MEASURE = 4;
	static constexpr double K_DEFAULT_BPM = 120.0;
	static constexpr double K_OFFSET_SEC = 0.15;
	static constexpr float  INPUT_OFFSET_SEC = 0.0f;

	// 蛻､螳壹え繧｣繝ｳ繝峨え・育ｧ抵ｼ・
	static constexpr float J_WIN_PERFECT = 0.050f;
	static constexpr float J_WIN_GREAT = J_WIN_PERFECT + 0.030f;
	static constexpr float J_WIN_GOOD = J_WIN_GREAT + 0.030f;

	// 蛻､螳壽嫌蜍戊｣懷勧
	static constexpr float J_POS_TOL_MULT = 2.0f;

// ... (Top of file: Remove local TempoEvent struct)
    // -----------------------------
    // BPM螟牙喧蟇ｾ蠢懃畑縺ｮ讒矩菴・
    // -----------------------------
    // MOVED TO HEADER

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
    // 蛻晄悄蛹厄ｼ夊ｭ憺擇隱ｭ縺ｿ霎ｼ縺ｿ繝ｻ繝弱・繝・函謌・
    // ==================================================
    void NoteManager::Begin() {
        // UI荳翫・ 7.0 縺・蜀・Κ蛟､ 30.0 縺ｫ縺ｪ繧九ｈ縺・↓螟画鋤
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

        // Mouse Slash Init
        GetCursorPos(&lastCursorPos);
        mouseSpeed = 0.0f;

        auto owner = actorRef.Target(); if (!owner) return;
        auto* scene = &owner->GetScene();

        std::string chartPath = "Save/chart/Sample.chart";
        if (auto* ingameScene = dynamic_cast<IngameScene*>(scene)) {
            const SongInfo& selectedSong = ingameScene->GetSelectedSong();
            if (!selectedSong.chartPath.empty())
                chartPath = selectedSong.chartPath;
        }

        noteSequence.clear();
        tempoMap.clear(); // Use strict member
        bool sawAnyTempoMeta = false;
        noteOffset = 0.0;

        // ----------------------------------
        // 隴憺擇繝輔ぃ繧､繝ｫ隱ｭ縺ｿ霎ｼ縺ｿ
        // ----------------------------------
        {
            // 隴憺擇繧偵ヱ繝ｼ繧ｹ
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

        // Calculate HitTime for ALL notes first
        for (auto& nd : noteSequence)
            nd.hittime = static_cast<float>(BeatToSeconds(nd.absBeat, tempoMap) + K_OFFSET_SEC + noteOffset + gGameConfig.offsetSec);

        // -------------------------------------------------------------
        // CRITICAL FIX: Sort noteSequence by TIME
        // -------------------------------------------------------------
        // We must preserve original pairing logic, so we need to track original indices or re-pair after sort.
        // Re-pairing is safer. We will use `pendingHolds` mechanism logic but adapted for post-sort.
        // Actually, we can just attach an original ID before sort, then fix pairIndex.
        
        for(int i=0; i<(int)noteSequence.size(); ++i) {
            // noteSequence[i].originalIndex = i; // Removed: Field does not exist and is not strictly needed for re-pairing logic below
        }
        
        // Use stable_sort to keep relative order of simultaneous notes (e.g. correct lane order from parser)
        std::stable_sort(noteSequence.begin(), noteSequence.end(), 
            [](const NoteData& a, const NoteData& b) {
                return a.hittime < b.hittime;
            });

        // Re-calculate pairIndex because indices shifted after sort
        // We can scan and re-link HoldStart/HoldEnd.
        // Since we know they share lane and holdId (wait, NoteData doesn't have holdId stored... parser had it).
        // If NoteData doesn't store holdId, we have a problem re-linking if multiple holds in same lane overlap (rare but possible in some formats, forbidden in others).
        // Assuming no overlaps per lane: HoldStart looks for next HoldEnd in same lane.
        
        // However, we already set pairIndex based on original indices.
        // If sorting changes indices, `pairIndex` acts as a stale pointer.
        // To fix this without storing holdId, we need a mapping: OldIndex -> NewIndex.
        std::vector<int> oldToNew(noteSequence.size());
        for(int i=0; i<(int)noteSequence.size(); ++i) {
            // But we don't have OriginalIndex stored in NoteData?
            // Let's assume we can't easily map back without extra memory.
            
            // Better strategy:
            // Since we already Linked them in Parser loop using `pairIndex`, 
            // `nd.pairIndex` currently holds the OLD index.
            // But validation is hard.
            
            // AUTOMATIC RE-PAIRING STRATEGY (Robust):
            // 1. Reset all pairIndices.
            // 2. Iterate sorted sequence.
            // 3. Keep a stack/queue of HoldStarts per lane.
            // 4. When HoldEnd checks in, match with pending HoldStart.
        }
        
        // Let's implement robust re-pairing:
        std::vector<int> pendingStartIdx(6, -1);
        for(int i=0; i<(int)noteSequence.size(); ++i) {
            noteSequence[i].pairIndex = -1; // Reset
            
            if (noteSequence[i].type == NoteType::HoldStart) {
                // If there was a pending start, that's an error (overlap), but let's override or ignore.
                pendingStartIdx[noteSequence[i].lane] = i;
            }
            else if (noteSequence[i].type == NoteType::HoldEnd) {
                int startIdx = pendingStartIdx[noteSequence[i].lane];
                if (startIdx != -1) {
                    // Link
                    noteSequence[startIdx].pairIndex = i;
                    noteSequence[i].pairIndex = startIdx;
                    
                    // Clear pending
                    pendingStartIdx[noteSequence[i].lane] = -1;
                }
            }
        }

        // Calculate Duration for Holds (After Sort and Re-pair)
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
		// 繝ｬ繝ｼ繝ｳ繝代Λ繝｡繝ｼ繧ｿ
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
		// 繝弱・繝・函謌・
		// ----------------------------------
        noteActors.clear();
        nextIndex = 0;
        currentCombo = 0;

        // ... (Actor Loop) ...
        for (size_t i = 0; i < noteSequence.size(); ++i) {
             // ...
             // Same as original
             // ...
             if (noteSequence[i].type == NoteType::SongEnd) {
				noteActors.push_back({});
				continue;
			}
            // ... (Generate Actor) ...
            
			float laneX = 0.0f;
			float laneYOffset = 0.0f; 

			if (noteSequence[i].lane == 4) {
				laneX = sideLaneX_Left;
			}
			else if (noteSequence[i].lane == 5) {
				laneX = sideLaneX_Right;
			}
            else if (noteSequence[i].type == NoteType::Skill) {
                // Skill Note spans all lanes, center it.
                laneX = 0.0f;
            }
			else {
				laneX = (noteSequence[i].lane - 4 * 0.5f + 0.5f) * laneW;
			}

			auto noteActor = scene->Instantiate();
            // ... (Actor Setup) ...
			// [INSTANCING] Only add Mesh for Hold Notes and Skill Notes (they need individual rendering)
            // Tap notes use instanced rendering and don't need Mesh component
            if (noteSequence[i].type == NoteType::HoldStart || 
                noteSequence[i].type == NoteType::HoldEnd ||
                noteSequence[i].type == NoteType::Skill) {
                auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
                mesh->SetGeometry(g_cube);
                mesh->material.SetColor({ 1, 1, 1, 1 });

                // Skill Note Color: LightBlue
                if (noteSequence[i].type == NoteType::Skill) {
                     // Light Blue (Cyan)
                     mesh->material.SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
                     // Span 4 lanes
                     noteActor.Target()->transform.SetScale({ laneW * 4.0f, 0.5f, 0.2f });
                } else {
                     noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
                }
            } else {
                 // Tap / Others
                 noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
            }
			 noteActor.Target()->transform.SetPosition({ laneX, startY + laneYOffset, startZ });
			 noteActor.Target()->transform.SetRotation({ rotX, 0, 0 });

			auto comp = noteActor.Target()->AddComponent<NoteComponent>();
			comp->info = noteSequence[i];
			comp->leadTime = leadTime;
			comp->noteSpeed = noteSpeed;

			noteActor.Target()->DeActivate();
			noteActors.push_back(noteActor);
        } // Loop End

        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        activeHolds.assign(lanes, -1);
        activeHoldNextBeats.assign(lanes, 0.0); // Init
        
        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            // Cache SongEnd index for O(1) lookup in CheckMissedNotes
            if (noteSequence[i].type == NoteType::SongEnd) {
                songEndIndex = i;
            }
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }
        
        // 各レーンのノーツをhittime順にソート（noteSequenceの順序が時間順でない場合の対策）
        // noteSequence自体をBegin()でソート済みのため、ここでのソートは不要になった
        /*
        // 同時刻のノーツ順序を保持するためにstable_sortを使用
        for (int lane = 0; lane < lanes; ++lane) {
            std::stable_sort(laneOrder[lane].begin(), laneOrder[lane].end(),
                [this](int a, int b) { return noteSequence[a].hittime < noteSequence[b].hittime; }
            );
        }
        */

        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));

        try {
            InitInstancing();
            sf::debug::Debug::Log("InitInstancing Success");
        }
        catch (const std::exception& e) {
            sf::debug::Debug::LogError("InitInstancing Failed: " + std::string(e.what()));
        }
        catch (...) {
            sf::debug::Debug::LogError("InitInstancing Failed: Unknown Exception");
        }
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


        // ==========================================
        // Mouse Slash Handling for Skill Notes
        // ==========================================
        POINT currentCursorPos;
        GetCursorPos(&currentCursorPos);
        
        float dx = (float)(currentCursorPos.x - lastCursorPos.x);
        float dy = (float)(currentCursorPos.y - lastCursorPos.y);
        float mouseSpeed = std::sqrt(dx * dx + dy * dy);
        
        // Threshold: e.g., 10 pixels per frame (Lowered from 20 to trigger earlier).
        if (mouseSpeed > 10.0f) {
            // Check ALL lanes for Skill Notes
            for (int l = 0; l < (int)laneOrder.size(); ++l) {
                // Peek
                auto& order = laneOrder[l];
                size_t& head = laneHeads[l];
                size_t maxScan = std::min(head + 3, order.size()); // Check top 3 notes

                for (size_t k = head; k < maxScan; ++k) {
                    int idx = order[k];
                    if (noteSequence[idx].judged) continue;
                    
                    // Only Skill Notes
                    if (noteSequence[idx].type == NoteType::Skill) {
                         // Check Timing (within Good window)
                         float diff = std::abs((songTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime);
                         if (diff <= J_WIN_GOOD) {
                             // Hit!
                             noteSequence[idx].judged = true;
                             noteSequence[idx].result = JudgeResult::Perfect;
                             JudgeStatsService::AddResult(JudgeResult::Perfect);
                             
                             // Log & Effect
                             sf::debug::Debug::Log("Skill Slash Hit! Lane=" + std::to_string(l) + " Speed=" + std::to_string(mouseSpeed));
                             if (auto* scene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene())) {
                                 scene->TriggerSkillEffect();
                             }
                             
                             // Spawn Hit Effect
                             if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                 // Central hit effect (Slash)
                                 // Horizontal long: 16.0f width, 4.0f height
                                 // Position: Lowered further to -300
                                 canvas->SpawnSlashEffect(0.0f, -300.0f, 16.0f, 4.0f, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
                             }
                             
                             // Destroy
                             if (auto* act = noteActors[idx].Target()) {
                                 act->DeActivate();
                                 act->Destroy();
                             }
                         }
                    }
                }
            }
        }
        
        lastCursorPos = currentCursorPos;
	}

	// ==================================================
	// 蛻､螳壹Ο繧ｸ繝・け
	// ==================================================
	JudgeResult NoteManager::JudgeNote(NoteType type, float noteTime, float inputTime) {
		inputTime += INPUT_OFFSET_SEC;
		float diff = std::abs(noteTime - inputTime);

        // Relaxed window for Hold Start
        if (type == NoteType::HoldStart) {
            if (diff <= J_WIN_PERFECT * 2.0f) return JudgeResult::Perfect; // Doubled
            if (diff <= J_WIN_GREAT * 1.5f)   return JudgeResult::Great;   // 1.5x
            if (diff <= J_WIN_GOOD * 1.5f)    return JudgeResult::Good;    // 1.5x
        } else {
            if (diff <= J_WIN_PERFECT) return JudgeResult::Perfect;
            if (diff <= J_WIN_GREAT)   return JudgeResult::Great;
            if (diff <= J_WIN_GOOD)    return JudgeResult::Good;
        }
		return JudgeResult::Miss;
	}

	// ==================================================
	// レーン判定
	// ==================================================
	JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
        if (lane < 0 || lane >= 6) { // Fixed lane count
             return JudgeResult::Skip;
        }

		auto& order = laneOrder[lane];
		size_t& head = laneHeads[lane];
        
        // Loop to find the best candidate note
        // Look ahead a few notes (e.g., 5) to tolerate some overlap/congestion
        size_t currentIdx = head;
        int lookAheadCount = 0;
        const int MAX_LOOKAHEAD = 5;

        while (currentIdx < order.size() && lookAheadCount < MAX_LOOKAHEAD) {
            
            // Skip already judged notes (should be handled by head increment, but just in case)
            if (noteSequence[order[currentIdx]].judged) {
                if (currentIdx == head) ++head; // Advance head if it was pointing to judged
                ++currentIdx;
                continue;
            }

            int idx = order[currentIdx];

            // EARLY SKIP: SongEnd notes must NEVER be processed by player input.
            // They are handled exclusively by CheckMissedNotes for scene transition.
            if (noteSequence[idx].type == NoteType::SongEnd) {
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            // 1. Check Spawn Status
			if (idx >= (int)nextIndex) {
                 // Not spawned yet. Since sorted by time, future notes define end of scope.
                 // Stop searching.
                 return JudgeResult::Skip;
            }

            // 2. Check Actor Validity
            auto* act = noteActors[idx].Target();
            if (!act) {
                // If head is null, skip it permanently
                if (currentIdx == head) ++head;
                ++currentIdx;
                continue;
            }

            float noteZ = act->transform.GetPosition().z;
        
            // Ignore HoldEnd notes for tap judgment. They are handled by CheckHold.
            // Also ignore Skill Notes for normal Tap logic (handled by Mouse Slash).
            if (noteSequence[idx].type == NoteType::HoldEnd || 
                noteSequence[idx].type == NoteType::Skill) {
                // Not a tap target. Skip it.
                if (currentIdx == head) ++head; 
                ++currentIdx;
                continue;
            }
        
            // HOLD NOTES: Adjust Z for Head
            if (noteSequence[idx].type == NoteType::HoldStart) {
                 float slopeRad = rotX * 3.14159265f / 180.0f;
                 float halfLenZ = act->transform.GetScale().z * std::cos(slopeRad) * 0.5f;
                 noteZ -= halfLenZ;
            }

            // 3. Time Judgment
		    JudgeResult res = JudgeNote(noteSequence[idx].type, noteSequence[idx].hittime, inputTime);
		
		    // 4. Decision Logic
		    if (res == JudgeResult::Miss) {
                // Time-based MISS (Too late or too early)
                // Need to distinguish Late (Past) vs Early (Future) processing
                // JudgeNote returns Miss if outside Good window.
                
                float diff = inputTime - noteSequence[idx].hittime;
                // Positive diff = Input is AFTER note (Note is in Past)
                // Negative diff = Input is BEFORE note (Note is in Future)

                if (diff > J_WIN_GOOD) {
                    // Note is in the PAST (Too Late).
                    // This is a "Zombie Note" that hasn't been killed by CheckMissedNotes yet.
                    // We should IGNORE this note and try the NEXT one.
                    // Do NOT consume input.
                    // Do NOT increment global head (leave it for CheckMissed to kill).
                    
                    // Proceed to next candidate
                    ++currentIdx;
                    continue; 
                } 
                else if (diff < -J_WIN_GOOD) {
                    // Note is in the FUTURE (Too Early).
                    // Since notes are sorted, any subsequent notes are even further in future.
                    // Stop searching.
                    return JudgeResult::Skip;
                }
                
                // If logic falls here, it's weird (inside window but Miss? JudgeNote shouldn't do that).
                // Just in case, treat as Miss or Position Check.
                
			    float diffZ = std::abs(noteZ - judgeZ);
			    if (diffZ > judgeRange * J_POS_TOL_MULT) {
                    // Positionally too far.
                    // If note is future -> Stop.
                    // If note is past -> Continue.
                     if (diff < 0) return JudgeResult::Skip; // Future
                     else {
                         ++currentIdx; continue; // Past
                     }
			    }
			    // Position Close + Time Miss? (Maybe close to border?)
                // Treat as Miss judgment.
                // CONFIRM MISS.
			    // sf::debug::Debug::Log("JudgeLane: Miss (Position Close) lane=" + std::to_string(lane) + " idx=" + std::to_string(idx));
		    }
            
            // If we got here, it's either Perfect/Great/Good/Miss(Close).
            // We found our target!
            
            // APPLY JUDGMENT
		    noteSequence[idx].judged = true;
		    noteSequence[idx].result = res;
		    JudgeStatsService::AddResult(res);

            if (res != JudgeResult::Skip && res != JudgeResult::Miss) {
                 // sf::debug::Debug::Log("JudgeLane: Result=" + judgeResultToString(res) + " L=" + std::to_string(lane) + " Idx=" + std::to_string(idx));
                 
                 // Skill Activation
                 if (noteSequence[idx].type == NoteType::Skill) {
                     sf::debug::Debug::Log("Skill Activated! Lane=" + std::to_string(lane));
                     
                     // Trigger Scene Effect
                     if (auto* scene = dynamic_cast<IngameScene*>(&actorRef.Target()->GetScene())) {
                         scene->TriggerSkillEffect();
                     }
                 }

                 // Spawn Effect
                 if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
					float hitY = -265.0f;
				    float uiLaneWidth = 300.0f;
				    float sideOffset = 250.0f;
				    float hitX = 0.0f;
				    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
				    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
				    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
				    float scale = (uiLaneWidth / 100.0f) * 1.5f;
				    canvas->SpawnHitEffect(hitX, hitY, scale, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
			    }
            }

            // Register Active Hold
            if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
                if (noteSequence[idx].type == NoteType::HoldStart && noteSequence[idx].pairIndex != -1) {
                    if (activeHolds[lane] != -1) {
                         // Overwrite warning
                    }
                    activeHolds[lane] = noteSequence[idx].pairIndex;
                    activeHoldNextBeats[lane] = noteSequence[idx].absBeat + 0.5;
                }
            }
            
            // FAST/SLOW Update (Simplified)
            if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
                 float diff = (inputTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime;
                 if (diff < 0) JudgeStatsService::AddFast(); else JudgeStatsService::AddSlow();
                 // Valid UI Update here if needed...
            }

            // Destroy Actor Logic
            bool shouldDestroy = true;
            if (noteSequence[idx].type == NoteType::HoldStart) shouldDestroy = false;
            if (shouldDestroy) {
                act->DeActivate();
                act->Destroy();
            }

            // Update Head:
            // Since we might have skipped some "Past/Zombie" notes to get here,
            // strictly speaking we processed `currentIdx`.
            // Should we update `head` to `currentIdx + 1`?
            // If we do, we implicitly "Give Up" on the skipped zombie notes (accepting they are Miss).
            // Yes, if we hit a later note, the previous ones are definitely Misses that we jumped over.
            // CheckMissedNotes will clean them up later, or we can leave head optimization compliant.
            
            // For safety, only advance head if we judged head. If we judged deeper, leaving head allows CheckMissed to see the gap.
            // BUT, if we don't advance head, next JudgeLane will scan them again (wasteful but safe).
            // However, noteSequence[idx].judged = true now.
            // So next time loop will skip it via `judged` check.
            
            // Better: If we exactly judged `laneHeads[lane]`, increment.
            // If we judged a future note, we leave head pointing to the zombie note so CheckMissed can catch it.
            // The `judged` flag on the future note prevents double judgment.
            
            // WAIT! strict loop at top: `while (head < size && judged) ++head`
            // So if we judge a future note (currentIdx > head), `head` stays at zombie.
            // Next frame, loop starts at zombie again -> skips zombie -> hits marked future note -> skips marked future note -> ...
            // This is efficient enough.
            
            return res;
            
            ++lookAheadCount;
        }

        // Nothing found in lookahead range
		return JudgeResult::Skip;
	}

	void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
		for (int l : pressedLanes) {
			JudgeLane(l, inputTime);
		}
	}

	void NoteManager::DebugForceComplete() {
		sf::debug::Debug::Log("DEBUG: Force Complete Triggered");

		for (auto& note : noteSequence) {
			if (note.judged) continue;
			if (note.type == NoteType::SongEnd) continue; // Skip transition trigger for now

			// Mark as Perfect
			note.judged = true;
			note.result = JudgeResult::Perfect;
			
			// Add Stats
			JudgeStatsService::AddResult(JudgeResult::Perfect);
		}

		// Calculate total combo based on all notes processed
		// Note: JudgeStatsService::AddResult tracks combo count internally? 
		// Yes, AddResult typically increments combo. 
		// But AddCombo(1) might be needed for Hold ticks?
		// "Simply treat as all perfect" -> Let's assume just hitting the notes + holds naturally.
		// For simplicity, we just mark them. 
		// If Hold Ticks are important for score, strict counting is complex (depends on duration).
		// But usually Score is based on results.
		// Let's assume AddResult(Perfect) is sufficient for "All Perfect" status for basic notes.
		// For Holds, we might miss the tick combos if we don't simulate ticks.
		// HOWEVER, user said "Treat all as perfect". Usually implies Max Combo + All Perfect judgments.
		// Simply iterating notes and adding Perfect is a good approximation.

		// Stop BGM
		/*if (auto* bgm = actorRef.Target()->GetComponent<BGMComponent>()) {
			bgm->Stop();
		}*/

		// Stop BGM - Removed to prevent potential crash (CheckMissedNotes doesn't stop it)
		/*
		if (auto* bgm = actorRef.Target()->GetComponent<app::test::BGMComponent>()) {
			bgm->Stop(); // Verify Stop method exists or use Pause
		}
		*/

		// Force Transition to Result
		// Force Transition to Result
		if (!sceneChanger.isNull()) {
			sf::debug::Debug::Log("DEBUG: Calling ClearAllRegistered");
			sf::Mesh::ClearAllRegistered();
			ShowCursor(TRUE);
			LoadingScene::SetLoadingType(LoadingType::Common);
			sf::debug::Debug::Log("DEBUG: Calling ChangeScene");
			if (resultScene.Get() == nullptr) {
				sf::debug::Debug::Log("DEBUG: resultScene is null, calling StandbyScene");
				sceneChanger->ChangeScene(ResultScene::StandbyScene());
			}
			else {
				sceneChanger->ChangeScene(resultScene);
			}
		}
	}

	void NoteManager::CheckMissedNotes() {
		// =====================================================
		// PRIORITY CHECK: SongEnd (O(1) using cached index)
		// =====================================================
		if (songEndIndex >= 0 && songEndIndex < (int)noteSequence.size()) {
			auto& songEnd = noteSequence[songEndIndex];
			if (!songEnd.judged && songEndIndex < (int)nextIndex) {
				if (songTime > songEnd.hittime + J_WIN_GOOD) {
					sf::debug::Debug::Log("SongEnd triggered at idx=" + std::to_string(songEndIndex));
					if (!sceneChanger.isNull()) {
						sf::Mesh::ClearAllRegistered();
						ShowCursor(TRUE);

						LoadingScene::SetLoadingType(LoadingType::Common);
						sceneChanger->ChangeScene(resultScene);
					}
					songEnd.judged = true;
					return;
				}
			}
		}

		// =====================================================
		// SIMPLE: Check ALL lanes every frame (no round-robin)
		// =====================================================
		// 毎フレーム全レーンをチェック（シンプルで信頼性の高い方式）
		const int totalLanes = (int)laneOrder.size();
		
		for (int l = 0; l < totalLanes; ++l) {
			auto& order = laneOrder[l];
			size_t& head = laneHeads[l];

			while (head < order.size()) {
				int idx = order[head];
				if (noteSequence[idx].judged) { ++head; continue; }
				if (idx >= (int)nextIndex) break;
				
				// SongEnd は優先チェックで処理済みなのでスキップ
				if (noteSequence[idx].type == NoteType::SongEnd) { ++head; continue; }

				if (songTime > noteSequence[idx].hittime + J_WIN_GOOD) {
                    // Miss Processing
					noteSequence[idx].judged = true;
					noteSequence[idx].result = JudgeResult::Miss;
					JudgeStatsService::AddResult(JudgeResult::Miss);

                    // Re-entry Logic: If HoldStart is missed, we still register it as "Active" 
                    // so the player can pick it up later.
                    if (noteSequence[idx].type == NoteType::HoldStart) {
                        if (noteSequence[idx].pairIndex != -1) {
                            activeHolds[l] = noteSequence[idx].pairIndex;
                            // Initialize Combo Beat for late entry
                            activeHoldNextBeats[l] = std::max(noteSequence[idx].absBeat + 0.5, SecondsToBeat(songTime));
                        }
                    } 
                    // If HoldEnd is missed, strictly fail the hold (Destroy Start Actor)
                    else if (noteSequence[idx].type == NoteType::HoldEnd) {
                        int startIdx = noteSequence[idx].pairIndex;
                        if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
                            if (auto* act = noteActors[startIdx].Target()) {
                                act->DeActivate();
                                act->Destroy();
                            }
                        }
                        activeHolds[l] = -1;
                    } 
                    else {
                        // Normal Note Miss -> Destroy
					    if (auto* act = noteActors[idx].Target()) {
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
		// songTime = 0.0f; // 竊・蜑企勁: Begin縺ｧ險ｭ螳壹＆繧後◆leadTime・医・繧､繝翫せ蛟､・峨ｒ邯ｭ謖√☆繧・
	}

	void NoteManager::SetCurrentSongTime(float time) {
		this->songTime = time;
	}

	void NoteManager::SyncTime(float time) {
		// 譎る俣蟾ｮ・医ラ繝ｪ繝輔ヨ・峨ｒ險育ｮ・
		float diff = time - this->songTime;

		// 繧ｺ繝ｬ縺悟､ｧ縺阪☆縺弱ｋ蝣ｴ蜷茨ｼ井ｾ九∴縺ｰ 50ms莉･荳奇ｼ峨・
        // 縺ｾ縺溘・雋縺ｮ譁ｹ蜷代↓螟ｧ縺阪☆縺弱ｋ・域凾髢薙′鬟帙・驕弱℃縺ｦ謌ｻ縺｣縺滂ｼ牙ｴ蜷医・蠑ｷ蛻ｶ逧・↓蜷医ｏ縺帙ｋ縲・
        // 窶ｻ蟆代＠縺上ｉ縺・・雋縺ｮ繧ｺ繝ｬ・・ame縺碁ｲ縺ｿ縺吶℃・峨・險ｱ螳ｹ縺励※Lerp縺ｧ蜷医ｏ縺帙ｋ・医き繧ｯ縺､縺埼亟豁｢・・
		if (std::abs(diff) > 0.05f) {
			this->songTime = time;
		}
		else {
            if (time <= 0.001f) {
                // 髻ｳ螢ｰ縺後∪縺蟋九∪縺｣縺ｦ縺・↑縺・0)縺ｮ蝣ｴ蜷医・縲∝酔譛溘○縺壹↓繧ｲ繝ｼ繝譎る俣繧帝ｲ繧√ｋ(DeltaTime莉ｻ縺・
                // 縺薙ｌ縺ｫ繧医ｊStart逶ｴ蠕後・縲梧ｺ懊ａ縲阪〒繝弱・繝・′豁｢縺ｾ繧九・繧帝亟縺・
            }
            else {
			    // 繧ｺ繝ｬ縺悟ｰ上＆縺・ｴ蜷医・縲∬｣憺俣縺励※貊代ｉ縺九↓霑代▼縺代ｋ
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
            return; 
        }

		auto& endNote = noteSequence[idx];
		if (endNote.judged) {
			activeHolds[lane] = -1; // Already done
			return;
		}

		if (isPressed) {
            // Check if we can add combo
            double curBeat = SecondsToBeat(songTime);
            // While to catch up if we missed a frame (unlikely but safe)
            while (curBeat >= activeHoldNextBeats[lane] && activeHoldNextBeats[lane] < endNote.absBeat) {
                // AddCombo(1); // Add 1 Combo (Split for smoother feel)
                
                // Treat hold ticks as Perfect judgements to match Total Notes with Max Combo
                JudgeResult res = JudgeResult::Perfect;
                JudgeStatsService::AddResult(res);
                UpdateCombo(res);

                activeHoldNextBeats[lane] += 0.5; // Next 0.5 beat
            }

			// Check if we reached End
			if (songTime >= endNote.hittime - J_WIN_PERFECT) { 
				// sf::debug::Debug::Log("CheckHold: Hold Complete! Lane=" + std::to_string(lane));
				
				JudgeResult res = JudgeResult::Perfect;
				endNote.judged = true;
				endNote.result = res;
				JudgeStatsService::AddResult(res);
				// UpdateCombo(res); // Handled by JudgeStatsService::AddResult

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

					canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
				}


				// Destroy HoldStart Actor
				int startIdx = endNote.pairIndex;
				if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
					if (auto* act = noteActors[startIdx].Target()) {
						act->DeActivate();
						act->Destroy();
					}
				}

                // ==========================================================
                // Chain Hold Logic (Auto-Judge Next Hold)
                // ==========================================================
                bool foundChain = false;
                
                // Look ahead for the next note in this lane
                if (lane >= 0 && lane < (int)laneOrder.size()) {
                    auto& order = laneOrder[lane];
                    // We start searching from laneHeads[lane]. 
                    // Note: laneHeads usually points to the *first unjudged*. 
                    // Since 'endNote' was just judged (we set judged=true above), 
                    // laneHeads might still be pointing to endNote or before it if CheckMissedNotes hasn't run.
                    // But actually, we don't rely strictly on laneHeads being perfectly up to date for *this* frame's specific note index.
                    // We just need to find the NEXT unjudged HoldStart.
                    
                    // Simple search:
                    for (size_t k = laneHeads[lane]; k < order.size(); ++k) {
                        int nextIdx = order[k];
                        auto& nextNote = noteSequence[nextIdx];

                        if (nextNote.judged) continue; // Skip judged
                        if (nextNote.type == NoteType::HoldEnd) continue; // Skip ends (shouldn't be first usually but logic)

                        // We found the next candidate (Tap or HoldStart)
                        if (nextNote.type == NoteType::HoldStart) {
                            // Check timing difference
                            // If the new hold starts within, say, "Great" window of current time (or from endNote time), allow chain.
                            // Using songTime is safer as it represents "Now".
                            float diff = std::abs(nextNote.hittime - songTime);
                            
                            // Allow chain if it's close enough (e.g. Good Window)
                            // This covers "Zero Gap" or "Tiny Gap" holds.
                            if (diff <= J_WIN_GOOD) {
                                // sf::debug::Debug::Log("CheckHold: Chain Hold Detected! Old=" + std::to_string(idx) + " New=" + std::to_string(nextIdx));

                                // Auto Judge as Perfect (Reward for holding)
                                nextNote.judged = true;
                                nextNote.result = JudgeResult::Perfect;
                                JudgeStatsService::AddResult(JudgeResult::Perfect);
                                // UpdateCombo(JudgeResult::Perfect); // Handled by Service

                                // Just in case, if there's a Tap Effect for Start
                                if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
					                sound->Play(GetHitSfxPath(NoteType::Tap)); // Play Tap sound for new start
				                }
                                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                    // Visual Effect provided by JudgeLane normally... let's spawn one here
                                    float uiLaneWidth = 300.0f;
					                float sideOffset = 250.0f;
					                float hitX = 0.0f;
					                if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
					                else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
					                else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
					                canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
                                }

                                // Update Active Hold to this new one
                                if (nextNote.pairIndex != -1) {
                                    activeHolds[lane] = nextNote.pairIndex;
                                    activeHoldNextBeats[lane] = nextNote.absBeat + 0.5;
                                    foundChain = true;
                                }
                            }
                        }
                        // Stop after checking the very next note. We don't skip over Taps to find a Hold.
                        break; 
                    }
                }

                if (!foundChain) {
				    activeHolds[lane] = -1;
                }
			} else {
				// Holding... logic
			}
		} else {
			// Released (Gap Logic)
            // Instead of failing, we just advance beats so user can't exploit combo without holding
            double curBeat = SecondsToBeat(songTime);
            while (curBeat >= activeHoldNextBeats[lane]) {
                 activeHoldNextBeats[lane] += 0.5;
            }
		}
	}

    // Helper: Seconds to Beat
    double NoteManager::SecondsToBeat(double seconds) const {
        // songTime -> Beat
        // Using tempoMap
        // We know Seconds = AtSec + (Beat - AtBeat) * SPB
        // So Beat = AtBeat + (Seconds - AtSec) / SPB
        
        // 1. Find the segment
        // Binary search for the segment with atSec <= seconds
        size_t lo = 0, hi = tempoMap.size();
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (tempoMap[mid].atSec <= seconds) lo = mid; else hi = mid;
        }
        
        if (lo >= tempoMap.size()) return 0.0;
        
        const auto& seg = tempoMap[lo];
        if (seg.spb <= 0.000001) return seg.atBeat; // Safety
        
        return seg.atBeat + (seconds - seg.atSec) / seg.spb;
    }

    void NoteManager::AddCombo(int amount) {
        currentCombo += amount;
        JudgeStatsService::AddCombo(amount);
    }

	// ==================================================
	// Instancing Implementation
	// ==================================================
    void NoteManager::InitInstancing()
    {
        sf::debug::Debug::Log("InitInstancing [" + std::to_string((uintptr_t)this) + "]");
        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();

        // 1. Create Instance Buffer (Dynamic)
        m_instanceDataCPU.reserve(2048);

		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = sizeof(NoteInstanceData) * 2048;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

        HRESULT hr = device->CreateBuffer(&desc, nullptr, &m_instanceBuffer);
        if (FAILED(hr)) {
            sf::debug::Debug::LogError("Failed to create Instance Buffer");
        }

        // 2. Create Manual Cube Mesh (Positions, Normals, UVs, Colors)
        // Unit Cube centered at 0, size 1x1x1 (-0.5 to 0.5)
        // Simple Vertex Struct for internal use
        struct SimpleVertex {
            float pos[3];
            float nor[3];
            float uv[2];
            float col[4];
        };

        // 24 vertices (4 per face * 6 faces)
        SimpleVertex vertices[] = {
            // Front Face (Z = -0.5) Normal (0,0,-1)
            { {-0.5f, -0.5f, -0.5f}, {0,0,-1}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {0,0,-1}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,0,-1}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,0,-1}, {1,1}, {1,1,1,1} },

            // Back Face (Z = +0.5) Normal (0,0,1)
            { { 0.5f, -0.5f,  0.5f}, {0,0,1}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,0,1}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,0,1}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f,  0.5f}, {0,0,1}, {1,1}, {1,1,1,1} },

            // Top Face (Y = +0.5) Normal (0,1,0)
            { {-0.5f,  0.5f, -0.5f}, {0,1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {0,1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {0,1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {0,1,0}, {1,1}, {1,1,1,1} },

            // Bottom Face (Y = -0.5) Normal (0,-1,0)
            { {-0.5f, -0.5f,  0.5f}, {0,-1,0}, {0,1}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {0,-1,0}, {0,0}, {1,1,1,1} },
            { { 0.5f, -0.5f, -0.5f}, {0,-1,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {0,-1,0}, {1,1}, {1,1,1,1} },

            // Left Face (X = -0.5) Normal (-1,0,0)
            { {-0.5f, -0.5f,  0.5f}, {-1,0,0}, {0,1}, {1,1,1,1} },
            { {-0.5f,  0.5f,  0.5f}, {-1,0,0}, {0,0}, {1,1,1,1} },
            { {-0.5f,  0.5f, -0.5f}, {-1,0,0}, {1,0}, {1,1,1,1} },
            { {-0.5f, -0.5f, -0.5f}, {-1,0,0}, {1,1}, {1,1,1,1} },

            // Right Face (X = +0.5) Normal (1,0,0)
            { { 0.5f, -0.5f, -0.5f}, {1,0,0}, {0,1}, {1,1,1,1} },
            { { 0.5f,  0.5f, -0.5f}, {1,0,0}, {0,0}, {1,1,1,1} },
            { { 0.5f,  0.5f,  0.5f}, {1,0,0}, {1,0}, {1,1,1,1} },
            { { 0.5f, -0.5f,  0.5f}, {1,0,0}, {1,1}, {1,1,1,1} },
        };

        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = sizeof(SimpleVertex) * 24;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA vInitData{};
        vInitData.pSysMem = vertices;
        hr = device->CreateBuffer(&vbd, &vInitData, &m_cubeVB);
        if (FAILED(hr)) sf::debug::Debug::LogError("Failed to create Cube VB");

        // Indices
        unsigned int indices[] = {
            0,1,2, 0,2,3,       // Front
            4,5,6, 4,6,7,       // Back
            8,9,10, 8,10,11,    // Top
            12,13,14, 12,14,15, // Bottom
            16,17,18, 16,18,19, // Left
            20,21,22, 20,22,23  // Right
        };
        m_cubeIndexCount = 36;
        
        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = sizeof(unsigned int) * 36;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA iInitData{};
        iInitData.pSysMem = indices;
        hr = device->CreateBuffer(&ibd, &iInitData, &m_cubeIB);
        if (FAILED(hr)) sf::debug::Debug::LogError("Failed to create Cube IB");

        // 3. Load Shader & Create Layout Manually
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        
        // Compile
        sf::debug::Debug::Log("Compiling Shader: vsNoteInstancing.hlsl");
        hr = D3DCompileFromFile(L"vsNoteInstancing.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                sf::debug::Debug::LogError((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            } else {
                sf::debug::Debug::LogError("Shader Compile Failed: No Error Blob (File not found?)");
            }
            if (vsBlob) vsBlob->Release();
            return;
        }

        // Create VS
        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
        if (FAILED(hr)) sf::debug::Debug::LogError("Failed to create Instancing VS");

        // Create Input Layout
        // Must match shader input structure semantics EXACTLY
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            // Per-Vertex
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            
            // Per-Instance
            // D3D11 requires SemanticName + SemanticIndex separately
            // HLSL "INSTANCE_WORLD0" -> D3D11 "INSTANCE_WORLD" with index 0
            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        hr = device->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);
        if (FAILED(hr)) sf::debug::Debug::LogError("Failed to create Instancing InputLayout");
        
        if (FAILED(hr)) sf::debug::Debug::LogError("Failed to create Instancing InputLayout");
        
        vsBlob->Release();
        sf::debug::Debug::Log("InitInstancing Done. m_vs=" + std::to_string((uintptr_t)m_vs) + " m_layout=" + std::to_string((uintptr_t)m_layout));
    }

    void NoteManager::UpdateInstanceBuffer()
    {
        // sf::debug::Debug::Log("UpdateInstanceBuffer Start");
        m_instanceDataCPU.clear();

        // Collect visible notes (OPTIMIZATION: only check spawned notes)
        // Notes with index >= nextIndex haven't been spawned yet, so skip them
        size_t maxIdx = std::min(nextIndex, noteActors.size());
        for (size_t i = 0; i < maxIdx; ++i) {
             // Safety check for index
             if (i >= noteSequence.size()) continue;

             // SKIP JUDGED NOTES FIRST (Avoids accessing destroyed actors)
             if (noteSequence[i].judged) continue;

             // SKIP HOLD NOTES & SKILL NOTES - they use individual Mesh rendering
             if (noteSequence[i].type == NoteType::HoldStart || 
                 noteSequence[i].type == NoteType::HoldEnd ||
                 noteSequence[i].type == NoteType::Skill ||
                 noteSequence[i].type == NoteType::SongEnd) continue;

             // Check if Ref is null (e.g. SongEnd dummy)
             if (noteActors[i].IsNull()) continue;

             // Check if Ref is valid (exists in map) so Target() doesn't throw
             if (!noteActors[i].IsValid()) continue;

             // Now safely access the actor
             auto act = noteActors[i].Target();
             if (!act) continue;

             // IsActive check removed. Relying on logic visibility (judged=false)
             {
                 NoteInstanceData data;
                 DirectX::XMMATRIX m = act->transform.Matrix();
                 // Store matrix directly - no transpose needed for instancing
                 DirectX::XMStoreFloat4x4(&data.world, m);
                 data.color = { 1, 1, 1, 1 };
                 m_instanceDataCPU.push_back(data);
             }
        }

        if (m_instanceDataCPU.empty()) return;

        // Map Buffer
        sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
        auto context = dx11->GetMainDevice().GetContext();

        D3D11_MAPPED_SUBRESOURCE mapped;
        // sf::debug::Debug::Log("Mapping Instance Buffer...");
        HRESULT hr = context->Map(m_instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            size_t sizeInBytes = sizeof(NoteInstanceData) * m_instanceDataCPU.size();
            if (m_instanceDataCPU.size() > 2048) {
                 sizeInBytes = sizeof(NoteInstanceData) * 2048;
            }
            memcpy(mapped.pData, m_instanceDataCPU.data(), sizeInBytes);
            context->Unmap(m_instanceBuffer, 0);
        } else {
             sf::debug::Debug::LogError("Failed to Map Instance Buffer");
        }
    }

    void NoteManager::DrawInstanced()
    {
        // sf::debug::Debug::Log("DrawInstanced Start");
        try {
            UpdateInstanceBuffer();
            if (m_instanceDataCPU.empty()) return;
            if (!m_vs || !m_layout || !m_cubeVB || !m_cubeIB) {
                std::string msg = "DrawInstanced Skipped: missing resources. [this=" + std::to_string((uintptr_t)this) + "] ";
                if (!m_vs) msg += "VS=null "; else msg += "VS=" + std::to_string((uintptr_t)m_vs) + " ";
                if (!m_layout) msg += "Layout=null "; else msg += "Layout=" + std::to_string((uintptr_t)m_layout) + " ";
                if (!m_cubeVB) msg += "VB=null ";
                if (!m_cubeIB) msg += "IB=null ";
                sf::debug::Debug::Log(msg);
                return;
            }

            sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();
            auto context = dx11->GetMainDevice().GetContext();

            // 1. Shaders
            context->VSSetShader(m_vs, nullptr, 0);
            context->IASetInputLayout(m_layout);

            // Use existing Pixel Shader
            dx11->ps3d.SetGPU(dx11->GetMainDevice());

            // Clear Geometry Shader (important! previous draws may have set gsCube etc.)
            dx11->gsNone.SetGPU(dx11->GetMainDevice());

            // 2. Geometry
            UINT strides[2] = { sizeof(float) * 12, sizeof(NoteInstanceData) };
            UINT offsets[2] = { 0, 0 };
            ID3D11Buffer* buffers[2] = { m_cubeVB, m_instanceBuffer };

            context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
            context->IASetIndexBuffer(m_cubeIB, DXGI_FORMAT_R32_UINT, 0);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            // 3. Constant Buffers
            // Set Material Buffer manually to ensure diffuseColor is white.
            // Otherwise, it might be 0 or leftover from previous draws.
            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            material.textureEnable = { 0.0f, 0.0f, 0.0f, 0.0f }; // No texture for now (white cube)
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            // 4. Draw

            context->DrawIndexedInstanced(m_cubeIndexCount, (UINT)m_instanceDataCPU.size(), 0, 0, 0);
        } catch (const std::exception& e) {
             sf::debug::Debug::LogError("DrawInstanced Exception: " + std::string(e.what()));
        } catch (...) {
             sf::debug::Debug::LogError("DrawInstanced Unknown Exception");
        }
    }

} // namespace app::test
