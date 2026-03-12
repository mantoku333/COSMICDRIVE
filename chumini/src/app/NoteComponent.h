#pragma once
#include "Component.h"
#include "NoteData.h"

namespace app::test { class NoteManager; }

namespace app::test {

    /// ノートの移動・表示を管理するコンポーネント
    /// レーン情報と傾斜パラメータに基づき、各フレームで座標を更新する
    class NoteComponent : public sf::Component {
    public:
        NoteComponent();

        void Begin() override;
        void Activate() override;
        void DeActivate() override;
        void Update(const sf::command::ICommand& command);

        NoteData info;           // ノートの基本データ（レーン・タイプ・判定時刻など）
        float leadTime = 0.f;    // ノートが出現してから判定ラインに到達するまでの時間
        float noteSpeed = 10.f;  // ノートのスクロール速度
        float spawnTime = 0.f;   // ノートの出現時刻
        float elapsed = 0.f;     // 経過時間

    private:
        sf::SafePtr<NoteManager> noteManager;  // ノートマネージャーへの参照

        sf::command::Command<> updateCommand;  // 更新コマンド

        // --- レーンレイアウト情報 ---
        int lanes = 0;        // レーン数
        float laneW = 0.f;    // レーン幅
        float laneH = 0.f;    // レーン奥行き
        float rotX = 0.f;     // X軸回転角度（度）
        float baseY = 0.f;    // レーンの基準Y座標
        float barRatio = 0.f; // 判定ラインの位置比率

        // --- 傾斜計算の事前計算値 ---
        float slopeRad = 0.f;  // 傾斜角度（ラジアン）
        float startZ = 0.f;   // 出現位置のZ座標（奥側）
        float endZ = 0.f;     // 判定ラインのZ座標（手前側）
        float startY = 0.f;   // 出現位置のY座標
        float endY = 0.f;     // 判定ラインのY座標

        // --- 状態フラグ ---
        bool isActive = false;            // ノートが有効化されているか
        bool skipFirstActivate = true;    // 初回のActivate呼び出しをスキップするフラグ
    };
}
