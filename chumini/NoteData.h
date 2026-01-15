#pragma once
#include <cstdint>

// lane, measure, beat, tick16, type
namespace app::test {
    // ノーツ種類
    enum class NoteType : uint8_t {
        Tap,        // タップ
		HoldStart,  // ホールド開始
        HoldEnd,    // ホールド終了
		//ExTap       // エクストラタップ 

		SongEnd,    // 曲終了判定につかう
    };

    // 判定結果
    enum class JudgeResult : uint8_t {
        None,       // 未判定
        Perfect,    // パーフェクト
        Great,      // グレート
        Good,       // グッド
        Miss,       // ミス
        Skip        // スキップ
    };

    // Measure（小節） / Beat（拍） / Tick（拍内のズレ） 16分まで
    struct Mbt16 {
        int measure = 0;  // 小節(0始まり)
        int beat = 0;     // 拍(0..3)
        int tick16 = 0;   // 16分(0..3)
    };

    // 読み込み先の譜面データ
    struct NoteData {

        // 譜面情報
        int      lane;        // レーン番号 ( 0 ～ LANE_COUNT-1 )
        NoteType type;        // ノーツ種類
        
        // 時間
        float    hittime;     // ノーツが降ってくる時間（秒）
        float    leadTime;    // ノーツが判定線に到達するまでの時間 ハイスピによって変化

        // MBT補助
		Mbt16 mbt{};		   // 小節、拍、16分
        double absBeat = 0.0;  // 通し拍 (measure*4 + beat + tick16/4.0)

        // 判定
        bool judged = false;  // ノーツが判定済みかどうかのsフラグ
        JudgeResult result = JudgeResult::None;
		
        // Hold Note Info
        int pairIndex = -1;   // Index of the paired HoldStart or HoldEnd
        float duration = 0.0f; // Duration in seconds (for HoldStart)
    };
}
