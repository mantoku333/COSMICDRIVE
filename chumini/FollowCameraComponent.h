#pragma once
#include "App.h"
namespace app
{
    namespace test
    {
        class FollowCameraComponent : public sf::Component
        {
        public:
            Vector3 offset = Vector3(0, 0, -10);

            void Begin() override;

           

		private:
            void Update(const sf::command::ICommand& command);

            float cameraYaw = 0.0f;   // …•½‰ñ“]
            float cameraPitch = 10.0f; // ‚’¼‰ñ“]i‰Šú’l‚Í‚â‚â‰ºŒü‚«j
            Vector2 prevMousePos = Vector2::Zero;
            bool firstUpdate = true;


            sf::command::Command<> updateCommand;
        };
    }
}