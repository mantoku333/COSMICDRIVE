#pragma once
#include "Component.h"
#include "SafePtr.h"
#include "SoundPlayer.h"
#include "SoundResource.h"
#include <string>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

namespace app::test {

    class BGMComponent : public sf::Component {
    public:
        void Begin() override;
       // void Update(const sf::command::ICommand&) override {}
        virtual ~BGMComponent();

        // API
        void SetPath(const std::string& p);
        void Play();
        void Stop();

        void SetVolume(float v);
        void SetLoop(bool v) { loop = v; }
        
        bool IsPlaying() const;

        float GetCurrentTime() const;

    private:
        void ensurePlayer();
        // ★追加: XAudio2 制御用変数
        IXAudio2* pXAudio2 = nullptr;             // エンジン本体
        IXAudio2MasteringVoice* pMasterVoice = nullptr; // マスターボイス
        IXAudio2SourceVoice* pSourceVoice = nullptr;    // ★これが「再生中の音声」
        WAVEFORMATEX waveFormat = { 0 };          // ★これが「音声フォーマット情報」

    private:
        sf::SafePtr<sf::sound::SoundPlayer> player; // 内部で利用
        sf::sound::SoundResource resource;
        std::string path;
        bool loaded = false;
        bool loop = true;
        float volume = 1.0f;
    };

}
