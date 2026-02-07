#pragma once
#include "Component.h"
#include "NoteData.h"
#include <vector>
#include <string>
#include "App.h"
#include "SceneChangeComponent.h"
#include "TempoService.h"
#include "ChartLoader.h"
#include "JudgeService.h"
#include "JudgeConstants.h"
#include "ComboManager.h"

namespace app::test {

    // TempoEvent は TempoService.h にて定義済み

    // ============================================================
    // NoteManager - ノーツ管理クラス
    // 
    // 役割:
    // - 譜面ファイル（.sus/.chart）の読み込みと解析
    // - ノーツの生成とアクティベート管理
    // - プレイヤー入力に対する判定処理
    // - ホールドノーツの継続判定
    // - ミスノーツの検出と処理
    // - インスタンシング描画による高速レンダリング
    // ============================================================
    class NoteManager : public sf::Component {
    public:
        /// コンポーネント初期化（譜面読み込み・ノーツ生成）
        void Begin() override;
        
        /// 毎フレーム更新（ノーツのアクティベート・ミスチェック）
        void Update(const sf::command::ICommand&);

        // --------------------------------------------
        // 公開メンバー変数
        // --------------------------------------------
        
        /// ノーツが画面上部に出現してから判定ラインに到達するまでの時間（秒）
        float leadTime = 0;
        


        /// ノーツ速度関連
        const float basenoteSpeed = 1.0f;  ///< 基本速度
        float HiSpeed = 18.0f;             ///< ハイスピード倍率
        float noteSpeed = basenoteSpeed * HiSpeed;  ///< 実際の速度

        /// サイドレーン（レーン4, 5）のX座標
        float sideLaneX_Left = -2.5f;
        float sideLaneX_Right = 2.5f;

        // --------------------------------------------
        // 判定API
        // --------------------------------------------
        
        /// 単一ノーツの時間判定
        /// @param type ノーツタイプ（HoldStartは判定緩和あり）
        /// @param noteTime ノーツのヒット予定時間
        /// @param inputTime プレイヤーの入力時間
        /// @return 判定結果（Perfect/Great/Good/Miss）
        JudgeResult JudgeNote(NoteType type, float noteTime, float inputTime);
        
        /// 指定レーンの判定処理
        /// 先読み範囲内から最適なノーツを探して判定
        /// @param lane 判定するレーン番号（0-5）
        /// @param inputTime 入力時間
        /// @return 判定結果
        JudgeResult JudgeLane(int lane, float inputTime);
        
        /// 複数レーン同時判定（同時押し対応）
        void JudgeBatch(const std::vector<int>& pressedLanes, float inputTime);

        // --------------------------------------------
        // ホールドノーツ処理
        // --------------------------------------------
        
        /// ホールドノーツの継続判定
        /// 押し続けている間、一定間隔でコンボを加算
        /// @param lane レーン番号
        /// @param isPressed 現在押されているか
        void CheckHold(int lane, bool isPressed);
        
        /// 各レーンでアクティブなホールドの終端インデックス（-1なら非アクティブ）
        std::vector<int> activeHolds;
        
        /// 各レーンでコンボ加算用の次のビート
        std::vector<double> activeHoldNextBeats;

        // --------------------------------------------
        // 時間・状態取得
        // --------------------------------------------
        float GetSongTime() const { return songTime; }
        void SetCurrentSongTime(float time);
        int GetCurrentCombo() const;
        void AddCombo(int amount);
        
        /// 最大コンボ数（譜面から事前計算）
        int GetMaxTotalCombo() const;

        // --------------------------------------------
        // 初期化・設定
        // --------------------------------------------
        
        /// シーンからレーンパラメータを受け取る
        void SetLaneParams(
            const std::vector<sf::ref::Ref<sf::Actor>>& lanes,
            float laneW_, float laneH_, float rotX_,
            float baseY_, float barRatio_,
            float sideLeftX_, float sideRightX_);
        
        /// ゲーム開始（更新処理を有効化）
        void StartGame();
        
        /// BGMとの時間同期
        void SyncTime(float time);

        // --------------------------------------------
        // インスタンシング描画
        // タップノーツを一括描画することでDrawCall削減
        // --------------------------------------------
        void InitInstancing();
        void UpdateInstanceBuffer();
        void DrawInstanced();

        // --------------------------------------------
        // ユーティリティ
        // --------------------------------------------
        
        /// 秒数を拍数に変換（BPM変化対応）
        double SecondsToBeat(double seconds) const;
        
        /// デバッグ用：全ノーツをPerfectとして強制終了
        void DebugForceComplete();

        /// 判定結果を文字列に変換（デバッグ用）
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
        
        /// インスタンシング用のノーツデータ
        struct NoteInstanceData {
            DirectX::XMFLOAT4X4 world;  ///< ワールド変換行列
            DirectX::XMFLOAT4   color;  ///< 色
        };

        // --------------------------------------------
        // 状態管理
        // --------------------------------------------
        bool isPlaying = false;      ///< ゲーム中かどうか
        float songTime = 0.f;        ///< 曲の経過時間
        size_t nextIndex = 0;        ///< 次にアクティベートするノーツのインデックス
        int songEndIndex = -1;       ///< SongEndノーツのインデックス（キャッシュ）
        int missCheckLane = 0;       ///< ミスチェック用レーン（分散処理用）
        double noteOffset = 0.0;     ///< 譜面のオフセット

        // --------------------------------------------
        // 譜面データ
        // --------------------------------------------
        std::vector<TempoEvent> tempoMap;  ///< BPM変化イベントリスト
        std::vector<NoteData> noteSequence;  ///< 全ノーツデータ（時間順）
        std::vector<sf::ref::Ref<sf::Actor>> noteActors;  ///< ノーツのアクター参照

        // --------------------------------------------
        // レーン管理
        // --------------------------------------------
        std::vector<sf::ref::Ref<sf::Actor>> laneRefs;  ///< レーンアクター参照
        std::vector<std::vector<int>> laneOrder;  ///< レーンごとのノーツインデックス
        std::vector<size_t> laneHeads;  ///< 各レーンの先頭ポインタ

        float laneW;      ///< レーン幅
        float laneH;      ///< レーン奥行き
        float rotX;       ///< レーン傾斜角
        float baseY;      ///< レーン高さ基準
        float barRatio;   ///< 判定バーの位置比率
        float judgeZ;     ///< 判定ラインのZ座標

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

        /// マウススラッシュ用（スキルノーツ判定）
        POINT lastCursorPos = { 0, 0 };
        float mouseSpeed = 0.0f;

        /// テンポ管理サービス
        TempoService tempoService;

        /// 譜面読み込みサービス
        ChartLoader chartLoader;

        /// 判定ロジックサービス
        JudgeService judgeService;

        /// コンボ管理サービス
        ComboManager comboManager;

        // --------------------------------------------
        // 内部メソッド
        // --------------------------------------------
        
        /// 判定結果に応じてコンボを更新
        void UpdateCombo(JudgeResult result);
        
        /// ミスしたノーツを検出して処理
        void CheckMissedNotes();
    };

} // namespace app::test
