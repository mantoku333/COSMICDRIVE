#include "Collider.h"

std::vector<sf::ref::Ref<sf::col::ICollider>> sf::col::ICollider::cols;
std::mutex sf::col::ICollider::mtx;
std::mutex sf::col::ICollider::deleteMtx;
std::vector<sf::ref::Ref<sf::col::ICollider>> sf::col::ICollider::deleteCols;

sf::col::ICollider::ICollider()
{
	{
		//std::lock_guard<std::mutex> guard(mtx);
		//cols.push_back(this);
	}
}

sf::col::ICollider::~ICollider()
{
	{
		std::lock_guard<std::mutex> guard(deleteMtx);
		Remove();
	}
}

void sf::col::ICollider::Activate()
{
	{
		std::lock_guard<std::mutex> guard(mtx);
		cols.push_back(this);
	}
#if DrawCollider
	debugCommand.Bind(std::bind(&ICollider::DebugDraw, this));
#endif
}

void sf::col::ICollider::DeActivate()
{
	{
		std::lock_guard<std::mutex> guard(deleteMtx);
		Remove();
	}

#if DrawCollider
	debugCommand.UnBind();
#endif
}

void sf::col::ICollider::DeleteColliders()
{
	while (!deleteCols.empty())
	{
		ref::Ref<ICollider> ref = deleteCols.back();
		deleteCols.pop_back();

		auto it = std::find_if(cols.begin(), cols.end(), [ref](ref::Ref<ICollider> v)
			{
				return v == ref;
			});

		{
			std::lock_guard<std::mutex> guard(mtx);
			if (it != cols.end())
			{
				cols.erase(it);
			}
		}
	}
}

void sf::col::ICollider::Check(ICollider* other)
{
	HitInfo info;

	bool hitFg = CheckHitCollider(other, info);

	//当たり判定の変化があれば
	if (hitFg != this->FindHitList(other))
	{
		if (call && other->call)
			if (hitFg)
			{
				this->AddHitList(other);
				other->AddHitList(this);

				info.a = this;
				info.b = other;
				this->onCollisionEnter(info);

				info.a = other;
				info.b = this;
				other->onCollisionEnter(info);
			}
			else
			{
				this->RemoveHitList(other);
				other->RemoveHitList(this);

				info.a = this;
				info.b = other;
				this->onCollisionExit(info);

				info.a = other;
				info.b = this;
				other->onCollisionExit(info);
			}
	}
}

void sf::col::ICollider::AddHitList(ICollider* other)
{
	hitList.push_back(other);
}

void sf::col::ICollider::RemoveHitList(ICollider* other)
{
	auto it = std::find(hitList.begin(), hitList.end(), other);

	if (it != hitList.end())
	{
		hitList.erase(it);
	}
}

bool sf::col::ICollider::FindHitList(ICollider* other)
{
	auto it = std::find(hitList.begin(), hitList.end(), other);

	return (it != hitList.end());
}

void sf::col::ICollider::Remove()
{
	deleteCols.push_back(this);

	for (auto& i : hitList) {
		//アクターが存在すれば
		if (i.IsValid())
			i.Target()->RemoveHitList(this);
	}
	return;
	{
		auto it = std::find(cols.begin(), cols.end(), this);

		if (it != cols.end())
		{
			cols.erase(it);
		}
	}
}

bool sf::col::dxHit::GetCollisionInfo(const BoundingSphere& sphere, const BoundingBox& box, HitInfo& hitInfo)
{
	// 球の中心
	XMVECTOR sphereCenter = XMLoadFloat3(&sphere.Center);

	// AABB の最小・最大座標を計算
	XMVECTOR boxCenter = XMLoadFloat3(&box.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&box.Extents);

	XMVECTOR boxMin = XMVectorSubtract(boxCenter, boxExtents);
	XMVECTOR boxMax = XMVectorAdd(boxCenter, boxExtents);

	// 最近接点を計算
	XMVECTOR closestPoint = XMVectorClamp(sphereCenter, boxMin, boxMax);

	// 最近接点と球の中心の距離を計算
	XMVECTOR delta = XMVectorSubtract(closestPoint, sphereCenter);
	float distSquared = XMVectorGetX(XMVector3LengthSq(delta));

	// 衝突しているか確認
	if (distSquared <= (sphere.Radius * sphere.Radius)) {
		// 衝突している場合、最近接点が衝突点となる
		DirectX::XMFLOAT3 p = hitInfo.hitPoint;
		XMStoreFloat3(&p, closestPoint);
		hitInfo.distance = sqrtf(distSquared);

		//補正値を求めるにはこの計算が必要
		hitInfo.distance = (1 - hitInfo.distance) - sphere.Radius;
		//hitInfo.distance = -hitInfo.distance + sphere.Radius;

		return true;
	}

	// 衝突していない場合
	hitInfo.hitPoint = Vector3(0.0f, 0.0f, 0.0f);
	hitInfo.distance = -1.0f; // 衝突していない場合は -1.0f とする

	return false;
}

bool sf::col::dxHit::GetCollisionInfo(const BoundingSphere& sphere1, const BoundingSphere& sphere2, HitInfo& hitInfo)
{
	// 球の中心座標を読み込み
	XMVECTOR center1 = XMLoadFloat3(&sphere1.Center);
	XMVECTOR center2 = XMLoadFloat3(&sphere2.Center);

	// 中心間のベクトルと距離を計算
	XMVECTOR delta = XMVectorSubtract(center2, center1);
	float distSquared = XMVectorGetX(XMVector3LengthSq(delta));

	// 2つの球の半径の合計を計算
	float radiusSum = sphere1.Radius + sphere2.Radius;

	// 衝突しているかどうかの判定
	if (distSquared <= (radiusSum * radiusSum)) {
		// 衝突している場合、衝突点と距離を計算
		float distance = std::sqrt(distSquared);

		// 衝突点の計算
		XMVECTOR hitPoint = XMVectorAdd(center1, XMVectorScale(delta, sphere1.Radius / radiusSum));

		// `HitInfo` の更新
		DirectX::XMFLOAT3 p = hitInfo.hitPoint;
		XMStoreFloat3(&p, hitPoint);
		hitInfo.distance = distance;

		return true;
	}

	// 衝突していない場合
	hitInfo.hitPoint = Vector3(0.0f, 0.0f, 0.0f);
	hitInfo.distance = -1.0f;

	return false;
}

sf::col::LayerMask::LayerMask(unsigned int masks)
{
	Set(masks);
}

void sf::col::LayerMask::Set(unsigned int masks)
{
	layer = masks;
}

bool sf::col::LayerMask::Hit(const LayerMask& other) const
{
	//同じレイヤーの時、レイヤーの下位1ビットが1なら(同レイヤー当たり判定なら)
	if (layer == other.layer)
	{
		if (layer & 0b01)
		{
			return true;
		}
	}
	//レイヤーのビットが同じなら
	else if ((layer & other.layer))
	{
		return true;
	}

	return false;
}
