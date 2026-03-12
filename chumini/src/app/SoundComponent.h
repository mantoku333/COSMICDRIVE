#pragma once
#include "App.h"
#include "NoteData.h"
#include <map>

namespace app
{
	namespace test
	{
		class SoundComponent :public sf::Component
		{
		public:
			void Begin()override;
			void Update();

			// 文字列指定での再生
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

			std::map<Sfx, sf::sound::SoundResource> soundResources;

			// 指定したIDの音をロードしてマップに保存
			void LoadAndCache(Sfx id);

			int SongVolume = 1.0;
			bool justone = true;

			const char* ResolvePath(Sfx id) const;
			float ResolveVolume(Sfx id) const;
		};
	}
}