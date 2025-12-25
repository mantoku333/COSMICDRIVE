#include "ResultScene.h"
#include "ResultCanvas.h"
#include "SceneChangeComponent.h"

void app::test::ResultScene::Init()
{
    uiManagerActor = Instantiate();
    auto selectCanvas = uiManagerActor.Target()->AddComponent<ResultCanvas>();
    uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    updateCommand.Bind(std::bind(&ResultScene::Update, this, std::placeholders::_1));

    sf::debug::Debug::Log("ResultScene‰Šú‰»Š®—¹");
}


void app::test::ResultScene::Update(const sf::command::ICommand& command)
{

}
