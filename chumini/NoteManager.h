#pragma once
#include "Component.h"
#include "NoteData.h"
#include <vector>
#include <string>   // ← 追加（judgeResultToStringで使用）
#include "App.h"
#include "SceneChangeComponent.h"

// ===================== 判定可視・調整用 定数 =====================

// 判定範囲（見た目のライン近傍判定に使うソフト制約）
constexpr float judgeRange = 1.5f;

// 判定窓（秒）? プロジェクト既存の名称を維持
constexpr float JUDGE_PERFECT = 0.08f;
constexpr float JUDGE_GREAT = 0.3f;
constexpr float JUDGE_GOOD = 0.5f;

// 実装側（cpp）で使う別名（変更しやすいようにエイリアス化）
inline constexpr float J_WIN_PERFECT = JUDGE_PERFECT;
inline constexpr float J_WIN_GREAT = JUDGE_GREAT;
inline constexpr float J_WIN_GOOD = JUDGE_GOOD;

// 同時押し・高速トリル耐性のためのチューニング
//  - LOOKAHEAD：先頭から何件先まで候補にするか
//  - EARLY_BIAS：未来ノーツに微ペナルティ（“早い側”優先のバイアス）
inline constexpr int   J_LOOKAHEAD_NOTES = 5;       // 2?6で調整
inline constexpr float J_ASSIGN_EARLY_BIAS = 0.005f;  // 0?0.01くらいで調整
inline constexpr float J_POS_TOL_MULT = 2.0f;    // 位置許容の緩和倍率（×2?3推奨）

namespace app::test {

    class NoteManager : public sf::Component {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);

        // ノーツを画面上部（startY）に生成するリードタイム（秒）
        float leadTime = 0;
        int   currentCombo = 0;

        // ノーツ速度（= basenoteSpeed * HiSpeed）
        const float basenoteSpeed = 1.0f;  // 基本速度
        const float HiSpeed = 18.0f;  // 倍率
        float       noteSpeed = basenoteSpeed * HiSpeed;

        // 判定API（単レーン）
        JudgeResult JudgeNote(NoteType type, float noteTime, float inputTime);
        JudgeResult JudgeLane(int lane, float inputTime);

        // 判定API（同時押し一括）? 追加
        // 同一フレームで押下されたレーン一覧を渡すと、それぞれ最適なノーツに割り当てて判定
        void JudgeBatch(const std::vector<int>& pressedLanes, float inputTime);

        // デバッグ表示
        std::string judgeResultToString(app::test::JudgeResult result) {
            switch (result) {
            case app::test::JudgeResult::None:    return "None";
            case app::test::JudgeResult::Perfect: return "Perfect";
            case app::test::JudgeResult::Great:   return "Great";
            case app::test::JudgeResult::Good:    return "Good";
            case app::test::JudgeResult::Miss:    return "Miss";
            default:                              return "Unknown";
            }
        }

        float GetSongTime() const { return songTime; }
        void SetCurrentSongTime(float time);
        int   GetCurrentCombo() const;

        //シーンからレーンのパラメータを取得
        void SetLaneParams(
            const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
            float laneW_, float laneH_, float rotX_,
            float baseY_, float barRatio_,
            float sideLeftX_, float sideRightX_);

        void StartGame();

        float sideLaneX_Left = -2.5f;  
        float sideLaneX_Right = 2.5f;
       
    private:
        sf::SafePtr<sf::IScene> resultScene;

        bool isPlaying = false;

        // 曲のオフセット（譜面ファイルの #OFFSET で設定される）
        double noteOffset = 0.0;

        // IngameScene から受け取るレーン情報
        std::vector<sf::ref::Ref<sf::Actor>> laneRefs;

        float laneW;     // レーン幅
        float laneH;     // 奥行き
        float rotX;      // 傾斜角
        float baseY;     // 高さ基準
        float barRatio;  // バーの比率位置
        float judgeZ;    // 判定Z位置

        // 更新コマンド
        sf::command::Command<> updateCommand;

        void UpdateCombo(JudgeResult result);
        void CheckMissedNotes();

        sf::geometry::GeometryCube g_cube;

        // すべてのノーツ（時間順に並べ替え後の実体）
        std::vector<NoteData> noteSequence;

        // ノーツのアクタ参照（noteSequenceと同じインデックス対応）
        std::vector<sf::ref::Ref<sf::Actor>> noteActors;

        // 次にアクティブ化するノーツの全体インデックス
        size_t nextIndex = 0;

        // 曲再生開始からの経過時間
        float songTime = 0.f;

        // レーンごとの時間順インデックス配列と、その先頭ポインタ
        // size = lanes, 各要素は noteSequence のインデックス
        std::vector<std::vector<int>> laneOrder;
        std::vector<size_t>           laneHeads;

        sf::SafePtr<SceneChangeComponent> sceneChanger;

    };

} // namespace app::test
