#include "Camera.h"
#include <DirectXMath.h>

using namespace DirectX;
sf::Camera* sf::Camera::main = nullptr;
sf::Camera* sf::Camera::shadow = nullptr;
VP sf::Camera::vp;




void sf::Camera::Begin()
{
	command.Bind(std::bind(&Camera::Update, this));
}

DirectX::XMMATRIX sf::Camera::GetViewMatrix() const
{
	Actor* actor = actorRef.Target();

	const Vector3& pos = actor->transform.GetPosition();

	DirectX::XMVECTOR eye{}, /*focus{},*/ up{};

	//視点
	eye = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0);

	//DirectX::XMVECTOR forward = WorldVector(0, 0, 1);

	//Vector3 forwardVec = Vector3(DirectX::XMVectorGetX(forward), DirectX::XMVectorGetY(forward), DirectX::XMVectorGetZ(forward));

	////注視点
	//focus = DirectX::XMVectorSet(pos.x + forwardVec.x, pos.y + forwardVec.y, pos.z + forwardVec.z, 0);


	// 注視点を決定
	DirectX::XMVECTOR focus;
	if (hasLookAt) {
		// LookAt() で指定されたターゲットを使う
		focus = DirectX::XMVectorSet(
			lookAtTarget.x,
			lookAtTarget.y,
			lookAtTarget.z,
			0
		);

	}
	else {
		// デフォルトはカメラの向いている「ワールド前方」
		DirectX::XMVECTOR forward = WorldVector(0, 0, 1);
		focus = eye + forward;

	}

	//上方向
	up = WorldVector(0, 1, 0);

	//ビュー変換行列
	DirectX::XMMATRIX matrixView = DirectX::XMMatrixLookAtLH(
		eye, focus, up);

	return matrixView;
}

DirectX::XMMATRIX sf::Camera::GetMatrixProj() const
{
	int w = 1920;
	int h = 1080;
	DirectX::XMMATRIX mtx = DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XMConvertToRadians(fov),
		float(w) / h,
		nearClip,	//near
		farClip);	//far

	return mtx;
}

Vector3 sf::Camera::WorldToScreen(Vector3 p) const
{
	DirectX::XMFLOAT3 a = p;
	DirectX::XMVECTOR viewPosition = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&a), GetViewMatrix());
	DirectX::XMVECTOR clipPosition = DirectX::XMVector3TransformCoord(viewPosition, GetMatrixProj());

	int screenWidth, screenHeight;

	//screenWidth = app::Application::GetWidth();
	//screenHeight = app::Application::GetHeight();

	//スクリーンサイズ補正は描画時に自動でされるのでデフォルトサイズを設定する
	screenWidth = 1920;
	screenHeight = 1080;

	Vector3 screenPosition;
	screenPosition.x = DirectX::XMVectorGetX(clipPosition) * screenWidth / 2.0f + screenWidth / 2.0f;
	screenPosition.y = -DirectX::XMVectorGetY(clipPosition) * screenHeight / 2.0f + screenHeight / 2.0f;
	screenPosition.z = DirectX::XMVectorGetZ(clipPosition);

	screenPosition.x -= screenWidth / 2.0f;
	screenPosition.y = screenHeight / 2.0f - screenPosition.y;

	return screenPosition;
}

void sf::Camera::SetGPU()
{
	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	dx11->vpBuffer.SetGPU(vp, dx11->GetMainDevice());
}

void sf::Camera::SetGPUShadow() const
{
	VP vpp;
	vpp.view = DirectX::XMMatrixTranspose(GetViewMatrix());
	vpp.proj = DirectX::XMMatrixTranspose(GetMatrixProj());

	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	dx11->vpBuffer.SetGPU(vpp, dx11->GetMainDevice());
}

void sf::Camera::Update()
{
	if (main == this)
	{
		Vector3 p = actorRef.Target()->transform.GetPosition();
		vp.cameraPos = DirectX::XMFLOAT4(p.x, p.y, p.z, 0);
		vp.view = DirectX::XMMatrixTranspose(GetViewMatrix());
		vp.proj = DirectX::XMMatrixTranspose(GetMatrixProj());
	}
	else if (shadow == this)
	{
		vp.shadowVP = DirectX::XMMatrixTranspose(GetViewMatrix() * GetMatrixProj());
	}
}

void sf::Camera::LookAt(const Vector3& target)
{
	lookAtTarget = target;
	hasLookAt = true;
}