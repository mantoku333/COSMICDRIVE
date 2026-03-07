#pragma once
#include "Collider.h"
namespace sf
{
	namespace col
	{
		//コライダーと衝突判定を取るレイクラス
		class Ray
		{
		public:
			/// <summary>
			/// レイの作成
			/// </summary>
			/// <param name="start">開始地点</param>
			/// <param name="end">終了地点</param>
			void CreateFromPoint(Vector3 _start, Vector3 _end);

			/// <summary>
			/// レイの作成
			/// </summary>
			/// <param name="start">開始地点</param>
			/// <param name="direction">終了地点</param>
			/// <param name="length">距離</param>
			void CreateFromDirection(Vector3 _start, Vector3 _direction, float _length);

			bool Intersects(HitInfo& info, LayerMask layer);

			/// <summary>
			/// 当たり判定処理
			/// </summary>
			/// <param name="other"></param>
			/// <param name="info"></param>
			/// <returns></returns>
			bool Intersect(ICollider* other, HitInfo& info)const;

			void Draw(DirectX::XMFLOAT4 color);

		private:
			DirectX::FXMVECTOR PointFXMVector()const;
			DirectX::FXMVECTOR DirFXMVector()const;

		private:
			Vector3 start;
			Vector3 direction;
			float length;
		};
	}
}