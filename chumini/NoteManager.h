#pragma once
#include "Component.h"
#include "NoteData.h"
#include <vector>
#include <string>
#include "App.h"
#include "SceneChangeComponent.h"

// ============================================================
// 判定関連の定数
// ============================================================

// 判定範囲（Z座標の許容距離）
constexpr float judgeRange = 1.5f;

// 判定ウィンドウ（秒）
constexpr float JUDGE_PERFECT = 0.08f;
constexpr float JUDGE_GREAT = 0.3f;
constexpr float JUDGE_GOOD = 0.5f;

// 判定ウィンドウのエイリアス
inline constexpr float J_WIN_PERFECT = JUDGE_PERFECT;
inline constexpr float J_WIN_GREAT = JUDGE_GREAT;
inline constexpr float J_WIN_GOOD = JUDGE_GOOD;

// 同時押し・トリル対応のパラメータ
inline constexpr int   J_LOOKAHEAD_NOTES = 5;      // 先読みノーツ数
inline constexpr float J_ASSIGN_EARLY_BIAS = 0.005f; // 早押しバイアス
inline constexpr float J_POS_TOL_MULT = 2.0f;      // 位置許容倍率

namespace app::test {

    // ============================================================
    // BPM変化用の構造体
    // ============================================================
    struct TempoEvent {
        double atBeat = 0.0;  // 開始拍
        double bpm = 120.0;   // テンポ
        double spb = 0.5;     // 1拍あたりの秒数
        double atSec = 0.0;   // 開始秒数
    };

    // ============================================================
    // ノーツ管理クラス
    // ============================================================
    class NoteManager : public sf::Component {
    public:
        void Begin() override;
        void Update(const sf::command::ICommand&);

        // --------------------------------------------
        // 公開メンバー変数
        // --------------------------------------------
        float leadTime = 0;       // ノーツ生成のリードタイム
        int   currentCombo = 0;   // 現在のコンボ数

        // ノーツ速度
        const float basenoteSpeed = 1.0f;
        float HiSpeed = 18.0f;
        float noteSpeed = basenoteSpeed * HiSpeed;

        // サイドレーンのX座標
        float sideLaneX_Left = -2.5f;
        float sideLaneX_Right = 2.5f;

        // --------------------------------------------
        // 判定API
        // --------------------------------------------
        JudgeResult JudgeNote(NoteType type, float noteTime, float inputTime);
        JudgeResult JudgeLane(int lane, float inputTime);
        void JudgeBatch(const std::vector<int>& pressedLanes, float inputTime);

        // --------------------------------------------
        // ホールドノーツ処理
        // --------------------------------------------
        void CheckHold(int lane, bool isPressed);
        std::vector<int> activeHolds;          // 各レーンのアクティブなホールド終端インデックス
        std::vector<double> activeHoldNextBeats; // コンボ加算用の次のビート

        // --------------------------------------------
        // 時間・状態取得
        // --------------------------------------------
        float GetSongTime() const { return songTime; }
        void SetCurrentSongTime(float time);
        int GetCurrentCombo() const;
        void AddCombo(int amount);
        int GetMaxTotalCombo() const { return maxTotalCombo; }

        // --------------------------------------------
        // 初期化・設定
        // --------------------------------------------
        void SetLaneParams(
            const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
            float laneW_, float laneH_, float rotX_,
            float baseY_, float barRatio_,
            float sideLeftX_, float sideRightX_);
        void StartGame();
        void SyncTime(float time);

        // --------------------------------------------
        // インスタンシング描画
        // --------------------------------------------
        void InitInstancing();
        void UpdateInstanceBuffer();
        void DrawInstanced();

        // --------------------------------------------
        // ユーティリティ
        // --------------------------------------------
        double SecondsToBeat(double seconds) const;
        void DebugForceComplete();

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

    private:
        // --------------------------------------------
        // 内部データ構造
        // --------------------------------------------
        struct NoteInstanceData {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4   color;
        };

        // --------------------------------------------
        // 状態管理
        // --------------------------------------------
        bool isPlaying = false;
        float songTime = 0.f;
        size_t nextIndex = 0;
        int maxTotalCombo = 0;
        int songEndIndex = -1;
        int missCheckLane = 0;
        double noteOffset = 0.0;

        // --------------------------------------------
        // 譜面データ
        // --------------------------------------------
        std::vector<TempoEvent> tempoMap;
        std::vector<NoteData> noteSequence;
        std::vector<sf::ref::Ref<sf::Actor>> noteActors;

        // --------------------------------------------
        // レーン管理
        // --------------------------------------------
        std::vector<sf::ref::Ref<sf::Actor>> laneRefs;
        std::vector<std::vector<int>> laneOrder;
        std::vector<size_t> laneHeads;

        float laneW;
        float laneH;
        float rotX;
        float baseY;
        float barRatio;
        float judgeZ;

        // --------------------------------------------
        // インスタンシング用リソース
        // --------------------------------------------
        ID3D11Buffer* m_instanceBuffer = nullptr;
        std::vector<NoteInstanceData> m_instanceDataCPU;
        ID3D11VertexShader* m_vs = nullptr;
        ID3D11InputLayout* m_layout = nullptr;
        ID3D11Buffer* m_cubeVB = nullptr;
        ID3D11Buffer* m_cubeIB = nullptr;
        int m_cubeIndexCount = 0;

        // --------------------------------------------
        // その他
        // --------------------------------------------
        sf::SafePtr<sf::IScene> resultScene;
        sf::SafePtr<SceneChangeComponent> sceneChanger;
        sf::command::Command<> updateCommand;
        sf::geometry::GeometryCube g_cube;

        // マウススラッシュ用
        POINT lastCursorPos = { 0, 0 };
        float mouseSpeed = 0.0f;

        // --------------------------------------------
        // 内部メソッド
        // --------------------------------------------
        void UpdateCombo(JudgeResult result);
        void CheckMissedNotes();
    };

} // namespace app::test
