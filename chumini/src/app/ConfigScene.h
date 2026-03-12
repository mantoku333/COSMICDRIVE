#pragma once
#include "App.h"
#include "SafePtr.h"
#include "SceneChangeComponent.h"

namespace app
{
	namespace test
	{
		class ConfigCanvas; // 前方宣言

		// MVP: ConfigSceneがLogicとStateを持つ
		class ConfigScene : public sf::Scene<ConfigScene>
		{
		public:
			// 状態定義（Canvasから移動）
			enum class State {
				Normal,
				WaitingForKey
			};

			void Init() override;
			void Update(const sf::command::ICommand& command);

			// MVP: 入力ハンドラ
			void OnNavigateUp();
			void OnNavigateDown();
			void OnNavigateLeft();
			void OnNavigateRight();
			void OnConfirm();
			void OnCancel();

			// キーコンフィグ用
			void OnKeyInputStart(int laneIndex);
			void OnKeyInput(int key);

			// 状態取得
			State GetState() const { return state; }
			int GetWaitingLaneIndex() const { return waitingLaneIndex; }
			int GetSelectedIndex() const { return selectedIndex; }

		private:
			sf::ref::Ref<sf::Actor> uiManagerActor;
			sf::command::Command<> updateCommand;

			sf::SafePtr<ConfigCanvas> configCanvas;
			sf::SafePtr<SceneChangeComponent> sceneChanger;

			// 状態管理
			State state = State::Normal;
			int waitingLaneIndex = -1;
			int selectedIndex = 0;

			void NavigateToTitle();
		};
	}
}
