#include "UI.h"
#include "App.h"
DirectX::XMMATRIX sf::ui::UI::GetMatrix() const
{
	float x = app::Application::GetMain()->width;
	float y = app::Application::GetMain()->height;

	//ウィンドウサイズに合わせてサイズ、位置を変える(ベースは1920×1080)
	x *= (1920.0f / x);
	y *= (1080.0f / y);

	DirectX::XMMATRIX windowMtx = DirectX::XMMatrixScaling(1.0f / x * 2.0f, 1.0f / y * 2.0f, 0);
	DirectX::XMMATRIX rect = DirectX::XMMatrixScaling(width / 2, height / 2, 0);
	DirectX::XMMATRIX ret = rect * transform.Matrix() * windowMtx;

	return ret;
}

void sf::ui::MovablePosition::DoMove(Vector3 _end, float _time, EASE _ease)
{
	start = t.GetPosition();
	end = _end;
	time = 0;
	limit = _time;

	ease = _ease;

	c.Bind(std::bind(&MovablePosition::UpdateMove, this, std::placeholders::_1));
}

void sf::ui::MovablePosition::UpdateMove(const command::ICommand& command)
{
	time += sf::Time::DeltaTime();

	if (time > limit)
	{
		time = limit;
		command.UnBind();
		End();
	}

	float r = time / limit;

	Vector3 v = sf::Lerp(start, end, r, ease);

	t.SetPosition(v);
}
