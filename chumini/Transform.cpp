#include "Transform.h"

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
