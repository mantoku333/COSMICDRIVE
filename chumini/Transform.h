#pragma once
#include <DirectXMath.h>
#include "Vector3.h"

namespace sf
{
	//SRTèÓïÒÉNÉâÉX
	class Transform
	{
	public:
		DirectX::XMMATRIX Matrix()const;

		DirectX::XMMATRIX GetRotationMatrix()const;

		void SetPosition(const Vector3& other);
		const Vector3& GetPosition()const { return position; }

		void SetRotation(const Vector3& other);
		const Vector3& GetRotation()const { return rotation; }

		void SetScale(const Vector3& other);
		const Vector3& GetScale()const { return scale; }

		Vector3 GetForward() const;
		Vector3 GetRight() const;
		Vector3 GetUp() const;

	private:
		void CalcMatrix();

	private:
		Vector3 position = Vector3::Zero;
		Vector3 rotation = Vector3::Zero;
		Vector3 scale = Vector3::One;

		DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
	};
}
