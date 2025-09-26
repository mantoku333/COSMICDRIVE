#pragma once
template<typename T>
class Singleton
{
public:
	static void Init()
	{
		if (instance == nullptr)
		{
			instance = new T();
		}
	}

	static void UnInit()
	{
		if (instance != nullptr)
		{
			delete instance;
			instance = nullptr;
		}
	}

	static T* Instance() { return instance; }

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

protected:
	Singleton() {}
	virtual ~Singleton() {}

private:
	static T* instance;
};

template<typename T>
T* Singleton<T>::instance = nullptr;

namespace sf
{
	//単一のコンポーネントを取得できるクラス(シングルトンではなく、簡易的にアクセスできるようにするためのクラス)
	template<typename T>
	class SingleComponent
	{
	public:
		SingleComponent()
		{
			instance = static_cast<T*>(this);
		}

		virtual ~SingleComponent()
		{
			instance = nullptr;
		}

		static T* Instance() { return instance; }

		SingleComponent(const SingleComponent&) = delete;
		SingleComponent& operator=(const SingleComponent&) = delete;
		SingleComponent(SingleComponent&&) = delete;
		SingleComponent& operator=(SingleComponent&&) = delete;

	private:
		static T* instance;
	};

	template<typename T>
	T* SingleComponent<T>::instance = nullptr;
}