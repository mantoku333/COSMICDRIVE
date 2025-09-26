#pragma once
#include "Component.h"
#include "Delegate.h"
#include <DirectXCollision.h>

#ifdef _DEBUG
#define DrawCollider	(true)
#else
#define DrawCollider	(false)
#endif

namespace sf
{
	namespace col
	{
		const unsigned int Mask_Same = 0b1 << 0;		//同コライダー衝突マスク
		const unsigned int Mask_Pad = 0b1 << 1;			//同レイヤー防止用パディング
		const unsigned int Mask_Area = 0b1 << 2;		//エリアマスク
		const unsigned int Mask_PlayerAtk = 0b1 << 3;	//プレイヤー攻撃マスク
		const unsigned int Mask_EnemyAtk = 0b1 << 4;	//敵攻撃マスク
		const unsigned int Mask_FieldHit = 0b1 << 5;	//フィールド衝突マスク
		const unsigned int Mask_Assist = 0b1 << 6;		//アシストマスク

		const unsigned int Layer_Default = Mask_Same;					//デフォルト
		const unsigned int Layer_PlayerHit = Mask_Area | Mask_EnemyAtk;	//プレイヤー被弾
		const unsigned int Layer_EnemyHit = Mask_PlayerAtk | Mask_Pad;	//敵被弾
		const unsigned int Layer_Area = Mask_Area;						//エリア用
		const unsigned int Layer_PlayerAtk = Mask_PlayerAtk;			//プレイヤー攻撃
		const unsigned int Layer_EnemyAtk = Mask_EnemyAtk;				//敵攻撃
		const unsigned int Layer_FieldHit = Mask_FieldHit | Mask_Same;	//キャラクター押し出し
		const unsigned int Layer_CharaPush = Mask_FieldHit | Mask_Same;	//キャラクター押し出し
		const unsigned int Layer_Assist = Mask_Assist;

		//レイヤーマスククラス
		class LayerMask
		{
		public:
			LayerMask() {}
			LayerMask(unsigned int masks);
			void Set(unsigned int masks);

			bool Hit(const LayerMask& other)const;

			unsigned int Get()const { return layer; }

		private:
			unsigned int layer = Layer_Default;
		};

		class ICollider;

		//衝突情報構造体
		struct HitInfo
		{
			//自身のコライダー
			ICollider* a = nullptr;

			//衝突相手のコライダー
			ICollider* b = nullptr;

			//衝突地点
			Vector3 hitPoint = Vector3::Zero;

			//距離
			float distance = -1.0f;
		};

		//当たり判定基底クラス
		class ICollider :public Component, public sf::ref::autoRef<ICollider>
		{
		public:
			ICollider();
			virtual ~ICollider();

			void Activate()override;
			void DeActivate()override;

			static void DeleteColliders();

			static const std::vector<sf::ref::Ref<ICollider>>& GetColliders() { return cols; }

			void Check(ICollider* other);

			virtual bool CheckHitCollider(ICollider* other, HitInfo& info) = 0;

			void AddHitList(ICollider* other);
			void RemoveHitList(ICollider* other);

			bool FindHitList(ICollider* other);

		protected:
			virtual void DebugDraw() = 0;

		private:
			void Remove();

		public:
			del::Delegate<HitInfo> onCollisionEnter;
			del::Delegate<HitInfo> onCollisionExit;

			LayerMask layer;

			bool call = true;

		protected:
			std::list<sf::ref::Ref<ICollider>> hitList;
		public:
			static std::vector<sf::ref::Ref<ICollider>> cols;
			static std::mutex mtx;
			static std::mutex deleteMtx;

		private:
			command::Command<> debugCommand;

		private:
			static std::vector<sf::ref::Ref<ICollider>> deleteCols;
		};

		namespace dxHit
		{
			using namespace DirectX;

			bool GetCollisionInfo(const BoundingSphere& sphere, const BoundingBox& box, HitInfo& hitInfo);

			bool GetCollisionInfo(const BoundingSphere& sphere1, const BoundingSphere& sphere2, HitInfo& hitInfo);
		}
	}
}