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

			// 補助処理
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
			// -------------------------------------------これらを排除して新しい設定にする

			const int lanes = 4;           // レーン数
			const float laneW = 3.0f;      // レーン幅
			const float laneH = 50.0f;     // レーン長
			const float rotX = -10.0f;
			const float baseY = 0.0f;

			const float barRatio = 0.1f;

			std::vector<sf::ref::Ref<sf::Actor>> lanePanels;
			std::vector<sf::ref::Ref<sf::Actor>> clickLanes;

			sf::ref::Ref<sf::Actor> managerActor;

			sf::ref::Ref<sf::Actor> judgeBar;

			void SetSelectedSong(const SongInfo& song) { selectedSong = song; }
			const SongInfo& GetSelectedSong() const { return selectedSong; }

            // Trigger visual glitch for Skill Notes
            void TriggerSkillEffect();

            // 処理本体
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

			bool isPlaying = false;

			void StartGame();

			SongInfo selectedSong;
			std::vector<sf::ref::Ref<sf::Actor>> laneEdges;

			// 処理本体
			sf::command::Command<> updateCommand;

			// 処理本体
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

            // 処理本体
            GameSession gameSession;
		};
	}
}
