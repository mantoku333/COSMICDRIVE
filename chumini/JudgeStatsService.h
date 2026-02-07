#pragma once
#include "NoteData.h"
#include <string>

namespace app::test
{
    /// <summary>
    /// 判定統計サービス（GameSessionへの委譲ラッパー）
    /// 
    /// 後方互換性のために維持。
    /// 新規コードでは GetCurrentSession() または GameSession& を直接使用することを推奨。
    /// </summary>
    class JudgeStatsService
    {
    public:
        static void Reset();
        static void AddResult(JudgeResult result);
        static void AddCombo(int amount);
        static void AddFast();
        static void AddSlow();

        static int GetCount(JudgeResult result);
        static int GetFastCount();
        static int GetSlowCount();
        static int GetCombo();
        static int GetMaxCombo();
        static JudgeResult GetLastResult();

        static void SetChartPath(const std::string& path);
        static std::string GetChartPath();

        static void SetDifficulty(int diff);
        static int GetDifficulty();

        static void SetTitle(const std::string& titleStr);
        static std::string GetTitle();
    };
}
