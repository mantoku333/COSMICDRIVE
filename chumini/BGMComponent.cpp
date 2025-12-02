#include "BGMComponent.h"
#include "Debug.h"
#include "Config.h"

namespace app::test {

    void BGMComponent::Begin() {
        ensurePlayer();
    }

    void BGMComponent::ensurePlayer() {
        if (player.isNull()) {
            if (auto a = actorRef.Target()) {
                player = a->AddComponent<sf::sound::SoundPlayer>();
            }
        }
    }

    void BGMComponent::SetPath(const std::string& p) {
        if (p == path) return;
        path = p;
        loaded = false;
    }

    void BGMComponent::Play() {
        ensurePlayer();
        if (player.isNull() || path.empty()) return;

        if (!loaded) {
            resource.LoadSound(path.c_str());
            loaded = true;
        }

        player->Stop();
        player->SetResource(resource);  // ← SoundPlayer に委譲
        player->Play();
        const float v = gAudioVolume.master * gAudioVolume.bgm;
        if (auto* sv = player->GetSourceVoice()) sv->SetVolume(v);

        sf::debug::Debug::Log(std::string("[BGM] Play: ") + path + " vol=" + std::to_string(v));
        char cwd[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, cwd);
        sf::debug::Debug::Log(std::string("[CWD] ") + cwd);
        sf::debug::Debug::Log(std::string("[BGM] TryPlay: ") + path);
    }

    void BGMComponent::Stop() {
        if (!player.isNull()) player->Stop();
    }

    void BGMComponent::SetVolume(float v) {
        volume = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
        if (!player.isNull()) {
            if (auto* sv = player->GetSourceVoice()) sv->SetVolume(volume);
        }
    }

    float BGMComponent::GetCurrentTime() const {
        // プレイヤーが存在しない場合は0秒
        if (player.isNull()) return 0.0f;

        // 1. 生のSourceVoiceを取得 (IXAudio2SourceVoice*)
        auto* sv = player->GetSourceVoice();
        if (!sv) return 0.0f;

        // 2. 現在の再生状態（累計再生サンプル数）を取得
        XAUDIO2_VOICE_STATE state;
        // 第1引数にNULL以外を指定しないと少し重いが、正確さを取るならこのままでOK
        sv->GetState(&state);

        // 3. ボイスの詳細情報（サンプリングレート）を取得
        // これを使えば waveFormat 変数がなくてもレートがわかります
        XAUDIO2_VOICE_DETAILS details;
        sv->GetVoiceDetails(&details);

        // 4. 計算： 再生サンプル数 / サンプリングレート(Hz) = 現在時刻(秒)
        if (details.InputSampleRate > 0) {
            return (double)state.SamplesPlayed / (double)details.InputSampleRate;
        }

        return 0.0f;
    }

}
