#include "SoundPlayer.h"

sf::sound::SoundPlayer::~SoundPlayer()
{
	Stop();
}

void sf::sound::SoundPlayer::DeActivate()
{
	Stop();
}

void sf::sound::SoundPlayer::Play()
{
	Stop();
	if (source == nullptr)
	{
		SetSound();
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

	SetSound();

	//source->Start();
}

void sf::sound::SoundPlayer::SetSound()
{
	WAVEFORMATEX data = resource.Target()->GetWAVEFORMATEXTENSIBLE().Format;

	SoundResource::GetIXAudio2()->CreateSourceVoice(&source, &data);
	XAUDIO2_BUFFER buffer = resource.Target()->GetXAUDIO2_BUFFER();
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


