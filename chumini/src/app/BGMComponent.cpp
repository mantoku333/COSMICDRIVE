#include "BGMComponent.h"
#include "Debug.h"
#include "Config.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace app::test {


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
        // プレイヤーが存在しない場合は0秒
        if (player.isNull()) return 0.0f;

        // SourceVoice取得
        auto* sv = player->GetSourceVoice();
        if (!sv) return 0.0f;

        // 現在の再生状態を取得
        XAUDIO2_VOICE_STATE state;

        sv->GetState(&state);

        // ボイスの詳細情報を取得
        XAUDIO2_VOICE_DETAILS details;
        sv->GetVoiceDetails(&details);

        sv->GetVoiceDetails(&details);

        // 計算
        if (details.InputSampleRate > 0) {
            return (double)state.SamplesPlayed / (double)details.InputSampleRate;
        }

        return 0.0f;
    }

    float BGMComponent::SampleRmsAtTime(float timeSec, int windowSamples) const {
        if (!loaded || windowSamples <= 0) {
            return 0.0f;
        }

        const BYTE* raw = resource.GetRawAudioData();
        const size_t rawSize = resource.GetRawAudioSize();
        if (raw == nullptr || rawSize == 0) {
            return 0.0f;
        }

        const WAVEFORMATEXTENSIBLE fmtEx = resource.GetWAVEFORMATEXTENSIBLE();
        const WAVEFORMATEX& fmt = fmtEx.Format;

        const int sampleRate = static_cast<int>(fmt.nSamplesPerSec);
        const int channels = std::max(1, static_cast<int>(fmt.nChannels));
        const int bits = static_cast<int>(fmt.wBitsPerSample);
        const int blockAlign = std::max(1, static_cast<int>(fmt.nBlockAlign));
        if (sampleRate <= 0 || bits <= 0 || blockAlign <= 0) {
            return 0.0f;
        }

        const bool isFloat =
            (fmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
            (fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE && bits == 32);

        const size_t totalFrames = rawSize / static_cast<size_t>(blockAlign);
        if (totalFrames == 0) {
            return 0.0f;
        }

        const int centerFrame = static_cast<int>(timeSec * static_cast<float>(sampleRate));
        const int startFrame = std::max(0, centerFrame - (windowSamples / 2));
        const int endFrame = std::min(static_cast<int>(totalFrames), startFrame + windowSamples);
        if (endFrame <= startFrame) {
            return 0.0f;
        }

        double sumSq = 0.0;
        int sampleCount = 0;
        const int bytesPerSample = std::max(1, bits / 8);

        for (int frame = startFrame; frame < endFrame; ++frame) {
            const size_t frameOffset = static_cast<size_t>(frame) * static_cast<size_t>(blockAlign);
            for (int ch = 0; ch < channels; ++ch) {
                const size_t sampleOffset = frameOffset + static_cast<size_t>(ch) * static_cast<size_t>(bytesPerSample);
                if (sampleOffset + static_cast<size_t>(bytesPerSample) > rawSize) {
                    break;
                }

                const BYTE* p = raw + sampleOffset;
                float v = 0.0f;

                switch (bits) {
                case 8: {
                    v = (static_cast<int>(p[0]) - 128) / 128.0f;
                    break;
                }
                case 16: {
                    int16_t s = 0;
                    std::memcpy(&s, p, sizeof(int16_t));
                    v = static_cast<float>(s) / 32768.0f;
                    break;
                }
                case 24: {
                    int32_t s = (static_cast<int32_t>(p[0])) |
                        (static_cast<int32_t>(p[1]) << 8) |
                        (static_cast<int32_t>(p[2]) << 16);
                    if (s & 0x00800000) {
                        s |= ~0x00FFFFFF;
                    }
                    v = static_cast<float>(s) / 8388608.0f;
                    break;
                }
                case 32: {
                    if (isFloat) {
                        float f = 0.0f;
                        std::memcpy(&f, p, sizeof(float));
                        v = std::clamp(f, -1.0f, 1.0f);
                    }
                    else {
                        int32_t s = 0;
                        std::memcpy(&s, p, sizeof(int32_t));
                        v = static_cast<float>(s) / 2147483648.0f;
                    }
                    break;
                }
                default:
                    break;
                }

                sumSq += static_cast<double>(v) * static_cast<double>(v);
                ++sampleCount;
            }
        }

        if (sampleCount <= 0) {
            return 0.0f;
        }

        const float rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(sampleCount)));
        return std::clamp(rms, 0.0f, 1.0f);
    }

    float BGMComponent::GetAmplitude01(float timeOffsetSec) const {
        const float currentTime = GetCurrentTime();
        const float time = std::max(0.0f, currentTime + timeOffsetSec);
        // 2048 samples keeps motion responsive while reducing per-frame noise.
        return SampleRmsAtTime(time, 2048);
    }

    bool BGMComponent::IsPlaying() const {
        if (player.isNull()) return false;
        auto* sv = player->GetSourceVoice();
        if (!sv) return false;

        // 既存挙動維持: SourceVoiceが存在すれば再生開始済み扱い。
        return true;
    }

}
