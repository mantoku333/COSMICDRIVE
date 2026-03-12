#pragma once
#include "App.h"

#include "sf/Time.h"        // for sf::Time::DeltaTime()
#include <fstream>
#include <sstream>
#include <algorithm>
#include "NoteData.h"
#include "NoteManager.h" 
#include "SelectCanvas.h"
#include "SongInfo.h"
#include "Live2DComponent.h"
#include "SoundPlayer.h"
#include "GameSession.h"

namespace app { namespace test { class BGMComponent; } }



namespace app
{
	namespace test
	{
		class IngameScene :public sf::Scene<IngameScene>
		{
		public:

		void Init()override;
			void Update(const sf::command::ICommand& command);

			void OnActivated() override;

			/// 繝・せ繝医Λ繧ｯ繧ｿ - 繧ｻ繝・す繝ｧ繝ｳ繧偵ヵ繧ｩ繝ｼ繝ｫ繝舌ャ繧ｯ縺ｫ繧ｳ繝斐・
			~IngameScene();

            void Draw() override;

			void DrawOverlay() override;

			void OnGUI() override;

			sf::ref::Ref<sf::Actor> GetPlayerActor() const { return playerActor; }

			//--------------
			/*const float panelW = 20.0f;
			const float panelH = 30.0f;
			const Vector3 panelPos{ -5.0f, 0.0f, 10.0f }; 

			const float laneW = panelW / lanes;*/
			//-------------------------------------------縺薙ｌ繧峨ｒ謗帝勁縺励※譁ｰ縺励＞險ｭ螳壹↓縺吶ｋ

			const int lanes = 4;           // 繝ｬ繝ｼ繝ｳ謨ｰ
			const float laneW = 3.0f;      // 繝ｬ繝ｼ繝ｳ蟷・
			const float laneH = 50.0f;    // 螂･陦後″・・譁ｹ蜷托ｼ・
			const float rotX = -10.0f;     // 蛯ｾ縺搾ｼ医メ繝･繧ｦ繝九ぜ繝鬚ｨ縺ｮ隗貞ｺｦ・・
			const float baseY = 0.0f;      // 鬮倥＆縺ｮ蝓ｺ貅・

			const float barRatio = 0.1f;

			std::vector<sf::ref::Ref<sf::Actor>> lanePanels;
			std::vector<sf::ref::Ref<sf::Actor>> clickLanes;

			sf::ref::Ref<sf::Actor> managerActor;

			sf::ref::Ref<sf::Actor> judgeBar;  // 蛻､螳壹Λ繧､繝ｳ蜿ら・逕ｨ

			void SetSelectedSong(const SongInfo& song) { selectedSong = song; }
			const SongInfo& GetSelectedSong() const { return selectedSong; }

            // Trigger visual glitch for Skill Notes
            void TriggerSkillEffect();

            /// 繧ｲ繝ｼ繝繧ｻ繝・す繝ｧ繝ｳ縺ｸ縺ｮ蜿ら・繧貞叙蠕・
            GameSession& GetGameSession() { return gameSession; }

		private:
			
			enum class State {
				Idle,
				Countdown,
				Playing
			};
			State state = State::Idle;
			float countdownTimer = 0.0f;
			float startDisplayTimer = 0.0f;
            float skillEffectTimer = 0.0f; // Effect duration timer

			bool isPlaying = false; // 繧ｲ繝ｼ繝荳ｭ縺九←縺・°縺ｮ繝輔Λ繧ｰ

			void StartGame();

			SongInfo selectedSong; // 驕ｸ謚樊･ｽ譖ｲ諠・ｱ
			std::vector<sf::ref::Ref<sf::Actor>> laneEdges;

			// 譖ｴ譁ｰ繧ｳ繝槭Φ繝・
			sf::command::Command<> updateCommand;

			// 繧ｭ繝･繝ｼ繝悶・蠖｢迥ｶ
			sf::geometry::GeometryCube g_cube;

			sf::ref::Ref<sf::Actor> playerActor;

			std::vector<NoteData> noteSequence;
			size_t nextIndex = 0;
			//float leadTime = 1.5f;


			sf::SafePtr<app::test::BGMComponent> bgmPlayer;
			
			sf::SafePtr<app::test::Live2DComponent> l2dComp;

            // Skill SE
            sf::ref::Ref<sf::sound::SoundResource> skillSeResource;
            sf::SafePtr<sf::sound::SoundPlayer> skillSePlayer;

            // Background Objects (3D Primitives)
            struct BgObject {
                sf::ref::Ref<sf::Actor> actor;
                Vector3 rotVel;
                Vector3 moveVel;
            };
            std::vector<BgObject> bgObjects;

            /// 繧ｲ繝ｼ繝繧ｻ繝・す繝ｧ繝ｳ・・繝励Ξ繧､蛻・・蛻､螳壹ョ繝ｼ繧ｿ繧剃ｿ晄戟・・
            GameSession gameSession;
		};
	}
}
