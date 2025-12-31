#pragma once
#include "App.h"
#include "SafePtr.h"

namespace app
{
	namespace test
	{
		class Live2DComponent;

		class ResultScene :public sf::Scene<ResultScene>
		{
		public:
			sf::ref::Ref<sf::Actor> uiManagerActor;
            // Live2D
            sf::SafePtr<Live2DComponent> live2DManager;
 
			void Init()override;
 			void Update(const sf::command::ICommand& command);
            void DrawOverlay() override;
 
		private:
			sf::command::Command<> updateCommand;
		};
	}
}