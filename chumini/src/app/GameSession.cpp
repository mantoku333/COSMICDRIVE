#include "GameSession.h"
#include <algorithm>

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

    // ==========================================
    // ・判定/スコア計算
    // ==========================================
    int GameSession::CalculateScore() const
    {
        if (totalNoteCount <= 0) return 0;

        double currentSum = perfect * 1.0 + great * 0.8 + good * 0.5;
        double maxPossible = (double)totalNoteCount;

        int score = (int)((currentSum / maxPossible) * 1000000.0);
        if (score > 1000000) score = 1000000;
        
        return score;
    }

    GameSession::Rank GameSession::GetRank() const
    {
        int score = CalculateScore();
        if (score >= 1000000) return GameSession::Rank::SSS;
        if (score >= 900000) return GameSession::Rank::SS;
        if (score >= 800000) return GameSession::Rank::S;
        if (score >= 600000) return GameSession::Rank::A;
        if (score >= 400000) return GameSession::Rank::B;
        return GameSession::Rank::C;
    }

    void GameSession::SetTotalNoteCount(int count)
    {
        totalNoteCount = count;
        if (totalNoteCount <= 0) totalNoteCount = 1;
    }
    
    // ==========================================
    // ・メタデータ
    // ==========================================
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

    // Global Accessor
    // Global Accessor
    static GameSession g_defaultSession;
    static GameSession* g_currentSessionPtr = &g_defaultSession;

    GameSession& GetCurrentSession() { 
        return *g_currentSessionPtr; 
    }

    void SetCurrentSession(GameSession* session) { 
        g_currentSessionPtr = session ? session : &g_defaultSession; 
    }
}
