#pragma once
#include "App.h"

#include "sf/Time.h"
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
		/// インゲームシーン
		/// レーン配置・ノート管理・カウントダウン・BGM同期・背景演出を総合管理する
		class IngameScene :public sf::Scene<IngameScene>
		{
		public:

		void Init()override;
			void Update(const sf::command::ICommand& command);

			void OnActivated() override;

			~IngameScene();

            void Draw() override;

			void DrawOverlay() override;

			void OnGUI() override;

			sf::ref::Ref<sf::Actor> GetPlayerActor() const { return playerActor; }

			// ===== レーンパラメータ =====
			const int lanes = 4;           // メインレーン数
			const float laneW = 3.0f;      // レーン幅
			const float laneH = 50.0f;     // レーン奥行き
			const float rotX = -10.0f;     // X軸回転角度（度）
			const float baseY = 0.0f;      // レーン基準Y座標
			const float barRatio = 0.1f;   // 判定ラインの位置比率

			std::vector<sf::ref::Ref<sf::Actor>> lanePanels;  // レーンパネルアクター
			std::vector<sf::ref::Ref<sf::Actor>> clickLanes;  // クリック判定レーン

			sf::ref::Ref<sf::Actor> managerActor;  // マネージャーアクター
			sf::ref::Ref<sf::Actor> judgeBar;      // 判定バーアクター

			void SetSelectedSong(const SongInfo& song) { selectedSong = song; }
			const SongInfo& GetSelectedSong() const { return selectedSong; }

            /// スキルノート発動時の視覚エフェクトをトリガーする
            void TriggerSkillEffect();

            /// 現在のゲームセッションを取得する
            GameSession& GetGameSession() { return gameSession; }

		private:
			
			// --- ステートマシン ---
			enum class State {
				Idle,       // 待機
				Countdown,  // カウントダウン中
				Playing     // プレイ中
			};
			State state = State::Idle;
			float countdownTimer = 0.0f;       // カウントダウン残り時間
			float startDisplayTimer = 0.0f;    // 「START」表示の残り時間
            float skillEffectTimer = 0.0f;     // スキルエフェクトの残り時間

			bool isPlaying = false;            // ゲーム開始済みフラグ

			void StartGame();                  // ゲーム開始処理

			// --- 楽曲情報 ---
			SongInfo selectedSong;
			std::vector<sf::ref::Ref<sf::Actor>> laneEdges;  // レーン外枠アクター

			sf::command::Command<> updateCommand;  // 更新コマンド

			sf::geometry::GeometryCube g_cube;     // 共通キューブジオメトリ

			sf::ref::Ref<sf::Actor> playerActor;   // プレイヤーアクター

			std::vector<NoteData> noteSequence;    // ノートシーケンス
			size_t nextIndex = 0;

			sf::SafePtr<app::test::BGMComponent> bgmPlayer;        // BGMプレイヤー
			sf::SafePtr<app::test::Live2DComponent> l2dComp;       // Live2Dコンポーネント

            // --- スキルSE ---
            sf::ref::Ref<sf::sound::SoundResource> skillSeResource;  // スキルSEリソース
            sf::SafePtr<sf::sound::SoundPlayer> skillSePlayer;       // スキルSEプレイヤー

            // --- 背景オブジェクト（3Dプリミティブ） ---
            struct BgObject {
                sf::ref::Ref<sf::Actor> actor;  // 背景オブジェクトアクター
                Vector3 rotVel;                 // 回転速度
                Vector3 moveVel;                // 移動速度
            };
            std::vector<BgObject> bgObjects;

            GameSession gameSession;  // ゲームセッションデータ
		};
	}
}
