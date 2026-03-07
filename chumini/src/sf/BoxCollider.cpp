#include "BoxCollider.h"
#include "SphereCollider.h"

DirectX::BoundingBox sf::col::BoxCollider::Get()const
{
	DirectX::BoundingBox ret;

	ret.Transform(ret, DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) * actorRef.Target()->transform.Matrix());

	return ret;
}

bool sf::col::BoxCollider::CheckHitCollider(ICollider* other, HitInfo& info)
{
	bool hitFg = false;

	if (typeid(*other) == typeid(SphereCollider))
	{
		hitFg = dxHit::GetCollisionInfo(static_cast<SphereCollider*>(other)->Get(), this->Get(), info);
	}
	else if (typeid(*other) == typeid(BoxCollider))
	{
		//hitFg = dxHit::GetCollisionInfo(this->Get(), static_cast<BoxCollider*>(other)->Get(), info);
	}

	return hitFg;
}

void sf::col::BoxCollider::DebugDraw()
{
	std::vector<Vector3> edge = GetVertices();

	sf::debug::Debug::DrawLine(edge[0], edge[1], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[1], edge[3], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[3], edge[2], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[2], edge[0], DirectX::XMFLOAT4(0, 1, 0, 1));

	sf::debug::Debug::DrawLine(edge[0 + 4], edge[1 + 4], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[1 + 4], edge[3 + 4], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[3 + 4], edge[2 + 4], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[2 + 4], edge[0 + 4], DirectX::XMFLOAT4(0, 1, 0, 1));

	sf::debug::Debug::DrawLine(edge[0], edge[4], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[1], edge[5], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[2], edge[6], DirectX::XMFLOAT4(0, 1, 0, 1));
	sf::debug::Debug::DrawLine(edge[3], edge[7], DirectX::XMFLOAT4(0, 1, 0, 1));
}

std::vector<Vector3> sf::col::BoxCollider::GetVertices()const {
	// キューブのローカル座標での頂点 (サイズ 1x1x1)
	DirectX::XMFLOAT3 localVertices[8] = {
		{-0.5f, -0.5f, -0.5f}, // 左下前
		{ 0.5f, -0.5f, -0.5f}, // 右下前
		{-0.5f,  0.5f, -0.5f}, // 左上前
		{ 0.5f,  0.5f, -0.5f}, // 右上前
		{-0.5f, -0.5f,  0.5f}, // 左下後
		{ 0.5f, -0.5f,  0.5f}, // 右下後
		{-0.5f,  0.5f,  0.5f}, // 左上後
		{ 0.5f,  0.5f,  0.5f}, // 右上後
	};

	std::vector<Vector3> worldVertices(8);

	const Transform& t = actorRef.Target()->transform;

	Vector3 p = t.GetPosition();
	Vector3 s = t.GetScale();

	// スケール行列の作成
	DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(s.x, s.y, s.z);

	// 回転行列の作成 (オイラー角をクォータニオンに変換)
	DirectX::XMMATRIX rotationMatrix = t.GetRotationMatrix();

	// 平行移動行列の作成
	DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(p.x, p.y, p.z);

	// ワールド変換行列の作成
	DirectX::XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	// 各頂点をワールド座標に変換
	for (int i = 0; i < 8; ++i) {
		DirectX::XMVECTOR localPos = XMLoadFloat3(&localVertices[i]);
		DirectX::XMVECTOR worldPos = XMVector3Transform(localPos, worldMatrix);
		DirectX::XMFLOAT3 p;
		XMStoreFloat3(&p, worldPos);

		worldVertices[i] = Vector3(p.x, p.y, p.z);
	}

	return worldVertices;
}
