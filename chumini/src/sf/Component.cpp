#include "Component.h"
#include "sf.h"

bool sf::Component::IsValid() const
{
	return actorRef.IsValid();
}

void sf::Component::Set(const Actor* other)
{
	actorRef = other;
}

DirectX::XMVECTOR sf::Component::WorldVector(float x, float y, float z) const
{
	// 回転行列を構築
	DirectX::XMMATRIX rotationMatrix = actorRef.Target()->transform.GetRotationMatrix();

	// 上方向ベクトルを計算
	DirectX::XMVECTOR upVector = DirectX::XMVectorSet(x, y, z, 0.0f); // ワールド空間での上方向
	upVector = DirectX::XMVector3TransformNormal(upVector, rotationMatrix); // 回転行列を適用

	return upVector;
}

Vector3 sf::Component::LookAt(Vector3 target) const
{
	Vector3 p = actorRef.Target()->transform.GetPosition();
	float dirX = target.x - p.x;
	float dirY = target.y - p.y;
	float dirZ = target.z - p.z;

	// 視線ベクトルの長さを計算 (必要なら正規化)
	double length = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);

	// ピッチ (上下回転)
	double pitch = std::atan2(dirY, std::sqrt(dirX * dirX + dirZ * dirZ));

	// ヨー (水平回転)
	double yaw = std::atan2(dirX, dirZ);

	return Vector3(DirectX::XMConvertToDegrees(-pitch), DirectX::XMConvertToDegrees(yaw), 0.0f);
}
