#pragma once

namespace app::test {

    // ============================================================
    // 判定関連の定数
    // ============================================================

    /// 判定範囲（Z座標の許容距離）
    /// ノーツが判定ラインに近いかどうかを判定する際に使用
    constexpr float K_JUDGE_RANGE = 1.5f;

    /// 判定ウィンドウ（秒）
    /// Perfect: ±0.050秒、Great: ±0.080秒、Good: ±0.120秒
    constexpr float K_JUDGE_PERFECT = 0.050f;
    constexpr float K_JUDGE_GREAT = 0.080f;
    constexpr float K_JUDGE_GOOD = 0.120f;

    /// 同時押し・トリル対応のパラメータ
    /// LOOKAHEAD_NOTES: 先読みするノーツ数（同時押し時に複数候補から選択）
    /// ASSIGN_EARLY_BIAS: 早押しへの微小ペナルティ（早い側を優先）
    /// POS_TOL_MULT: 位置判定の許容倍率
    inline constexpr int   K_LOOKAHEAD_NOTES = 5;
    inline constexpr float K_ASSIGN_EARLY_BIAS = 0.005f;
    inline constexpr float K_POS_TOL_MULT = 2.0f;

    /// ミス判定スキップ用の定数（NoteManagerで使用していたもの）
    inline constexpr float K_MISS_THRESHOLD = -K_JUDGE_GOOD;

} // namespace app::test
