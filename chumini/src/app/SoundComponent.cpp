#include "SoundComponent.h"
#include "SongInfo.h"
#include "IngameScene.h"
#include "Debug.h"
#include "Config.h"

void app::test::SoundComponent::Begin()
{
	// Update関数の設定
	updateCommand.Bind(std::bind(&SoundComponent::Update, this));

	// 再生用プレイヤ生成
	if (auto actor = actorRef.Target()) {
		Player = actor->AddComponent<sf::sound::SoundPlayer>();
	}

	// 使う音をすべてプリロードしておく
	LoadAndCache(Sfx::Tap);
	LoadAndCache(Sfx::EmptyTap);

}

// 指定したIDの音をロードしてマップに保持する
void app::test::SoundComponent::LoadAndCache(Sfx id)
{
	const char* path = ResolvePath(id);
	if (path) {
		soundResources[id].LoadSound(path);
	}
}

// 文字列指定での再生デバッグ用
// 基本的には Play(Sfx)
void app::test::SoundComponent::Play(const std::string& path)
{
	if (Player.isNull()) return;
}

const char* app::test::SoundComponent::ResolvePath(Sfx id) const {
	switch (id) {
	case Sfx::Tap:       return "Assets\\sound\\tap.wav";
	case Sfx::EmptyTap:  return "Assets\\sound\\emptytap.wav";

	default:             return nullptr;
	}
}

float app::test::SoundComponent::ResolveVolume(Sfx id) const {
	using namespace app::test;
	float v = gAudioVolume.master;
	switch (id) {
	case Sfx::Tap:       v *= gAudioVolume.tap; break;
	case Sfx::EmptyTap:  v *= gAudioVolume.emptyTap; break;
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

	// ロード済みのリソースがあるかを確認
	auto it = soundResources.find(id);
	if (it != soundResources.end()) {
		// リソースをセット
		Player->SetResource(it->second);

		float vol = ResolveVolume(id);
                vol *= gAudioVolume.master * gAudioVolume.tap;

                Player->Play();
                Player->SetVolume(vol);
	}
	else {
	}
}

void app::test::SoundComponent::Update() {
	// 必要な更新処理があれば
}
