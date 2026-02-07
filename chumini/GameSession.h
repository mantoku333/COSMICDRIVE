#pragma once
#include "NoteData.h"
#include <string>

namespace app::test
{
    /// <summary>
    /// ゲームセッション
    /// 
    /// 1プレイ分の判定結果・コンボ・メタデータを保持する。
    /// IngameSceneで生成し、NoteManager/IngameCanvas/ResultCanvasに渡す。
    /// </summary>
    class GameSession
    {
    public:
        GameSession() = default;

        /// <summary>
        /// セッションをリセット（プレイ開始時に呼ぶ）
        /// </summary>
        void Reset();

        // ===== 判定結果の追加 =====
        void AddResult(JudgeResult result);
        void AddCombo(int amount);
        void AddFast();
        void AddSlow();

        // ===== 判定結果の取得 =====
        int GetCount(JudgeResult result) const;
        int GetFastCount() const;
        int GetSlowCount() const;
        int GetCombo() const;
        int GetMaxCombo() const;
        JudgeResult GetLastResult() const;

        // ===== メタデータ =====
        void SetChartPath(const std::string& path);
        const std::string& GetChartPath() const;

        void SetDifficulty(int diff);
        int GetDifficulty() const;

        void SetTitle(const std::string& titleStr);
        const std::string& GetTitle() const;

    private:
        // 判定カウント
        int perfect = 0;
        int great = 0;
        int good = 0;
        int miss = 0;
        int fast = 0;
        int slow = 0;

        // コンボ
        int combo = 0;
        int maxCombo = 0;
        JudgeResult lastResult = JudgeResult::None;

        // メタデータ
        std::string chartPath;
        int difficulty = 0;
        std::string title;
    };

    /// <summary>
    /// グローバルGameSessionアクセサ（移行期間用）
    /// 
    /// 将来的にはDependency Injectionに置換予定
    /// </summary>
    GameSession& GetCurrentSession();

    /// <summary>
    /// 現在のアクティブなセッションを設定（IngameSceneから呼び出し）
    /// </summary>
    void SetCurrentSession(GameSession* session);
}
