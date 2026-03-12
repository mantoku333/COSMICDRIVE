#include "ControlCamera.h"
#include "Config.h"

void app::ControlCamera::Begin()
{
	command.Bind(std::bind(&ControlCamera::Update, this));

	auto actor = actorRef.Target();
	if (!actor) return;

	auto cam = actor->GetComponent<sf::Camera>();
	if (cam)
	{
		//cam->LookAt({ 0.0f, 5.0f, 5.0f });         // 奥方向(Z+)を注視
	}

	//Vector3 eye = { 0.0f, 1.0f, 0.0f };   // カメラ位置
	//Vector3 rot = { -45.0f, 0.0f, 0.0f };    // ピッチ(下向き45°)

	// もし SetRotation がラジアン指定なら：
	// rot = { XMConvertToRadians(-45.0f), 0.0f, 0.0f };

	//actor->transform.SetPosition(eye);
	//actor->transform.SetRotation(rot);


	// ===== 初期位置と回転を設定 =====
	if (auto actor = actorRef.Target())
	{
		actor->transform.SetPosition(Vector3(0.0f, 3.0f, -20.0f));
		actor->transform.SetRotation(Vector3(40.0f, 0.0f, 0.0f));
	}

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
	if (!test::gGameConfig.enableDebugCamera) return;

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
	if (!test::gGameConfig.enableDebugCamera) return;

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

		if (SInput::Instance().GetKey(Key::KEY_UP))
		{
			moveVec.y += speed * 2;
		}
		if (SInput::Instance().GetKey(Key::KEY_DOWN))
		{
			moveVec.y -= speed * 2;
		}
		auto v3 = actorRef.Target()->transform.GetPosition();
		v3 += moveVec;
		actorRef.Target()->transform.SetPosition(v3);
	}
}
