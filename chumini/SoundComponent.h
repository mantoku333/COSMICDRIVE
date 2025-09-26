#pragma once
#include "App.h"
#include "NoteData.h"

namespace app
{
	namespace test
	{
		class SoundComponent :public sf::Component
		{
		public:
			void Begin()override;
			void Update();

			void Play(const std::string& path);

			//効果音引数
			enum class Sfx
			{
				Tap,
				EmptyTap,
				HoldStart,
				HoldEnd, 
				SongEnd
			};

			void Play(Sfx id);   // ← これで管理側はパスを知らなくてOK
		private:


			//更新コマンド
			sf::command::Command<> updateCommand;

			//音再生コンポーネント(AudioSource)
			sf::SafePtr<sf::sound::SoundPlayer> Player;

			//音源リソース(AudioClip)
			sf::sound::SoundResource Resource;

			int SongVolume = 1.0;
			bool justone = true;

			const char* ResolvePath(Sfx id) const;
			float ResolveVolume(Sfx id) const;
		};
	}
}