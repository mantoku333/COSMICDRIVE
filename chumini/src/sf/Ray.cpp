#include "Ray.h"
#include "BoxCollider.h"
#include "SphereCollider.h"

void sf::col::Ray::CreateFromPoint(Vector3 _start, Vector3 _end)
{
	start = _start;

	Vector3 vec = _end - _start;

	direction = vec.Normarize();

	length = vec.Length();
}

void sf::col::Ray::CreateFromDirection(Vector3 _start, Vector3 _direction, float _length)
{
	start = _start;

	direction = _direction;

	length = _length;
}

bool sf::col::Ray::Intersects(HitInfo& info, LayerMask layer)
{
	bool result = false;
	{
		std::lock_guard<std::mutex> lock(ICollider::mtx);
		for (auto& i : ICollider::GetColliders())
		{
			if (i.IsValid())
				if (i.Target()->layer.Hit(layer))
				{
					HitInfo mInfo;

					if (Intersect(i.Target(), mInfo))
					{
						if ((info.distance < 0) ||
							(mInfo.distance < info.distance))
						{
							info = mInfo;
						}
						result = true;
					}
				}
		}
	}

	return result;
}

bool sf::col::Ray::Intersect(ICollider* other, HitInfo& info) const
{
	bool result = false;
	float l = 0;
	if (typeid(*other) == typeid(BoxCollider))
	{
		const BoxCollider* col = static_cast<const BoxCollider*>(other);

		result = col->Get().Intersects(PointFXMVector(), DirFXMVector(), l);
	}
	else if (typeid(*other) == typeid(SphereCollider))
	{
		const SphereCollider* col = static_cast<const SphereCollider*>(other);

		result = col->Get().Intersects(PointFXMVector(), DirFXMVector(), l);
	}
	else if (typeid(*other) == typeid(BoneSphereCollider))
	{
		const BoneSphereCollider* col = static_cast<const BoneSphereCollider*>(other);

		result = col->Get().Intersects(PointFXMVector(), DirFXMVector(), l);
	}

	if (!result)
	{
		return false;
	}

	if (l > length)
	{
		return false;
	}

	info.a = nullptr;
	info.b = other;
	info.distance = l;
	info.hitPoint = start + direction * l;

	return true;
}

void sf::col::Ray::Draw(DirectX::XMFLOAT4 color)
{
	sf::debug::Debug::DrawLine(start, start + direction * length, color);
}

DirectX::FXMVECTOR sf::col::Ray::PointFXMVector() const
{
	return DirectX::XMVectorSet(start.x, start.y, start.z, 0);
}

DirectX::FXMVECTOR sf::col::Ray::DirFXMVector() const
{
	return  DirectX::XMVectorSet(direction.x, direction.y, direction.z, 0);
}
