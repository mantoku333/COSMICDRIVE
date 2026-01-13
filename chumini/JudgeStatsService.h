#pragma once
#include "NoteData.h"

namespace app::test
{
    class JudgeStatsService
    {
    public:
        static void Reset();
        static void AddResult(JudgeResult result);
        static void AddFast();
        static void AddSlow();

        static int GetCount(JudgeResult result);
        static int GetFastCount();
        static int GetSlowCount();
        static int GetCombo();
        static int GetMaxCombo();
        static JudgeResult GetLastResult();

    private:
        static inline int perfect = 0;
        static inline int great = 0;
        static inline int good = 0;
        static inline int miss = 0;
        static inline int fast = 0;
        static inline int slow = 0;

        static inline int combo = 0;
        static inline int maxCombo = 0;
        static inline JudgeResult lastResult = JudgeResult::None;
    };
}

