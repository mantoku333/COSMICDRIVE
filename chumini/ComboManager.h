#pragma once
#include "NoteData.h"

namespace app::test {
    class ComboManager {
    public:
        // コンストラクタ
        ComboManager();

        /// <summary>
        /// 判定結果に応じてコンボ数を更新
        /// Perfect/Great/Good: コンボ加算
        /// Miss: コンボリセット
        /// </summary>
        /// <param name="result">判定結果</param>
        void UpdateCombo(JudgeResult result);

        /// <summary>
        /// コンボ数を直接加算（ホールドの途中などで使用）
        /// </summary>
        /// <param name="amount">加算量</param>
        void AddCombo(int amount);

        /// <summary>
        /// 現在のコンボ数を取得
        /// </summary>
        int GetCurrentCombo() const;

        /// <summary>
        /// コンボ数をリセット
        /// </summary>
        void Reset();

        /// <summary>
        /// 最大コンボ数を取得
        /// </summary>
        int GetMaxTotalCombo() const;

        /// <summary>
        /// 最大コンボ数を設定
        /// </summary>
        void SetMaxTotalCombo(int max);

    private:
        int currentCombo = 0;
        int maxTotalCombo = 0;
    };
}
