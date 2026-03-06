#include "Vector3.h"

const Vector2 Vector2::Zero = Vector2(0);
const Vector2 Vector2::One = Vector2(1, 1);
const Vector2 Vector2::Up = Vector2(0, 1);
const Vector2 Vector2::Down = Vector2(0, -1);
const Vector2 Vector2::Left = Vector2(-1, 0);
const Vector2 Vector2::Right = Vector2(1, 0);

Vector2::Vector2(float _x, float _y)
{
	x = _x;
	y = _y;
}

Vector2::Vector2(const DirectX::XMVECTOR& v)
{
	Convert(v);
}

float Vector2::Length() const
{
	return sqrt(x * x + y * y);
}

Vector2 Vector2::Normarize() const
{
	return *this / Length();
}

Vector2 Vector2::operator+(Vector2 v) const
{
	return Vector2(this->x + v.x, this->y + v.y);
}

Vector2 Vector2::operator-(Vector2 v) const
{
	return Vector2(this->x - v.x, this->y - v.y);
}

Vector2 Vector2::operator*(float v) const
{
	return Vector2(this->x * v, this->y * v);
}

Vector2 Vector2::operator/(float v) const
{
	return Vector2(this->x / v, this->y / v);
}

Vector2 Vector2::operator*(Vector2 v) const
{
	return Vector2(this->x * v.x, this->y * v.y);
}

Vector2 Vector2::operator/(Vector2 v) const
{
	return Vector2(this->x / v.x, this->y / v.y);
}

void Vector2::operator+=(Vector2 v)
{
	*this = (*this) + v;
}

void Vector2::operator-=(Vector2 v)
{
	*this = (*this) - v;
}

void Vector2::operator*=(float v)
{
	*this = (*this) * v;
}

void Vector2::operator/=(float v)
{
	*this = (*this) / v;
}

void Vector2::operator*=(Vector2 v)
{
	*this = (*this) * v;
}

void Vector2::operator/=(Vector2 v)
{
	*this = (*this) / v;
}

bool Vector2::operator==(Vector2 v) const
{
	return (this->x == v.x) && (this->y == v.y);
}

bool Vector2::operator!=(Vector2 v) const
{
	return (this->x != v.x) || (this->y != v.y);
}

void Vector2::operator=(const DirectX::XMVECTOR& v)
{
	Convert(v);
}

void Vector2::Convert(const DirectX::XMVECTOR& v)
{
	x = DirectX::XMVectorGetX(v);
	y = DirectX::XMVectorGetY(v);
}


const Vector3 Vector3::Zero = Vector3(0, 0, 0);
const Vector3 Vector3::One = Vector3(1, 1, 1);
const Vector3 Vector3::Forward = Vector3(0, 0, 1);
const Vector3 Vector3::Back = Vector3(0, 0, -1);
const Vector3 Vector3::Up = Vector3(0, 1, 0);
const Vector3 Vector3::Down = Vector3(0, -1, 0);
const Vector3 Vector3::Left = Vector3(-1, 0, 0);
const Vector3 Vector3::Right = Vector3(1, 0, 0);

Vector3::Vector3(float _x, float _y, float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

Vector3::Vector3(const DirectX::XMVECTOR& v)
{
	Convert(v);
}

float Vector3::Length() const
{
	return sqrt(x * x + y * y + z * z);
}

Vector3 Vector3::Normarize() const
{
	return *this / Length();
}

Vector3 Vector3::operator+(Vector3 v) const
{
	return Vector3(this->x + v.x, this->y + v.y, this->z + v.z);
}

Vector3 Vector3::operator-(Vector3 v) const
{
	return Vector3(this->x - v.x, this->y - v.y, this->z - v.z);
}

Vector3 Vector3::operator*(float v) const
{
	return Vector3(this->x * v, this->y * v, this->z * v);
}

Vector3 Vector3::operator/(float v) const
{
	return Vector3(this->x / v, this->y / v, this->z / v);
}

Vector3 Vector3::operator*(Vector3 v) const
{
	return Vector3(this->x * v.x, this->y * v.y, this->z * v.z);
}

Vector3 Vector3::operator/(Vector3 v) const
{
	return Vector3(this->x / v.x, this->y / v.y, this->z / v.z);
}

void Vector3::operator+=(Vector3 v)
{
	*this = (*this) + v;
}

void Vector3::operator-=(Vector3 v)
{
	*this = (*this) - v;
}

void Vector3::operator*=(float v)
{
	*this = (*this) * v;
}

void Vector3::operator/=(float v)
{
	*this = (*this) / v;
}

void Vector3::operator*=(Vector3 v)
{
	*this = (*this) * v;
}

void Vector3::operator/=(Vector3 v)
{
	*this = (*this) / v;
}

bool Vector3::operator==(Vector3 v) const
{
	return (this->x == v.x) && (this->y == v.y) && (this->z == v.z);
}

bool Vector3::operator!=(Vector3 v) const
{
	return (this->x != v.x) || (this->y != v.y) || (this->z != v.z);
}

void Vector3::operator=(const DirectX::XMVECTOR& v)
{
	Convert(v);
}

Vector3 Vector3::GetTranslation(const DirectX::XMMATRIX& mtx)
{
	return Vector3(
		mtx.r[3].m128_f32[0],
		mtx.r[3].m128_f32[1],
		mtx.r[3].m128_f32[2]
	);
}

Vector3 Vector3::GetScalling(const DirectX::XMMATRIX& mtx)
{
	return Vector3(
		mtx.r[0].m128_f32[0],
		mtx.r[1].m128_f32[1],
		mtx.r[2].m128_f32[2]
	);
}

void Vector3::Convert(const DirectX::XMVECTOR& v)
{
	x = DirectX::XMVectorGetX(v);
	y = DirectX::XMVectorGetY(v);
	z = DirectX::XMVectorGetZ(v);
}
