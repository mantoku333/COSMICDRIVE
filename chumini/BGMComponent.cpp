#include "BGMComponent.h"
#include "Debug.h"
#include "Config.h"

namespace app::test {

    // ★追加: デストラクタ
    // コンポーネント破棄時に確実に停止させることで、
    // 死んだポインタへのアクセス（例外）を防ぎます。
    BGMComponent::~BGMComponent() {
        Stop();
    }

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
        player->SetResource(resource);
        player->Play();

        const float v = gAudioVolume.master * gAudioVolume.bgm;
        if (auto* sv = player->GetSourceVoice()) {
            sv->SetVolume(v);
        }

        sf::debug::Debug::Log(std::string("[BGM] Play: ") + path + " vol=" + std::to_string(v));
        // デバッグログ周りはそのまま維持
        char cwd[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, cwd);
        sf::debug::Debug::Log(std::string("[CWD] ") + cwd);
        sf::debug::Debug::Log(std::string("[BGM] TryPlay: ") + path);
    }

    void BGMComponent::Stop() {
        // playerが生きていれば止める
        if (!player.isNull()) {
            player->Stop();
        }
    }

    void BGMComponent::SetVolume(float v) {
        volume = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
        if (!player.isNull()) {
            if (auto* sv = player->GetSourceVoice()) {
                sv->SetVolume(volume);
            }
        }
    }

    float BGMComponent::GetCurrentTime() const {
        // 1. プレイヤーが存在しない場合は0秒
        if (player.isNull()) return 0.0f;

        // 2. 生のSourceVoiceを取得
        // シーン遷移中はここが nullptr になっている可能性があるのでチェック必須
        auto* sv = player->GetSourceVoice();
        if (!sv) return 0.0f;

        // 3. 現在の再生状態を取得
        XAUDIO2_VOICE_STATE state;

        // ★変更点: XAUDIO2_VOICE_NOSAMPLESPLAYED フラグを削除
        // これにより SamplesPlayed が正しく更新されるようになります
        sv->GetState(&state);

        // 4. ボイスの詳細情報を取得
        XAUDIO2_VOICE_DETAILS details;
        sv->GetVoiceDetails(&details);

        sv->GetVoiceDetails(&details);

        // 5. 計算
        if (details.InputSampleRate > 0) {
            return (double)state.SamplesPlayed / (double)details.InputSampleRate;
        }

        return 0.0f;
    }

    bool BGMComponent::IsPlaying() const {
        if (player.isNull()) return false;
        auto* sv = player->GetSourceVoice();
        if (!sv) return false;

        // 再生中かどうかの簡易チェック
        // XAudio2には直接 IsPlaying はないが、バッファキューの状態などで判定可能かもしれない
        // ここではSoundPlayerの状態に依存するか、SourceVoiceがあれば再生中とみなす実装にする
        // より厳密には XAUDIO2_VOICE_STATE の pCurrentBufferContext などを見る必要があるが
        // 今回は「再生開始済み」かどうかが分かればいいので
        return true; 
    }

}