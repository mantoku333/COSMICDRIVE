#include "MotionMesh.h"
#include "GUI.h"

DirectX::XMMATRIX sf::MotionMesh::WorldMatrix() const
{
	DirectX::XMMATRIX mtx =
		offsetTransform.Matrix() *
		motion->motionBone_LeftHand->GetBoneMatrix() *
		actorRef.Target()->transform.Matrix();


	return mtx;
}
