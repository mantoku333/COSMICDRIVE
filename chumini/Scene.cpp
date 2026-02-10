#include "Scene.h"
#include "sf.h"
#include "App.h"

std::queue<const sf::IScene*> sf::IScene::deActiveScene;
std::mutex sf::IScene::deActiveMtx;
std::list<sf::IScene*> sf::IScene::scenes;

sf::IScene::IScene()
{
	scenes.push_back(this);
}

sf::IScene::~IScene()
{
	for (auto& i : actors) {
		delete i;
	}

	auto it = std::find(scenes.begin(), scenes.end(), this);
	scenes.erase(it);
}

void sf::IScene::Load()
{
	app::Application::GetMain()->jobSystem.addJob([this]
		{
			std::string sceneName = typeid(*this).name();

			sf::debug::Debug::LogEngine("Start Scene Loading: " + sceneName);

			bool result = true;

			try
			{
				// Scene Init
				this->Init();
			}
			catch (const std::exception& hoge)
			{
				sf::debug::Debug::LogError("Exception during scene loading:\n" + std::string(hoge.what()));

				result = false;
			}

			// Add scene as standby
			// app::Application::GetMain()->SetStandbyScene(this);

			// Log

			if (result)
			{
				sf::debug::Debug::LogEngine("Scene Loading Completed: " + sceneName);
			}
			else
			{
				sf::debug::Debug::LogWarning("Scene Loading Completed with Exception: " + sceneName);
			}

			// Loaded Flag
			this->Loaded();
		}
	);
}

void sf::IScene::Activate()
{
	std::string sceneName = typeid(*this).name();
	sf::debug::Debug::LogEngine("[Scene] Activate Start: " + sceneName);
	
	activate = true;

	for (auto& i : actors) {
		if (i) i->Activate();
	}

	OnActivated();

	app::Application::GetMain()->ActivateScene(this);
	
	sf::debug::Debug::LogSafety("[Scene] Activate Completed: " + sceneName);
}

void sf::IScene::DeActivate()
{
	std::string sceneName = typeid(*this).name();
	sf::debug::Debug::LogEngine("[Scene] DeActivate Start: " + sceneName);
	
	activate = false;

	{
		std::lock_guard<std::mutex> m(deActiveMtx);
		deActiveScene.push(this);
	}
	app::Application::GetMain()->DeActivateScene(this);
	
	sf::debug::Debug::Log("[Scene] DeActivate Completed: " + sceneName);
}

void sf::IScene::DestroyScenes()
{
	while (!deActiveScene.empty())
	{
		const IScene* scene = deActiveScene.front();
		deActiveScene.pop();
		std::string name = typeid(*scene).name();
		delete scene;
		sf::debug::Debug::LogEngine("Scene Deactivated: " + name);
	}
}

void sf::IScene::DestroyFromOnApplicaitonExit()
{
	for (int i = 0; i < scenes.size(); i++)
	{
		delete scenes.front();
	}
}

sf::ref::Ref<sf::Actor> sf::IScene::Instantiate()
{
	Actor* obj = new Actor(this);
	{
		std::lock_guard<std::mutex> lock(actorsMtx);
		actors.push_back(obj);

	}
	ref::Ref<Actor> ret = obj;
	return ret;
}

void sf::IScene::OnGUI()
{
	std::string sceneName = typeid(*this).name();
	ImGui::Begin((sceneName + "Hierarchy").c_str());
	{
		std::lock_guard<std::mutex> lock(actorsMtx);
		for (auto& i : actors)
		{
			if (ImGui::Button(std::to_string(i->GetRef()).c_str()))
			{
				select = i;
			}

		}
	}
	ImGui::End();

	ImGui::Begin((sceneName + "Inspector").c_str());

	if (!select.IsNull())
	{

		Actor* actor = select.Target();
		float hoge[3]{};


		Vector3 p = actor->transform.GetPosition();
		hoge[0] = p.x;
		hoge[1] = p.y;
		hoge[2] = p.z;
		if (ImGui::DragFloat3(("Position" + gui::GUI::GetOffSet()).c_str(), hoge, 0.1f))
		{
			p.x = hoge[0];
			p.y = hoge[1];
			p.z = hoge[2];
			actor->transform.SetPosition(p);
		}

		p = actor->transform.GetRotation();
		hoge[0] = p.x;
		hoge[1] = p.y;
		hoge[2] = p.z;
		if (ImGui::DragFloat3(("Rotation" + gui::GUI::GetOffSet()).c_str(), hoge, 0.1f))
		{
			p.x = hoge[0];
			p.y = hoge[1];
			p.z = hoge[2];
			actor->transform.SetRotation(p);
		}

		p = actor->transform.GetScale();
		hoge[0] = p.x;
		hoge[1] = p.y;
		hoge[2] = p.z;
		if (ImGui::DragFloat3(("Rotation" + gui::GUI::GetOffSet()).c_str(), hoge, 0.1f))
		{
			p.x = hoge[0];
			p.y = hoge[1];
			p.z = hoge[2];
			actor->transform.SetScale(p);
		}

		ImGui::NewLine();

		for (auto& i : actor->components) {
			ImGui::Text(typeid(*i).name());
		}
	}
	ImGui::End();
}

void sf::IScene::Destroy(const Actor* actor)
{
	std::lock_guard<std::mutex> lock(actorsMtx);
	auto it = std::find(actors.begin(), actors.end(), actor);
	if (it != actors.end())
	{
		actors.erase(it);
	}
}
