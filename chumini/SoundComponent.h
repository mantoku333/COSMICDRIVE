#pragma once
#include "App.h"
#include "NoteData.h"
#include <map> // ★ 追加: これが必要です

namespace app
{
	namespace test
	{
		class SoundComponent :public sf::Component
		{
		public:
			void Begin()override;
			void Update();

			// 文字列指定での再生（※注意: これもキャッシュしないと連打で落ちる可能性がありますが、一旦そのままにします）
			void Play(const std::string& path);

			// 効果音引数
			enum class Sfx
			{
				Tap,
				EmptyTap,
				HoldStart,
				HoldEnd,
				SongEnd
			};

			void Play(Sfx id);

		private:

			// 更新コマンド
			sf::command::Command<> updateCommand;

			// 音再生コンポーネント(AudioSource)
			sf::SafePtr<sf::sound::SoundPlayer> Player;

			// ★ 変更: 単一のResourceではなく、SfxのIDをキーにしたマップに変更
			// sf::sound::SoundResource Resource; // ← これは削除
			std::map<Sfx, sf::sound::SoundResource> soundResources;

			// ★ 追加: 指定したIDの音をロードしてマップに保存する関数
			void LoadAndCache(Sfx id);

			int SongVolume = 1.0;
			bool justone = true;

			const char* ResolvePath(Sfx id) const;
			float ResolveVolume(Sfx id) const;
		};
	}
}