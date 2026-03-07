#pragma once
#include "Debug.h"

namespace sf
{
	//Nullptrを自動で検出することが出来る、生ポインタの代替クラス
	template<typename T>
	class SafePtr
	{
	public:
		SafePtr() : ptr(nullptr) {}
		SafePtr(T* p) : ptr(p) {}

		SafePtr(const SafePtr& other) : ptr(other.ptr) {}

		//SafePtrとの代入演算子
		SafePtr& operator=(const SafePtr& other)
		{
			ptr = other.Get();
			return *this;
		}

		//継承関係のあるSafePtrの初期化処理
		template<typename U>
		SafePtr(const SafePtr<U>& other) : ptr(other.Get()) {}

		//継承関係のあるSafePtr同士の代入演算子
		template<typename U>
		SafePtr& operator=(const SafePtr<U>& other)
		{
			ptr = other.Get();
			return *this;
		}

		//継承関係のあるSafePtrをUへキャストする関数
		template<typename U>
		U* Cast()const
		{
			if (ptr == nullptr)
			{
				return nullptr;
			}

			return static_cast<U*>(ptr);
		}

		//ポインタへのアクセス(nullチェック)
		T* operator->() const
		{
			if (ptr == nullptr)
			{
				debug::Debug::LogError("Nullptrを検出しました");
				throw std::out_of_range("範囲外エラー");
			}
			return ptr;
		}

		//参照へのアクセス(nullチェック)
		T& operator*() const
		{
			if (ptr == nullptr)
			{
				debug::Debug::LogError("Nullptrを検出しました");
				throw std::out_of_range("範囲外エラー");

				static T t{};
				return t;
			}

			return *ptr;
		}

		//同ポインタ比較
		bool operator==(const SafePtr& other)const
		{
			return this->Get() == other.Get();
		}

		//nullかどうか
		bool isNull() const { return ptr == nullptr; }

		~SafePtr() {}

		//ポインタの取得
		T* Get()const { return ptr; }

	private:
		//ポインタの実体
		T* ptr = nullptr;
	};
}
