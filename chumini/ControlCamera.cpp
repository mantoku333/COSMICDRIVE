#include "ControlCamera.h"

void app::ControlCamera::Begin()
{
	command.Bind(std::bind(&ControlCamera::Update, this));

	del += &ControlCamera::Rotation;
	del += &ControlCamera::Move;

	GetCursorPos(&pre);
}

void app::ControlCamera::Update()
{
	del();
}

void app::ControlCamera::Rotation()
{
	POINT p;
	GetCursorPos(&p);

	float x = float(p.x - pre.x);
	float y = float(p.y - pre.y);

	if (SInput::Instance().GetMouse(1))
	{
		Vector3 rot = actorRef.Target()->transform.GetRotation();

		rot.y += x * 0.1f;
		rot.x += y * 0.1f;

		actorRef.Target()->transform.SetRotation(rot);
		SetCursorPos(1920 / 2, 1080 / 2);
		GetCursorPos(&p);
	}


	pre = p;
}

void app::ControlCamera::Move()
{
	if (SInput::Instance().GetMouse(1))
	{
		const float speed = 5.0f * sf::Time::DeltaTime();

		Vector3 moveVec;

		Vector3 forward = WorldVector(0, 0, 1);
		Vector3 right = WorldVector(1, 0, 0);
		Vector3 up = WorldVector(0, 1, 0);


		if (SInput::Instance().GetKey(Key::KEY_W))
		{
			moveVec += forward * speed;
		}
		if (SInput::Instance().GetKey(Key::KEY_S))
		{
			moveVec += forward * -speed;
		}
		if (SInput::Instance().GetKey(Key::KEY_E))
		{
			moveVec += up * speed;
		}
		if (SInput::Instance().GetKey(Key::KEY_Q))
		{
			moveVec += up * -speed;
		}
		if (SInput::Instance().GetKey(Key::KEY_D))
		{
			moveVec += right * speed;
		}
		if (SInput::Instance().GetKey(Key::KEY_A))
		{
			moveVec += right * -speed;
		}

		auto v3 = actorRef.Target()->transform.GetPosition();
		v3 += moveVec;
		actorRef.Target()->transform.SetPosition(v3);
	}
}
