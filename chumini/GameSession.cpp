#include "GameSession.h"

namespace app::test {

    void GameSession::Reset()
    {
        perfect = great = good = miss = 0;
        fast = slow = 0;
        combo = maxCombo = 0;
        lastResult = JudgeResult::None;
        // メタデータはリセットしない（プレイ中に参照するため）
    }

    void GameSession::AddFast() { ++fast; }
    void GameSession::AddSlow() { ++slow; }

    int GameSession::GetFastCount() const { return fast; }
    int GameSession::GetSlowCount() const { return slow; }

    void GameSession::AddResult(JudgeResult result)
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

    void GameSession::AddCombo(int amount) {
        combo += amount;
        if (combo > maxCombo) maxCombo = combo;
    }

    int GameSession::GetCount(JudgeResult result) const
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

    int GameSession::GetCombo() const
    {
        return combo;
    }

    int GameSession::GetMaxCombo() const
    {
        return maxCombo;
    }

    JudgeResult GameSession::GetLastResult() const
    {
        return lastResult;
    }

    void GameSession::SetChartPath(const std::string& path)
    {
        chartPath = path;
    }

    const std::string& GameSession::GetChartPath() const
    {
        return chartPath;
    }

    void GameSession::SetDifficulty(int diff)
    {
        difficulty = diff;
    }

    int GameSession::GetDifficulty() const
    {
        return difficulty;
    }

    void GameSession::SetTitle(const std::string& titleStr)
    {
        title = titleStr;
    }

    const std::string& GameSession::GetTitle() const
    {
        return title;
    }

    // グローバルGameSessionインスタンス（移行期間用）
    static GameSession g_currentSession;

    GameSession& GetCurrentSession()
    {
        return g_currentSession;
    }

}
