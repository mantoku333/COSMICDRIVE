#pragma once
#include "App.h"
#include "Actor.h"

namespace app
{
	//マウスでカメラ移動するコンポーネント
	class ControlCamera :public sf::Component
	{
	public:
		void Begin()override;

		// targetActorメンバーを追加
		sf::SafePtr<sf::Actor> targetActor;


	private:
		void Update();

		void Rotation();
		void Move();

	private:
		sf::command::Command<> command;

		Vector3 rot;

		sf::del::DELEGATE<ControlCamera> del = this;
		

		POINT pre{};
	};
}