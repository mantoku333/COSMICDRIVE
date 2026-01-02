#include "SoundPlayer.h"

sf::sound::SoundPlayer::~SoundPlayer()
{
	Stop();
}

void sf::sound::SoundPlayer::DeActivate()
{
	Stop();
}

void sf::sound::SoundPlayer::Play(float startTime)
{
	Stop();
	if (source == nullptr)
	{
		SetSound(startTime);
	}
	source->Start();
}

void sf::sound::SoundPlayer::Stop()
{
	if (source != nullptr)
	{
		source->Stop();
		source = nullptr;
	}
}

void sf::sound::SoundPlayer::SetResource(sf::ref::Ref<SoundResource> _v)
{
	resource = _v;

	SetSound(0.0f);

	//source->Start();
}

void sf::sound::SoundPlayer::SetSound(float startTime)
{
	WAVEFORMATEX data = resource.Target()->GetWAVEFORMATEXTENSIBLE().Format;

	SoundResource::GetIXAudio2()->CreateSourceVoice(&source, &data);
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
