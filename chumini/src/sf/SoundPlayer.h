#pragma once
#include "Component.h"
#include "SoundResource.h"
namespace sf
{
	namespace sound
	{
		// Sound Player Component
		class SoundPlayer :public Component
		{
		public:
            ~SoundPlayer();
			void DeActivate()override;
            void Destroy(); // Added for proper cleanup

			void Play(float startTime = 0.0f);
			void Stop();

			void SetResource(sf::ref::Ref<SoundResource> _v);

			// Volume control methods
			void SetVolume(float volume);
			float GetVolume() const;
			IXAudio2SourceVoice* GetSourceVoice() const { return source; }


		private:
			void SetSound(float startTime = 0.0f);

		private:
			sf::ref::Ref<SoundResource> resource;

			IXAudio2SourceVoice* source = nullptr;
			float currentVolume = 1.0f;
		};
	}
}