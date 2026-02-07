#include "JudgeStatsService.h"
#include "GameSession.h"

// JudgeStatsService は GameSession への委譲ラッパーとして機能する
// これにより既存の呼び出しコードを変更せずに GameSession に移行できる

namespace app::test {

    void JudgeStatsService::Reset()
    {
        GetCurrentSession().Reset();
    }

    void JudgeStatsService::AddFast() { GetCurrentSession().AddFast(); }
    void JudgeStatsService::AddSlow() { GetCurrentSession().AddSlow(); }

    int JudgeStatsService::GetFastCount() { return GetCurrentSession().GetFastCount(); }
    int JudgeStatsService::GetSlowCount() { return GetCurrentSession().GetSlowCount(); }

    void JudgeStatsService::AddResult(JudgeResult result)
    {
        GetCurrentSession().AddResult(result);
    }

    void JudgeStatsService::AddCombo(int amount)
    {
        GetCurrentSession().AddCombo(amount);
    }

    int JudgeStatsService::GetCount(JudgeResult result)
    {
        return GetCurrentSession().GetCount(result);
    }

    int JudgeStatsService::GetCombo()
    {
        return GetCurrentSession().GetCombo();
    }

    int JudgeStatsService::GetMaxCombo()
    {
        return GetCurrentSession().GetMaxCombo();
    }

    JudgeResult JudgeStatsService::GetLastResult()
    {
        return GetCurrentSession().GetLastResult();
    }

    void JudgeStatsService::SetChartPath(const std::string& path)
    {
        GetCurrentSession().SetChartPath(path);
    }

    std::string JudgeStatsService::GetChartPath()
    {
        return GetCurrentSession().GetChartPath();
    }

    void JudgeStatsService::SetDifficulty(int diff)
    {
        GetCurrentSession().SetDifficulty(diff);
    }

    int JudgeStatsService::GetDifficulty()
    {
        return GetCurrentSession().GetDifficulty();
    }

    void JudgeStatsService::SetTitle(const std::string& titleStr)
    {
        GetCurrentSession().SetTitle(titleStr);
    }

    std::string JudgeStatsService::GetTitle()
    {
        return GetCurrentSession().GetTitle();
    }

}