#pragma once
#include "Mesh.h"
namespace sf
{
	//モーションと紐づけ可能なメッシュ
	class MotionMesh :public Mesh
	{
	public:
		void SetMotion(SafePtr<motion::Motion> _motion) { motion = _motion; }

	protected:
		DirectX::XMMATRIX WorldMatrix()const override;

	public:
		Transform offsetTransform;

	private:
		SafePtr<motion::Motion> motion;
	};
}