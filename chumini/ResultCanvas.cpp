#include "ResultCanvas.h"

void app::test::ResultCanvas::Begin()
{
	//基底クラスのBeginを必ず呼び出す
	sf::ui::Canvas::Begin();

	updateCommand.Bind(std::bind(&ResultCanvas::Update, this, std::placeholders::_1));
}

void app::test::ResultCanvas::Update(const sf::command::ICommand&)
{

}