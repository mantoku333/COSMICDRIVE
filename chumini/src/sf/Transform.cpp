#include "Transform.h"
#include <cmath>

DirectX::XMMATRIX sf::Transform::Matrix() const
{
	return matrix;
}

DirectX::XMMATRIX sf::Transform::GetRotationMatrix() const
{
	return DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(rotation.x)) *
		DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(rotation.y)) *
		DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(rotation.z));
}

void sf::Transform::SetPosition(const Vector3& other)
{
	position = other;

	CalcMatrix();
}

void sf::Transform::SetRotation(const Vector3& other)
{
	rotation = other;

	CalcMatrix();
}

void sf::Transform::SetScale(const Vector3& other)
{
	scale = other;

	CalcMatrix();
}

void sf::Transform::CalcMatrix()
{
	matrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
		DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(rotation.x)) *
		DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(rotation.y)) *
		DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(rotation.z)) *
		DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}

// --- 前方（Z+） ---
Vector3 sf::Transform::GetForward() const
{
    Vector3 rot = GetRotation();

    // ラジアン変換
    float radX = rot.x * 3.14159265f / 180.0f;
    float radY = rot.y * 3.14159265f / 180.0f;

    // Yaw (Y回転) と Pitch (X回転) の組み合わせで前方向を決定
    Vector3 forward;
    forward.x = std::sin(radY) * std::cos(radX);
    forward.y = -std::sin(radX);
    forward.z = std::cos(radY) * std::cos(radX);
    return forward.Normarize();
}

// --- 右方向（X+） ---
Vector3 sf::Transform::GetRight() const
{
    Vector3 rot = GetRotation();
    float radY = rot.y * 3.14159265f / 180.0f;

    Vector3 right;
    right.x = std::cos(radY);
    right.y = 0.0f;
    right.z = -std::sin(radY);
    return right.Normarize();
}

// --- 上方向（Y+） ---
Vector3 sf::Transform::GetUp() const
{
    // ForwardとRightの外積
    Vector3 f = GetForward();
    Vector3 r = GetRight();
    Vector3 up(
        f.y * r.z - f.z * r.y,
        f.z * r.x - f.x * r.z,
        f.x * r.y - f.y * r.x
    );
    return up.Normarize();
}