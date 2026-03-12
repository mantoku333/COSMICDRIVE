#include "Canvas.h"
#include "Debug.h"
#include <exception>

std::list<sf::ui::Canvas*> sf::ui::Canvas::canvasies;
std::mutex sf::ui::Canvas::mtx;

sf::ui::Canvas::~Canvas()
{
	// Safe UI Cleanup
	for (auto& i : uis) {
		try {
			delete i;
		} catch (const std::exception& e) {
			sf::debug::Debug::LogError("[Canvas] ~Canvas: Exception during UI deletion - " + std::string(e.what()));
		} catch (...) {
			sf::debug::Debug::LogError("[Canvas] ~Canvas: Unknown exception during UI deletion");
		}
	}
	uis.clear();

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
		if (i) {
			i->Activate();
		}
	}
}

void sf::ui::Canvas::DeActivate()
{
	for (auto& i : uis) {
		if (i) {
			i->DeActivate();
		}
	}
}

void sf::ui::Canvas::SetLayer(int _layer)
{
	layer = _layer;
}

void sf::ui::Canvas::DrawCanvasies()
{
	// Draw Canvases (With Exception Handling)
	std::lock_guard<std::mutex> lock(mtx);
	for (auto& i : canvasies) {
		if (!i) continue;
		
		try {
			i->Draw();
		} catch (const std::exception& e) {
			sf::debug::Debug::LogError("[Canvas] DrawCanvasies: Exception during Draw - " + std::string(e.what()));
		} catch (...) {
			sf::debug::Debug::LogError("[Canvas] DrawCanvasies: Unknown exception during Draw");
		}
	}
}

void sf::ui::Canvas::Draw()
{
	// Draw UI (With Exception Handling)
	for (auto& i : uis) {
		if (!i || !i->enable) continue;
		
		try {
			i->Draw();
		} catch (const std::exception& e) {
			sf::debug::Debug::LogError("[Canvas] Draw: Exception during UI Draw - " + std::string(e.what()));
		} catch (...) {
			sf::debug::Debug::LogError("[Canvas] Draw: Unknown exception during UI Draw");
		}
	}
}
