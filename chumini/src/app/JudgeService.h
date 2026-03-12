#pragma once
#include "NoteData.h"
#include <vector>

namespace app::test {

    // ============================================================
    // 判定結果の詳細情報
    // ============================================================
    struct JugdeAction {
        int noteIndex = -1;         // 判定されたノーツのインデックス
        JudgeResult result = JudgeResult::None; // 判定結果
        float timeDiff = 0.0f;      // 判定時間差（入力 - ヒット時間）
        bool isSkill = false;       // スキルノーツかどうか
    };

    // ============================================================
    // 判定候補ノーツ情報
    // ============================================================
    struct JudgeCandidate {
        int noteIndex;          // 元のノーツ配列でのインデックス
        NoteType type;          // ノーツタイプ
        float hittime;          // 理想ヒット時間
        float zPosition;        // 現在のZ座標
        
        // 必要に応じて追加
    };

    // ============================================================
    // JudgeService - 判定ロジック管理
    // ============================================================
    class JudgeService {
    public:
        /// 単一ノーツの時間判定を行う
        JudgeResult JudgeNote(NoteType type, float noteTime, float inputTime);

        /// 候補リストの中から最適な判定対象を探して判定する
        /// @param inputTime 入力時間
        /// @param candidates 判定候補ノーツのリスト（手前から奥の順）
        /// @param judgeZ 判定ラインのZ座標
        /// @return 判定アクション（判定されなかった場合はResult=Skip）
        JugdeAction JudgeCandidates(
            float inputTime,
            const std::vector<JudgeCandidate>& candidates,
            float judgeZ
        );
    };

} // namespace app::test
