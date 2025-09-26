#include "SoundComponent.h"
#include "SongInfo.h"
#include "TestScene.h"
#include "Config.h"

void app::test::SoundComponent::Begin()
{
	//Update関数の設定
    updateCommand.Bind(std::bind(&SoundComponent::Update, this));

    // 再生用プレイヤ生成
    if (auto actor = actorRef.Target()) {
        Player = actor->AddComponent<sf::sound::SoundPlayer>();
    }
}

void app::test::SoundComponent::Play(const std::string& path)
{
    if (Player.isNull()) return;

    // 読み込み → セット → 再生
    Resource.LoadSound(path.c_str());
    Player->SetResource(Resource);
    Player->Play();
}

const char* app::test::SoundComponent::ResolvePath(Sfx id) const {
    switch (id) {
    case Sfx::Tap:       return "Assets\\sound\\tap.wav";
    case Sfx::EmptyTap:  return "Assets\\sound\\emptytap.wav";
    //case Sfx::HoldStart: return "Assets\\sound\\hold_start.wav"; // あれば
    //case Sfx::HoldEnd:   return "Assets\\sound\\hold_end.wav";   // あれば
   // case Sfx::SongEnd:   return "Assets\\sound\\clear.wav";      // あれば
    default:             return nullptr;
    }
}

float app::test::SoundComponent::ResolveVolume(Sfx id) const {
    using namespace app::test;
    float v = gAudioVolume.master;
    switch (id) {
    case Sfx::Tap:       v *= gAudioVolume.tap; break;
    case Sfx::EmptyTap:  v *= gAudioVolume.emptyTap; break;
   // case Sfx::HoldStart: v *= gAudioVolume.holdStart; break;
    //case Sfx::HoldEnd:   v *= gAudioVolume.holdEnd; break;
   // case Sfx::SongEnd:   v *= gAudioVolume.bgm; break; // 仮
    }
    return v;
}

void app::test::SoundComponent::Play(Sfx id) {
    if (Player.isNull()) {
        if (auto actor = actorRef.Target()) {
            Player = actor->AddComponent<sf::sound::SoundPlayer>();
        }
    }
    if (Player.isNull()) return;

    if (const char* path = ResolvePath(id)) {
        Resource.LoadSound(path);
        Player->SetResource(Resource);

        float vol = ResolveVolume(id);
       // sf::debug::Debug::Log("Play " + std::string(path) + " vol=" + std::to_string(vol));

        Player->Play();
        Player->SetVolume(vol); // ←後から
    }
}

void app::test::SoundComponent::Update(){

}

//
//
//void app::test::SoundComponent::Update()
//{
//
//	if (justone) {
//		if (Song->GetSourceVoice()) Song->GetSourceVoice()->SetVolume(0.1f);
//		if (EmptyTap->GetSourceVoice()) EmptyTap->GetSourceVoice()->SetVolume(0.5f);
//		justone = false;
//	}
//
//	//if (SInput::Instance().GetKeyDown(Key::KEY_1))
//	//{
//	//	if (Song->GetSourceVoice()) {
//	//		SongVolume - 0.1;
//	//		Song->GetSourceVoice()->SetVolume(SongVolume);//音量ダウン
//	//	}
//	//}
//
//	//if (SInput::Instance().GetKeyDown(Key::KEY_2))
//	//{
//	//	if (Song->GetSourceVoice()) {
//	//		SongVolume + 0.1;
//	//		Song->GetSourceVoice()->SetVolume(SongVolume);//音量アップ
//	//	}
//	//}
//
//
//	// 任意サウンド再生 (デバッグ用)
//	if (SInput::Instance().GetKeyDown(Key::KEY_B))
//	{
//		Tap->Play();
//	}
//}

////ノーツタイプにあわせた音を再生
//void app::test::SoundComponent::HitSound(app::test::NoteType noteType)
//{
//	switch (noteType)
//	{
//	case app::test::NoteType::Tap:
//		Tap->Play();
//		break;
//	case app::test::NoteType::HoldStart:
//		break;
//
//	case app::test::NoteType::HoldEnd:
//		break;
//
//	case app::test::NoteType::SongEnd:
//		break;
//	default:
//
//		break;
//	}
//}
//
////なにもない時の音
//void app::test::SoundComponent::EmptyHitSound()
//{
//	EmptyTap->Play();
//}
//
//
