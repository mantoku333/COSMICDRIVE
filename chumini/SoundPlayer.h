#pragma once
#include "Component.h"
#include "SoundResource.h"
namespace sf
{
	namespace sound
	{
		//音再生コンポーネント
		class SoundPlayer :public Component
		{
		public:
			~SoundPlayer();
			void DeActivate()override;

			void Play();
			void Stop();

			void SetResource(sf::ref::Ref<SoundResource> _v);

			// 音量制御メソッドを追加
			void SetVolume(float volume);
			float GetVolume() const;
			IXAudio2SourceVoice* GetSourceVoice() const { return source; }


		private:
			void SetSound();

		private:
			sf::ref::Ref<SoundResource> resource;

			IXAudio2SourceVoice* source = nullptr;
			float currentVolume = 1.0f;
		};
	}
}