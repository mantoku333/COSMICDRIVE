#include "ComboManager.h"
#include "GameSession.h"

namespace app::test {

    ComboManager::ComboManager() 
        : currentCombo(0), maxTotalCombo(0) {
    }

    void ComboManager::UpdateCombo(JudgeResult result) {
        switch (result) {
        case JudgeResult::Perfect:
        case JudgeResult::Great:
        case JudgeResult::Good:
            ++currentCombo;
            break;
        case JudgeResult::Miss:
            currentCombo = 0;
            break;
        default:
            break;
        }
    }

    void ComboManager::AddCombo(int amount) {
        currentCombo += amount;
        GetCurrentSession().AddCombo(amount);
    }

    int ComboManager::GetCurrentCombo() const {
        return currentCombo;
    }

    void ComboManager::Reset() {
        currentCombo = 0;
    }

    int ComboManager::GetMaxTotalCombo() const {
        return maxTotalCombo;
    }

    void ComboManager::SetMaxTotalCombo(int max) {
        maxTotalCombo = max;
    }

}
