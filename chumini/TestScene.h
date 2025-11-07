#pragma once
#include "App.h"

#include "Time.h"        // for sf::Time::DeltaTime()
#include <fstream>
#include <sstream>
#include <algorithm>
#include "NoteData.h"
#include "NoteManager.h" 
#include "SelectCanvas.h"
#include "SongInfo.h"
#include "BGMComponent.h"



namespace app
{
	namespace test
	{
		class TestScene :public sf::Scene<TestScene>
		{
		public:

			void Init()override;
			void Update(const sf::command::ICommand& command);

			void OnActivated() override;

			sf::ref::Ref<sf::Actor> GetPlayerActor() const { return playerActor; }

			//--------------
			/*const float panelW = 20.0f;
			const float panelH = 30.0f;
			const Vector3 panelPos{ -5.0f, 0.0f, 10.0f }; 

			const float laneW = panelW / lanes;*/
			//-------------------------------------------これらを排除して新しい設定にする

			const int lanes = 5;           // レーン数
			const float laneW = 3.0f;      // レーン幅
			const float laneH = 50.0f;    // 奥行き（Z方向）
			const float rotX = -10.0f;     // 傾き（チュウニズム風の角度）
			const float baseY = 0.0f;      // 高さの基準

			const float barRatio = 0.1f;

			std::vector<sf::ref::Ref<sf::Actor>> lanePanels;

			sf::ref::Ref<sf::Actor> managerActor;

			

			void SetSelectedSong(const SongInfo& song) { selectedSong = song; }
			const SongInfo& GetSelectedSong() const { return selectedSong; }

		private:
			
			SongInfo selectedSong; // 選択楽曲情報
			std::vector<sf::ref::Ref<sf::Actor>> laneEdges;

			// 更新コマンド
			sf::command::Command<> updateCommand;

			// キューブの形状
			sf::geometry::GeometryCube g_cube;

			sf::ref::Ref<sf::Actor> playerActor;

			std::vector<NoteData> noteSequence;
			size_t nextIndex = 0;
			//float leadTime = 1.5f;


			sf::SafePtr<app::test::BGMComponent> bgmPlayer;
		};
	}
}