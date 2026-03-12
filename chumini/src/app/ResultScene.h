#pragma once
#include "App.h"
#include "SafePtr.h"
#include "SceneChangeComponent.h"

namespace app
{
	namespace test
	{
		class Live2DComponent;
		class ResultCanvas;

		// ============================================================
		// ResultScene - リザルト画面のPresenter（MVP）
		// 
		// 役割:
		// - シーン遷移ロジック
		// ============================================================
		class ResultScene :public sf::Scene<ResultScene>
		{
		public:
			sf::ref::Ref<sf::Actor> uiManagerActor;
            // Live2D
            sf::SafePtr<Live2DComponent> live2DManager;
 
			void Init()override;
 			void Update(const sf::command::ICommand& command);
            void DrawOverlay() override;
            void OnActivated() override;

			// MVP: シーン遷移
			void NavigateToSelect();

		private:
			sf::command::Command<> updateCommand;
			sf::SafePtr<SceneChangeComponent> sceneChanger;
			sf::SafePtr<ResultCanvas> resultCanvas;
		};
	}
}