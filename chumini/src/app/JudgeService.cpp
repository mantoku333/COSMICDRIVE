#include "JudgeService.h"
#include "JudgeConstants.h"
#include <cmath>
#include <algorithm>

namespace app::test {

    // ============================================================
    // 単一ノーツの時間判定
    // ============================================================
    JudgeResult JudgeService::JudgeNote(NoteType type, float noteTime, float inputTime) {
        // 入力オフセットは呼び出し元で適用済みとする、またはここで単純差分を取る
        // NoteManager::JudgeNoteでは inputTime += INPUT_OFFSET_SEC していたが、
        // 呼び出し元で統一したtimeを渡す方針にする。
        
        float diff = std::abs(noteTime - inputTime);

        // HoldStartは判定ウィンドウを広げる（ホールドは開始が重要）
        if (type == NoteType::HoldStart) {
            if (diff <= K_JUDGE_PERFECT * 2.0f) return JudgeResult::Perfect;
            if (diff <= K_JUDGE_GREAT * 1.5f)   return JudgeResult::Great;
            if (diff <= K_JUDGE_GOOD * 1.5f)    return JudgeResult::Good;
        } else {
            // 通常ノーツの判定
            if (diff <= K_JUDGE_PERFECT) return JudgeResult::Perfect;
            if (diff <= K_JUDGE_GREAT)   return JudgeResult::Great;
            if (diff <= K_JUDGE_GOOD)    return JudgeResult::Good;
        }
        return JudgeResult::Miss;
    }

    // ============================================================
    // 判定候補リストに対する判定
    // ============================================================
    JugdeAction JudgeService::JudgeCandidates(
        float inputTime,
        const std::vector<JudgeCandidate>& candidates,
        float judgeZ
    ) {
        JugdeAction action;
        action.result = JudgeResult::Skip;

        for (const auto& candidate : candidates) {
            
            // HoldEnd, Skill はタップ判定しない（呼び出し元でフィルタリングしても良いが念のため）
            if (candidate.type == NoteType::HoldEnd ||
                candidate.type == NoteType::Skill) {
                continue;
            }

            // NoteType::HoldStart の補正は呼び出し元で行われているか、
            // judgeZ と candidate.zPosition の関係で考慮される。
            // ここでは純粋に JudgeNote と位置判定を行う。

            JudgeResult res = JudgeNote(candidate.type, candidate.hittime, inputTime);

            if (res == JudgeResult::Miss) {
                float diff = inputTime - candidate.hittime;

                // 入力が遅すぎる（過去のノーツを叩こうとした）-> Good範囲より遅い -> 対象外（次の候補へ）
                if (diff > K_JUDGE_GOOD) {
                    continue;
                }
                // 入力が早すぎる（未来のノーツを叩こうとした）-> Good範囲より早い -> まだ判定圏外（終了、これ以上奥は見ない）
                // ただし、候補は手前から順に並んでいる前提
                if (diff < -K_JUDGE_GOOD) {
                    return action; // Skip
                }

                // 時間的には判定圏内だが、JudgeNoteでMissになった場合
                // Z座標判定を行う
                float diffZ = std::abs(candidate.zPosition - judgeZ);
                if (diffZ > K_JUDGE_RANGE * K_POS_TOL_MULT) {
                    if (diff < 0) return action; // 早すぎるケース（かつ遠い）→ 無視
                    else {
                        continue; // 通り過ぎたケース（かつ遠い）→ 次の候補へ
                    }
                }
            }

            // 判定成功
            action.result = res;
            action.noteIndex = candidate.noteIndex;
            action.timeDiff = inputTime - candidate.hittime;
            action.isSkill = (candidate.type == NoteType::Skill);
            return action;
        }

        return action; // Skip
    }

} // namespace app::test
