#include "NoteManager.h"
#include "NoteComponent.h"
#include "Scene.h"
#include "Time.h"
#include "BGMComponent.h"
#include "Config.h"
#include "DirectX11.h"
#include <cstdio>
#pragma comment(lib, "d3dcompiler.lib")
#include <fstream>
#include <sstream>
#include <algorithm>
#include "NoteData.h"
#include "SoundComponent.h"
#include "SongInfo.h"
#include <numeric>
#include <cmath>
#include <map>
#include "GameSession.h"
#include "ChedParser.h"
#include "ResultScene.h"
#include "IngameCanvas.h"
#include "LoadingScene.h"
#include <filesystem>
#include <windows.h>
#include "StringUtils.h"  // 文字コード変換ユーティリティ

// sf::util の関数を使用（ローカル関数は削除）
namespace {
    using sf::util::Utf8ToWstring;
    using sf::util::Utf8ToShiftJis;
}

namespace app::test {

    // ============================================================
    // 効果音パス取得
    // ============================================================
    namespace {
        const char* GetHitSfxPath(NoteType t) {
            switch (t) {
            case NoteType::Tap:   return "Assets\\sound\\tap.wav";
            case NoteType::Skill: return "Assets\\sound\\tap.wav";
            default:              return "Assets\\sound\\tap.wav";
            }
        }
    }

    // ============================================================
    // 定数定義
    // ============================================================
    static constexpr int    K_BEATS_PER_MEASURE = 4;
    static constexpr double K_DEFAULT_BPM = 120.0;
    static constexpr double K_OFFSET_SEC = 0.15;
    static constexpr float  INPUT_OFFSET_SEC = 0.0f;



    // ============================================================
    // 初期化（Begin）
    // 譜面ファイルを読み込み、ノーツを生成する
    // 1. ハイスピード設定
    // 2. 譜面ファイルのパース
    // 3. テンポマップ構築
    // 4. ノーツデータのソートとペアリング
    // 5. ノーツアクターの生成
    // 6. インスタンシング初期化
    // 7. 最大コンボ数の事前計算
    // ============================================================
    void NoteManager::Begin() {
        // UI上の 7.0 が内部値 30.0 になるように変換
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
        GetCurrentSession().Reset();

        GetCursorPos(&lastCursorPos);
        mouseSpeed = 0.0f;

        auto owner = actorRef.Target();
        if (!owner) return;
        auto* scene = &owner->GetScene();

        // 譜面パス取得（DI経由で渡されたSongInfoを使用）
        std::string chartPath = "Save/chart/Sample.chart";
        if (songInfoPtr && !songInfoPtr->chartPath.empty()) {
            chartPath = songInfoPtr->chartPath;
        }

        // ------------------------------------------
        // 譜面ファイル読み込み（ChartLoaderに委譲）
        // ------------------------------------------
        ChartLoadResult chartResult = chartLoader.Load(
            chartPath, 
            K_OFFSET_SEC, 
            gGameConfig.offsetSec
        );
        
        noteSequence = std::move(chartResult.notes);
        tempoMap = std::move(chartResult.tempoMap);
        noteOffset = chartResult.noteOffset;
        comboManager.SetMaxTotalCombo(chartResult.maxTotalCombo);
        songEndIndex = chartResult.songEndIndex;


        // ------------------------------------------
        // レーンパラメータ
        // ------------------------------------------
        float slopeRad = rotX * 3.14159265f / 180.0f;
        float startZ = laneH * 0.5f;
        float hitZ = -laneH * 0.5f + laneH * barRatio;
        float startY = baseY - std::tan(slopeRad) * startZ;

        int lanes = std::max(1, (int)laneRefs.size());
        float totalDistance = std::abs(startZ - hitZ);

        leadTime = (noteSpeed != 0.f) ? (totalDistance / noteSpeed) : 0.f;
        songTime = -leadTime;

        // ------------------------------------------
        // ノーツアクター生成
        // ------------------------------------------
        noteActors.clear();
        nextIndex = 0;
        comboManager.Reset();

        for (size_t i = 0; i < noteSequence.size(); ++i) {
            if (noteSequence[i].type == NoteType::SongEnd) {
                noteActors.push_back({});
                continue;
            }

            float laneX = 0.0f;
            float laneYOffset = 0.0f;

            if (noteSequence[i].lane == 4) {
                laneX = sideLaneX_Left;
            }
            else if (noteSequence[i].lane == 5) {
                laneX = sideLaneX_Right;
            }
            else if (noteSequence[i].type == NoteType::Skill) {
                laneX = 0.0f;
            }
            else {
                laneX = (noteSequence[i].lane - 4 * 0.5f + 0.5f) * laneW;
            }

            auto noteActor = scene->Instantiate();

            // メッシュ追加（ホールド・スキルのみ）
            if (noteSequence[i].type == NoteType::HoldStart ||
                noteSequence[i].type == NoteType::HoldEnd ||
                noteSequence[i].type == NoteType::Skill) {
                auto mesh = noteActor.Target()->AddComponent<sf::Mesh>();
                mesh->SetGeometry(g_cube);
                mesh->material.SetColor({ 1, 1, 1, 1 });

                if (noteSequence[i].type == NoteType::Skill) {
                    mesh->material.SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
                    noteActor.Target()->transform.SetScale({ laneW * 4.0f, 0.5f, 0.2f });
                } else {
                    noteActor.Target()->transform.SetScale({ laneW * 0.8f, 0.5f, 0.2f });
                }
            } else {
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
        }

        // レーン管理初期化
        laneOrder.assign(lanes, {});
        laneHeads.assign(lanes, 0);
        activeHolds.assign(lanes, -1);
        activeHoldNextBeats.assign(lanes, 0.0);

        for (int i = 0; i < (int)noteSequence.size(); ++i) {
            if (noteSequence[i].type == NoteType::SongEnd) {
                songEndIndex = i;
            }
            int l = std::clamp(noteSequence[i].lane, 0, lanes - 1);
            laneOrder[l].push_back(i);
        }

        updateCommand.Bind(std::bind(&NoteManager::Update, this, std::placeholders::_1));

        try {
            InitInstancing();
        }
        catch (...) {
            sf::debug::Debug::LogError("InitInstancing Failed");
        }

        // 最大コンボ数はChartLoaderで計算済み
    }

    // ============================================================
    // 更新（Update）
    // 毎フレーム呼び出される
    // 1. 曲時間の更新
    // 2. ノーツのアクティベート（画面外から可視化）
    // 3. ミスノーツの検出
    // 4. マウススラッシュによるスキルノーツ判定
    // ============================================================
    void NoteManager::Update(const sf::command::ICommand&) {
        if (!isPlaying) return;

        // 曲時間を進める
        songTime += sf::Time::DeltaTime();

        // ノーツのアクティベート
        // leadTime分前倒しでノーツを画面に出現させる
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

        // ミスノーツの検出と処理
        CheckMissedNotes();

        // ------------------------------------------
        // マウススラッシュ処理
        // マウスの移動速度が閾値を超えたら
        // スキルノーツを判定する
        // ------------------------------------------
        POINT currentCursorPos;
        GetCursorPos(&currentCursorPos);

        float dx = (float)(currentCursorPos.x - lastCursorPos.x);
        float dy = (float)(currentCursorPos.y - lastCursorPos.y);
        float mouseSpeed = std::sqrt(dx * dx + dy * dy);

        if (mouseSpeed > 10.0f) {
            for (int l = 0; l < (int)laneOrder.size(); ++l) {
                auto& order = laneOrder[l];
                size_t& head = laneHeads[l];
                size_t maxScan = std::min(head + 3, order.size());

                for (size_t k = head; k < maxScan; ++k) {
                    int idx = order[k];
                    if (noteSequence[idx].judged) continue;

                    if (noteSequence[idx].type == NoteType::Skill) {
                        float diff = std::abs((songTime + INPUT_OFFSET_SEC) - noteSequence[idx].hittime);
                        if (diff <= K_JUDGE_GOOD) {
                            noteSequence[idx].judged = true;
                            noteSequence[idx].result = JudgeResult::Perfect;
                            GetCurrentSession().AddResult(JudgeResult::Perfect);
                            UpdateCombo(JudgeResult::Perfect);

                            // スキルエフェクト発動（コールバック経由）
                            if (onSkillTriggered) {
                                onSkillTriggered();
                            }

                            if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                canvas->SpawnSlashEffect(0.0f, -300.0f, 16.0f, 4.0f, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
                            }

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

    // ============================================================
    // 単一ノーツの時間判定
    // ノーツのヒット時間と入力時間の差から判定結果を決定
    // HoldStartは判定緩和（ホールドは始まりが重要）
    // ============================================================
    JudgeResult NoteManager::JudgeNote(NoteType type, float noteTime, float inputTime) {
        return judgeService.JudgeNote(type, noteTime, inputTime + INPUT_OFFSET_SEC);
    }

    // ============================================================
    // レーン単位の判定処理
    // 先読み範囲内から最適なノーツを探して判定
    // - 判定済みノーツはスキップ
    // - まだスポーンしていないノーツはスキップ
    // - HoldEndとSkillはタップ判定しない
    // - 位置許容度を考慮した判定
    // ============================================================

    JudgeResult NoteManager::JudgeLane(int lane, float inputTime) {
        if (lane < 0 || lane >= (int)laneOrder.size()) {
            return JudgeResult::Skip;
        }

        auto& order = laneOrder[lane];
        size_t& head = laneHeads[lane];
        
        std::vector<JudgeCandidate> candidates;
        candidates.reserve(K_LOOKAHEAD_NOTES);
        
        size_t currentIdx = head;
        int count = 0;
        const int MAX_SCAN = K_LOOKAHEAD_NOTES;

        while(currentIdx < order.size() && count < MAX_SCAN) {
             int idx = order[currentIdx];
             
             if (noteSequence[idx].judged || noteSequence[idx].type == NoteType::SongEnd) {
                 if (currentIdx == head) ++head;
                 ++currentIdx;
                 continue;
             }
             
             if (idx >= (int)nextIndex) {
                 break;
             }
            
             auto* act = noteActors[idx].Target();
             if (!act) {
                 if (currentIdx == head) ++head;
                 ++currentIdx;
                 continue;
             }
             
             if (noteSequence[idx].type == NoteType::HoldEnd || 
                 noteSequence[idx].type == NoteType::Skill) {
                     if (currentIdx == head) ++head;
                     ++currentIdx;
                     continue;
             }
             
             float noteZ = act->transform.GetPosition().z;
             
             if (noteSequence[idx].type == NoteType::HoldStart) {
                 float slopeRad = rotX * 3.14159265f / 180.0f;
                 float halfLenZ = act->transform.GetScale().z * std::cos(slopeRad) * 0.5f;
                 noteZ -= halfLenZ;
             }
             
             JudgeCandidate c;
             c.noteIndex = idx;
             c.type = noteSequence[idx].type;
             c.hittime = noteSequence[idx].hittime;
             c.zPosition = noteZ;
             candidates.push_back(c);
             
             ++count;
             ++currentIdx;
        }

        if (candidates.empty()) return JudgeResult::Skip;

        float adjustedInputTime = inputTime + INPUT_OFFSET_SEC;
        float currentJudgeZ = -laneH * 0.5f + laneH * barRatio;

        JugdeAction action = judgeService.JudgeCandidates(
            adjustedInputTime, 
            candidates, 
            currentJudgeZ
        );

        if (action.result == JudgeResult::Skip) {
            return JudgeResult::Skip;
        }
        
        int idx = action.noteIndex;
        auto& note = noteSequence[idx];
        
        JudgeResult res = action.result;
        note.judged = true;
        note.result = res;
        GetCurrentSession().AddResult(res);
        UpdateCombo(res);

        if (res != JudgeResult::Skip && res != JudgeResult::Miss) {
            if (note.type == NoteType::Skill) {
                // スキルエフェクト発動（コールバック経由）
                if (onSkillTriggered) {
                    onSkillTriggered();
                }
            }

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

        if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
            if (note.type == NoteType::HoldStart && note.pairIndex != -1) {
                activeHolds[lane] = note.pairIndex;
                activeHoldNextBeats[lane] = note.absBeat + 0.5;
            }
        }

        if (res != JudgeResult::Miss && res != JudgeResult::Skip) {
            float diff = (inputTime + INPUT_OFFSET_SEC) - note.hittime;
            int type = (diff < 0) ? 1 : 2;
            if (diff < 0) GetCurrentSession().AddFast();
            else GetCurrentSession().AddSlow();

            if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                canvas->ShowFastSlow(type);
            }
        }

        // アクター破棄
        bool shouldDestroy = true;
        if (note.type == NoteType::HoldStart) shouldDestroy = false;
        
        if (shouldDestroy) {
            auto* act = noteActors[idx].Target();
            if (act) {
                act->DeActivate();
                act->Destroy();
            }
        }

        return res;
    }

    void NoteManager::JudgeBatch(const std::vector<int>& pressedLanes, float inputTime) {
        for (int l : pressedLanes) {
            JudgeLane(l, inputTime);
        }
    }

    // ============================================================
    // デバッグ：強制クリア
    // ============================================================
    void NoteManager::DebugForceComplete() {
        for (auto& note : noteSequence) {
            if (note.judged) continue;
            if (note.type == NoteType::SongEnd) continue;

            note.judged = true;
            note.result = JudgeResult::Perfect;
            GetCurrentSession().AddResult(JudgeResult::Perfect);
            UpdateCombo(JudgeResult::Perfect);
        }

        if (!sceneChanger.isNull()) {
            sf::Mesh::ClearAllRegistered();
            ShowCursor(TRUE);
            LoadingScene::SetLoadingType(LoadingType::Common);
            if (resultScene.Get() == nullptr) {
                sceneChanger->ChangeScene(ResultScene::StandbyScene());
            }
            else {
                sceneChanger->ChangeScene(resultScene);
            }
        }
    }

    // ============================================================
    // ミスチェック
    // ============================================================
    void NoteManager::CheckMissedNotes() {
        // SongEnd処理
        if (songEndIndex >= 0 && songEndIndex < (int)noteSequence.size()) {
            auto& songEnd = noteSequence[songEndIndex];
            if (!songEnd.judged && songEndIndex < (int)nextIndex) {
                if (songTime > songEnd.hittime + K_JUDGE_GOOD) {
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

        // 各レーンのミスチェック
        const int totalLanes = (int)laneOrder.size();

        for (int l = 0; l < totalLanes; ++l) {
            auto& order = laneOrder[l];
            size_t& head = laneHeads[l];

            while (head < order.size()) {
                int idx = order[head];
                if (noteSequence[idx].judged) { ++head; continue; }
                if (idx >= (int)nextIndex) break;

                if (noteSequence[idx].type == NoteType::SongEnd) { ++head; continue; }

                if (songTime > noteSequence[idx].hittime + K_JUDGE_GOOD) {
                    noteSequence[idx].judged = true;
                    noteSequence[idx].result = JudgeResult::Miss;
                    GetCurrentSession().AddResult(JudgeResult::Miss);
                    UpdateCombo(JudgeResult::Miss);

                    if (noteSequence[idx].type == NoteType::HoldStart) {
                        if (noteSequence[idx].pairIndex != -1) {
                            activeHolds[l] = noteSequence[idx].pairIndex;
                            activeHoldNextBeats[l] = std::max(noteSequence[idx].absBeat + 0.5, SecondsToBeat(songTime));
                        }
                    }
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

    // ============================================================
    // コンボ管理
    // 判定結果に応じてコンボを更新
    // Perfect/Great/Good: +1、Miss: 0にリセット
    // ============================================================
    void NoteManager::UpdateCombo(JudgeResult result) {
        comboManager.UpdateCombo(result);
    }

    int NoteManager::GetCurrentCombo() const { return comboManager.GetCurrentCombo(); }

    void NoteManager::AddCombo(int amount) {
        comboManager.AddCombo(amount);
    }

    int NoteManager::GetMaxTotalCombo() const {
        return comboManager.GetMaxTotalCombo();
    }

    // ============================================================
    // レーン設定
    // ============================================================
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
    }

    void NoteManager::SetCurrentSongTime(float time) {
        this->songTime = time;
    }

    // ============================================================
    // 時間同期
    // BGMとノーツマネージャーの時間を同期
    // - 大きなズレ（50ms以上）: 即座に合わせる
    // - 小さなズレ: 補間で滑らかに近づける
    // - 音声未開始時: ゲーム時間をそのまま進める
    // ============================================================
    void NoteManager::SyncTime(float time) {
        float diff = time - this->songTime;

        // 大きなズレは即座に合わせる
        if (std::abs(diff) > 0.05f) {
            this->songTime = time;
        }
        else {
            // 音声未開始時は同期しない
            if (time <= 0.001f) {
            }
            else {
                // 小さなズレは補間で滑らかに
                this->songTime += diff * 0.2f;
            }
        }
    }

    // ============================================================
    // ホールドチェック
    // ホールドノーツの継続判定
    // - 押し続けている間: 0.5拍ごとにコンボ加算
    // - 終端到達時: Perfect判定、エフェクト再生
    // - チェーンホールド: 次のホールドを自動判定
    // - リリース時: コンボ加算をスキップ
    // ============================================================
    void NoteManager::CheckHold(int lane, bool isPressed) {
        if (lane < 0 || lane >= (int)activeHolds.size()) return;
        int idx = activeHolds[lane];
        if (idx == -1) return;

        auto& endNote = noteSequence[idx];
        if (endNote.judged) {
            activeHolds[lane] = -1;
            return;
        }

        if (isPressed) {
            // コンボ加算
            double curBeat = SecondsToBeat(songTime);
            while (curBeat >= activeHoldNextBeats[lane] && activeHoldNextBeats[lane] < endNote.absBeat) {
                JudgeResult res = JudgeResult::Perfect;
                GetCurrentSession().AddResult(res);
                UpdateCombo(res);
                activeHoldNextBeats[lane] += 0.5;
            }

            // 終端判定
            if (songTime >= endNote.hittime - K_JUDGE_PERFECT) {
                JudgeResult res = JudgeResult::Perfect;
                endNote.judged = true;
                endNote.result = res;
                GetCurrentSession().AddResult(res);
                UpdateCombo(res);

                if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
                    sound->Play(GetHitSfxPath(endNote.type));
                }

                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                    float uiLaneWidth = 300.0f;
                    float sideOffset = 250.0f;
                    float hitX = 0.0f;
                    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
                    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
                    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
                    canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
                }

                // 開始アクター破棄
                int startIdx = endNote.pairIndex;
                if (startIdx >= 0 && startIdx < (int)noteActors.size()) {
                    if (auto* act = noteActors[startIdx].Target()) {
                        act->DeActivate();
                        act->Destroy();
                    }
                }

                // チェーンホールド処理
                bool foundChain = false;
                if (lane >= 0 && lane < (int)laneOrder.size()) {
                    auto& order = laneOrder[lane];
                    for (size_t k = laneHeads[lane]; k < order.size(); ++k) {
                        int nextIdx = order[k];
                        auto& nextNote = noteSequence[nextIdx];

                        if (nextNote.judged) continue;
                        if (nextNote.type == NoteType::HoldEnd) continue;

                        if (nextNote.type == NoteType::HoldStart) {
                            float diff = std::abs(nextNote.hittime - songTime);

                            if (diff <= K_JUDGE_GOOD) {
                                nextNote.judged = true;
                                nextNote.result = JudgeResult::Perfect;
                                GetCurrentSession().AddResult(JudgeResult::Perfect);
                                UpdateCombo(JudgeResult::Perfect);

                                if (auto* sound = actorRef.Target()->GetComponent<SoundComponent>()) {
                                    sound->Play(GetHitSfxPath(NoteType::Tap));
                                }
                                if (auto* canvas = actorRef.Target()->GetComponent<IngameCanvas>()) {
                                    float uiLaneWidth = 300.0f;
                                    float sideOffset = 250.0f;
                                    float hitX = 0.0f;
                                    if (lane <= 3) hitX = (lane - 1.5f) * uiLaneWidth;
                                    else if (lane == 4) hitX = (-1.5f * uiLaneWidth) - sideOffset;
                                    else if (lane == 5) hitX = (1.5f * uiLaneWidth) + sideOffset;
                                    canvas->SpawnHitEffect(hitX, -265.0f, (uiLaneWidth / 100.0f) * 1.5f, 0.4f, { 1,1,1,1 });
                                }

                                if (nextNote.pairIndex != -1) {
                                    activeHolds[lane] = nextNote.pairIndex;
                                    activeHoldNextBeats[lane] = nextNote.absBeat + 0.5;
                                    foundChain = true;
                                }
                            }
                        }
                        break;
                    }
                }

                if (!foundChain) {
                    activeHolds[lane] = -1;
                }
            }
        } else {
            // リリース時：ビートを進めてコンボ悪用を防ぐ
            double curBeat = SecondsToBeat(songTime);
            while (curBeat >= activeHoldNextBeats[lane]) {
                activeHoldNextBeats[lane] += 0.5;
            }
        }
    }

    // ============================================================
    // 時間変換
    // TempoServiceに委譲
    // ============================================================
    double NoteManager::SecondsToBeat(double seconds) const {
        return tempoService.SecondsToBeat(seconds, tempoMap);
    }

    // ============================================================
    // インスタンシング初期化
    // NoteInstanceRendererに委譲
    // ============================================================
    void NoteManager::InitInstancing()
    {
        noteRenderer.Init();
        m_instanceDataCPU.reserve(2048);
    }

    // ============================================================
    // インスタンスバッファ更新
    // 可視なタップノーツのワールド行列を収集
    // ============================================================
    void NoteManager::UpdateInstanceBuffer()
    {
        m_instanceDataCPU.clear();

        // スポーン済みノーツのみチェック
        size_t maxIdx = std::min(nextIndex, noteActors.size());
        for (size_t i = 0; i < maxIdx; ++i) {
            if (i >= noteSequence.size()) continue;
            
            // 判定済みはスキップ（破棄済み）
            if (noteSequence[i].judged) continue;

            // ホールド・スキル・終了ノーツは個別描画
            if (noteSequence[i].type == NoteType::HoldStart ||
                noteSequence[i].type == NoteType::HoldEnd ||
                noteSequence[i].type == NoteType::Skill ||
                noteSequence[i].type == NoteType::SongEnd) continue;

            // 無効なアクターはスキップ
            if (noteActors[i].IsNull()) continue;
            if (!noteActors[i].IsValid()) continue;

            auto act = noteActors[i].Target();
            if (!act) continue;

            // ワールド行列を収集
            NoteInstanceData data;
            DirectX::XMMATRIX m = act->transform.Matrix();
            DirectX::XMStoreFloat4x4(&data.world, m);
            data.color = { 1, 1, 1, 1 };
            m_instanceDataCPU.push_back(data);
        }

        // NoteInstanceRendererにバッファ更新を委譲
        noteRenderer.UpdateBuffer(m_instanceDataCPU);
    }

    // ============================================================
    // インスタンス描画
    // NoteInstanceRendererに委譲
    // ============================================================
    void NoteManager::DrawInstanced()
    {
        UpdateInstanceBuffer();
        if (m_instanceDataCPU.empty()) return;
        noteRenderer.Draw(m_instanceDataCPU.size());
    }

} // namespace app::test
