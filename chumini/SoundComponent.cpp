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

	// ★ここで使う音をすべてプリロード（メモリに確保）しておく
	// これにより、再生中にディスク読み込みやメモリ再確保が走らなくなります
	LoadAndCache(Sfx::Tap);
	LoadAndCache(Sfx::EmptyTap);
	// LoadAndCache(Sfx::HoldStart); // 必要に応じてコメント解除
	// LoadAndCache(Sfx::HoldEnd);
	// LoadAndCache(Sfx::SongEnd);
}

// ★新規追加: 指定したIDの音をロードしてマップに保持する
void app::test::SoundComponent::LoadAndCache(Sfx id)
{
	const char* path = ResolvePath(id);
	if (path) {
		// マップの [] 演算子は、キーが存在しない場合に新規作成します
		// 既にロード済みなら何もしない、あるいは上書きロードになります
		soundResources[id].LoadSound(path);
	}
}

// 文字列指定での再生
// ※注意: ヘッダーから `Resource` 変数を消したので、
// 以前のコードのままだとコンパイルエラーになります。
// 必要でなければ削除するか、デバッグ用として割り切るなら以下のように実装できますが、
// 基本的には Play(Sfx) を使うことを推奨します。
void app::test::SoundComponent::Play(const std::string& path)
{
	if (Player.isNull()) return;

	// (デバッグ用: 毎回ロードするので連打すると危険ですが、一旦動くように書くならこうです)
	// 本来は文字列用のマップも用意すべきです
	/*
	static sf::sound::SoundResource tempRes;
	tempRes.LoadSound(path.c_str());
	Player->SetResource(tempRes);
	Player->Play();
	*/
}

const char* app::test::SoundComponent::ResolvePath(Sfx id) const {
	switch (id) {
	case Sfx::Tap:       return "Assets\\sound\\tap.wav";
	case Sfx::EmptyTap:  return "Assets\\sound\\emptytap.wav";
		// case Sfx::HoldStart: return "Assets\\sound\\hold_start.wav";
		// case Sfx::HoldEnd:   return "Assets\\sound\\hold_end.wav";
		// case Sfx::SongEnd:   return "Assets\\sound\\clear.wav";
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
		// case Sfx::HoldEnd:   v *= gAudioVolume.holdEnd; break;
		// case Sfx::SongEnd:   v *= gAudioVolume.bgm; break;
	}
	return v;
}

void app::test::SoundComponent::Play(Sfx id) {
	// プレイヤーがない、またはプレイヤー生成を試みる
	if (Player.isNull()) {
		if (auto actor = actorRef.Target()) {
			Player = actor->AddComponent<sf::sound::SoundPlayer>();
		}
	}
	if (Player.isNull()) return;

	// ★ここが修正のメインです
	// findを使って「ロード済みのリソースがあるか」を確認します
	auto it = soundResources.find(id);
	if (it != soundResources.end()) {
		// ロードはせず、マップ内のリソース(it->second)をセットするだけ
		// これならメモリのアドレスが変わらないのでXAudio2が参照し続けても安全です
		Player->SetResource(it->second);

		float vol = ResolveVolume(id);
                vol *= gAudioVolume.master * gAudioVolume.tap;

                Player->Play();
                Player->SetVolume(vol);
	}
	else {
		// まだロードされていない場合（Beginに追加し忘れた場合など）
		// 必要であればここで LoadAndCache(id); して再生することも可能です
	}
}

void app::test::SoundComponent::Update() {
	// 必要な更新処理があれば
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
