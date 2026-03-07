#pragma once
#include "Collider.h"
#include "Motion.h"
namespace sf
{
	namespace col
	{
		//スフィアコライダークラス
		class SphereCollider :public ICollider
		{
		public:
			bool CheckHitCollider(ICollider* other, HitInfo& info)override;

			DirectX::BoundingSphere Get()const;

			virtual Vector3 GetCenter()const;
		protected:
			void DebugDraw()override;

		private:

			std::vector<Vector3> GetVertices()const;

		public:
			float radius = 0.5f;

			Vector3 offset = Vector3::Zero;

		private:
			static const int latitudeSegments = 6;
			static const int longitudeSegments = 6;
		};

		//ボーンに付随可能なスフィアコライダークラス
		class BoneSphereCollider :public SphereCollider
		{
		public:
			virtual void SetBone(sf::motion::MotionBone* _bone);

			Vector3 GetCenter()const override;

		private:
			sf::motion::MotionBone* bone = nullptr;
		};
	}
}