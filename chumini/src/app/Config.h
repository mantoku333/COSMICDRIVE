#pragma once
#include "SInput.h"
#include <string>

namespace app::test{

    struct AudioVolume {
        float master = 1.0f;  // 全体音量倍率
        float bgm = 1.0f;
        float tap = 1.0f;
        float emptyTap = 1.0f;
        /*float holdStart = 1.0f;
        float holdEnd = 1.0f;*/
    };

    struct KeyConfig {
        Key lane1 = Key::KEY_A;
        Key lane2 = Key::KEY_S;
        Key lane3 = Key::KEY_D;
        Key lane4 = Key::KEY_F;
    };

    struct GameConfig {
        float hiSpeed = 4.0f; // Default HiSpeed (UI scale 0-10)
        bool isControllerMode = false;
        bool enableTapSound = true;
        bool enableFastSlow = true; // FAST/SLOW Display
        float offsetSec = 0.0f; // Offset in seconds
        bool enableLog = true; // ログ出力のオンオフ（リリース時はfalse推奨）
        bool enableCommandLog = true; // コマンドログ出力のオンオフ
        bool enableDebugCamera = false; // デバッグカメラの有効化
        bool enableInputLatencyLog = false; // 入力レイテンシ(MsgDelay)ログ
        bool enableSoundLatencyLog = false; // 音声レイテンシ(InputToSound)ログ
    };

    struct SaveData {
        KeyConfig keyConfig;
        GameConfig gameConfig;
    };

    // インラインで設定インスタンスをもつ　
    inline AudioVolume gAudioVolume{};
    inline KeyConfig   gKeyConfig{};
    inline GameConfig  gGameConfig{};

    void LoadConfig();
    void SaveConfig();
    std::wstring KeyToString(Key key);
}