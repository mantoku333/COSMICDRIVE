#pragma once
#include "App.h"
#include "SceneChangeComponent.h"
#include "SongInfo.h"

namespace app
{
	namespace test
	{
		class SelectCanvas;

		// ============================================================
		// SelectScene - 選曲画面のPresenter（MVP）
		// ============================================================
		class SelectScene : public sf::Scene<SelectScene>
        {
        public:
            void Init() override;
            void Update(const sf::command::ICommand& command);

			// MVP: シーン遷移
			void NavigateToGame(const SongInfo& song);
			void NavigateToTitle();

        private:
            sf::ref::Ref<sf::Actor> uiManagerActor;
			sf::SafePtr<SceneChangeComponent> sceneChanger;
			sf::SafePtr<SelectCanvas> selectCanvas;
            sf::command::Command<> updateCommand;
        };
	}
}