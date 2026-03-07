#pragma once
#include <DirectXMath.h>
#include "Easing.h"
namespace sf
{
	//01ï‚ä‘
	inline float Lerp01(float _min, float _max, float value)
	{
		if (_max == _min)return 0.0f;

		if (value < _min)return 0.0f;
		if (_max < value)return 1.0f;

		return (value - _min) / (_max - _min);
	}

	//ê¸å`ï‚ä‘
	template<typename T>
	T Lerp(T a, T b, float r)
	{
		return a + (b - a) * r;
	}

	//ê¸å`ï‚ä‘
	template<typename T>
	T Lerp(T a, T b, float r, EASE ease)
	{
		return a + (b - a) * Easing(r, ease);
	}
}

class Vector2
{
public:
	Vector2(float _x = 0.0f, float _y = 0.0f);
	Vector2(const DirectX::XMVECTOR& v);

	float Length()const;
	Vector2 Normarize()const;

public:
	Vector2 operator+(Vector2 v)const;
	Vector2 operator-(Vector2 v)const;
	Vector2 operator*(float v)const;
	Vector2 operator/(float v)const;
	Vector2 operator*(Vector2 v)const;
	Vector2 operator/(Vector2 v)const;

	void operator+=(Vector2 v);
	void operator-=(Vector2 v);
	void operator*=(float v);
	void operator/=(float v);
	void operator*=(Vector2 v);
	void operator/=(Vector2 v);

	bool operator==(Vector2 v)const;
	bool operator!=(Vector2 v)const;

	operator DirectX::XMFLOAT2()const { return DirectX::XMFLOAT2(x, y); }

	void operator=(const DirectX::XMVECTOR& v);

private:
	void Convert(const DirectX::XMVECTOR& v);

public:
	float x = 0.0f;
	float y = 0.0f;

public:
	static const Vector2 Zero;		//Vector2(0,0)
	static const Vector2 One;		//Vector2(1,1)
	static const Vector2 Up;		//Vector2(0,1)
	static const Vector2 Down;		//Vector2(0,-1)
	static const Vector2 Left;		//Vector2(-1,0)
	static const Vector2 Right;		//Vector2(1,0)
};

class Vector3
{
public:
	Vector3(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f);
	Vector3(const DirectX::XMVECTOR& v);

	float Length()const;
	Vector3 Normarize()const;

public:
	Vector3 operator+(Vector3 v)const;
	Vector3 operator-(Vector3 v)const;
	Vector3 operator*(float v)const;
	Vector3 operator/(float v)const;
	Vector3 operator*(Vector3 v)const;
	Vector3 operator/(Vector3 v)const;

	void operator+=(Vector3 v);
	void operator-=(Vector3 v);
	void operator*=(float v);
	void operator/=(float v);
	void operator*=(Vector3 v);
	void operator/=(Vector3 v);

	bool operator==(Vector3 v)const;
	bool operator!=(Vector3 v)const;

	operator DirectX::XMFLOAT3()const { return DirectX::XMFLOAT3(x, y, z); }

	void operator=(const DirectX::XMVECTOR& v);

	/// <summary>
	/// ïΩçsà⁄ìÆó ÇÃéÊìæ
	/// </summary>
	/// <param name="mtx"></param>
	/// <returns></returns>
	static Vector3 GetTranslation(const DirectX::XMMATRIX& mtx);

	/// <summary>
	/// ägëÂó ÇÃéÊìæ
	/// </summary>
	/// <param name="mtx"></param>
	/// <returns></returns>
	static Vector3 GetScalling(const DirectX::XMMATRIX& mtx);

private:
	void Convert(const DirectX::XMVECTOR& v);

public:
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

public:
	static const Vector3 Zero;		//Vector3(0,0,0)
	static const Vector3 One;		//Vector3(1,1,1)
	static const Vector3 Forward;	//Vector3(0,0,1)
	static const Vector3 Back;		//Vector3(0,0,-1)
	static const Vector3 Up;		//Vector3(0,1,0)
	static const Vector3 Down;		//Vector3(0,-1,0)
	static const Vector3 Left;		//Vector3(-1,0,0)
	static const Vector3 Right;		//Vector3(1,0,0)
};