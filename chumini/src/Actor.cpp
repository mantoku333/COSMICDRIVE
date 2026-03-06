#include "Actor.h"
#include "Scene.h"
#include "Component.h"

std::map<uintptr_t, sf::Actor*> sf::Actor::actors;
std::queue<sf::ref::Ref<sf::Actor>> sf::Actor::destroyActors;
std::mutex sf::Actor::destroyActorsMtx;

sf::Actor::Actor(IScene* _scene) :scene(*_scene)
{
	actors[GetRef()] = this;

	this->activate = scene.activate;
}

sf::Actor::~Actor()
{
	actors.erase(actors.find(GetRef()));

#if !USE_SMARTPTR
	for (auto i : components) {
		delete i;
}
#endif
}

void sf::Actor::DestroyActors()
{
	while (true)
	{
		sf::ref::Ref<sf::Actor> actor = nullptr;
		{
			std::lock_guard<std::mutex> lock(destroyActorsMtx);
			if (destroyActors.empty()) {
				break;
			}
			actor = destroyActors.front();
			destroyActors.pop();
		}

		delete actor.Target();
	}
}

void sf::Actor::Activate()
{
	activate = true;

	for (auto& i : components) {
		try
		{
			i->Begin();
			i->Activate();
		}
		catch (const std::exception& log)
		{
			debug::Debug::LogError(log.what());
		}
	}
}

void sf::Actor::DeActivate()
{
	activate = false;
	for (auto& i : components) {
		i->DeActivate();
	}
}

void sf::Actor::Destroy()
{
	scene.Destroy(this);
	{
		std::lock_guard<std::mutex> lock(destroyActorsMtx);
		destroyActors.push(this);
	}
}
