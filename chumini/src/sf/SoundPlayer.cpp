#include "SoundPlayer.h"

sf::sound::SoundPlayer::~SoundPlayer()
{
	Destroy();
}

void sf::sound::SoundPlayer::DeActivate()
{
	Destroy();
}

void sf::sound::SoundPlayer::Destroy()
{
	if (source != nullptr)
	{
		source->Stop();
		source->FlushSourceBuffers();
		source->DestroyVoice();
		source = nullptr;
	}
}

void sf::sound::SoundPlayer::Play(float startTime)
{
	if (source != nullptr)
	{
		source->Stop();
		source->FlushSourceBuffers();
	}
	
	SetSound(startTime);
	
	if (source) {
		source->Start();
	}
}

void sf::sound::SoundPlayer::Stop()
{
	if (source != nullptr)
	{
		source->Stop();
		source->FlushSourceBuffers();
	}
}

void sf::sound::SoundPlayer::SetResource(sf::ref::Ref<SoundResource> _v)
{
    // Destroy existing voice to ensure new format is applied and state is clean
    Destroy();
	resource = _v;
}

void sf::sound::SoundPlayer::SetSound(float startTime)
{
    if (resource.Target() == nullptr) return;

	WAVEFORMATEX data = resource.Target()->GetWAVEFORMATEXTENSIBLE().Format;

	if (source == nullptr) {
		if (FAILED(SoundResource::GetIXAudio2()->CreateSourceVoice(&source, &data))) {
			return; // Failed
		}
		source->SetVolume(currentVolume);
	}

	XAUDIO2_BUFFER buffer = resource.Target()->GetXAUDIO2_BUFFER();

	// Calculate start position
	buffer.PlayBegin = (UINT32)(startTime * data.nSamplesPerSec);
	if (buffer.PlayBegin >= buffer.AudioBytes / data.nBlockAlign) {
		buffer.PlayBegin = 0; 
	}

	source->SubmitSourceBuffer(&buffer);
}

void sf::sound::SoundPlayer::SetVolume(float volume)
{
	currentVolume = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);

	if (source) {
		source->SetVolume(currentVolume);
	}
}

float sf::sound::SoundPlayer::GetVolume() const
{
	return currentVolume;
}
