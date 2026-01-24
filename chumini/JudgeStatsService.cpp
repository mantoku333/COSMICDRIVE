#include "JudgeStatsService.h"

using namespace app::test;

void JudgeStatsService::Reset()
{
    perfect = great = good = miss = 0;
    fast = slow = 0;
    combo = maxCombo = 0;
    lastResult = JudgeResult::None;
}

void JudgeStatsService::AddFast() { ++fast; }
void JudgeStatsService::AddSlow() { ++slow; }

int JudgeStatsService::GetFastCount() { return fast; }
int JudgeStatsService::GetSlowCount() { return slow; }

void JudgeStatsService::AddResult(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::Perfect:
        ++perfect;
        ++combo;
        break;
    case JudgeResult::Great:
        ++great;
        ++combo;
        break;
    case JudgeResult::Good:
        ++good;
        ++combo;
        break;
    case JudgeResult::Miss:
        ++miss;
        combo = 0;
        break;
    default:
        break;
    }

    if (combo > maxCombo)
        maxCombo = combo;

    lastResult = result;
}

void JudgeStatsService::AddCombo(int amount) {
    combo += amount;
    if (combo > maxCombo) maxCombo = combo;
}

int JudgeStatsService::GetCount(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::Perfect: return perfect;
    case JudgeResult::Great:   return great;
    case JudgeResult::Good:    return good;
    case JudgeResult::Miss:    return miss;
    default:                   return 0;
    }
}

int JudgeStatsService::GetCombo()
{
    return combo;
}

int JudgeStatsService::GetMaxCombo()
{
    return maxCombo;
}

JudgeResult JudgeStatsService::GetLastResult()
{
    return lastResult;
}

void JudgeStatsService::SetChartPath(const std::string& path)
{
    chartPath = path;
}

std::string JudgeStatsService::GetChartPath()
{
    return chartPath;
}

void JudgeStatsService::SetDifficulty(int diff)
{
    difficulty = diff;
}

int JudgeStatsService::GetDifficulty()
{
    return difficulty;
}