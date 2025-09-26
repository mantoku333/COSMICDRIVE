#pragma once

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

    };

    // インラインで設定インスタンスをもつ　
    inline AudioVolume gAudioVolume{};
    inline KeyConfig   gKeyConfig{};

}