#include "Canvas.h"

std::list<sf::ui::Canvas*> sf::ui::Canvas::canvasies;
std::mutex sf::ui::Canvas::mtx;

sf::ui::Canvas::~Canvas()
{
	for (auto& i : uis) {
		delete i;
	}

	{
		std::lock_guard<std::mutex> lock(mtx);

		auto it = std::find(canvasies.begin(), canvasies.end(), this);

		if (it != canvasies.end())
		{
			canvasies.erase(it);
		}
	}
}

void sf::ui::Canvas::Begin()
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		canvasies.push_back(this);
		canvasies.sort([](sf::ui::Canvas* a, sf::ui::Canvas* b)
			{
				return a->GetLayer() < b->GetLayer();
			});
	}
}

void sf::ui::Canvas::Activate()
{
	for (auto& i : uis) {
		i->Activate();
	}
}

void sf::ui::Canvas::DeActivate()
{
	for (auto& i : uis) {
		i->DeActivate();
	}
}

void sf::ui::Canvas::SetLayer(int _layer)
{
	layer = _layer;
}

void sf::ui::Canvas::DrawCanvasies()
{
	//ƒLƒƒƒ“ƒoƒX‚Ì•`‰æ
	for (auto& i : canvasies) {
		i->Draw();
	}
}

void sf::ui::Canvas::Draw()
{
	//UI‚Ì•`‰æ
	for (auto& i : uis) {
		if (i->enable)
			i->Draw();
	}
}
