#pragma once
#include "SInput.h"
#include <string>

namespace app::test{

    struct AudioVolume {
        float master = 1.0f;  // 全体音量倍率
        float bgm = 0.3f;
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

    // インラインで設定インスタンスをもつ　
    inline AudioVolume gAudioVolume{};
    inline KeyConfig   gKeyConfig{};

    void LoadConfig();
    void SaveConfig();
    std::wstring KeyToString(Key key);
}