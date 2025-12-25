#include "SelectScene.h"
#include "SelectCanvas.h"
#include "SInput.h"
#include "Debug.h"

void app::test::SelectScene::Init()
{

    uiManagerActor = Instantiate();
    auto selectCanvas = uiManagerActor.Target()->AddComponent<SelectCanvas>();
    uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    updateCommand.Bind(std::bind(&SelectScene::Update, this, std::placeholders::_1));

    sf::debug::Debug::Log("SelectScene‰Šú‰»Š®—¹");
    
}

void app::test::SelectScene::Update(const sf::command::ICommand& command)
{
    
}
