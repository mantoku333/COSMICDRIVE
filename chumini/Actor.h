#pragma once
#include <list>
#include "SafePtr.h"
#include "Ref.h"
#include "Command.h"
#include "Transform.h"

#define USE_SMARTPTR	(true)

namespace sf
{
	class IScene;
	class Component;

	//アクタークラス(UnityのGameObject)
	class Actor :public ref::autoRef<sf::Actor>
	{
	public:
		Actor(IScene* _scene);
		~Actor();

		/// <summary>
		/// 削除予定のアクターをメモリから削除する
		/// </summary>
		static void DestroyActors();

		DirectX::XMVECTOR WorldVector(float x, float y, float z) const
		{
			// 回転行列を構築
			DirectX::XMMATRIX rotationMatrix = transform.GetRotationMatrix();

			// 上方向ベクトルを計算
			DirectX::XMVECTOR upVector = DirectX::XMVectorSet(x, y, z, 0.0f); // ワールド空間での上方向
			upVector = DirectX::XMVector3TransformNormal(upVector, rotationMatrix); // 回転行列を適用

			return upVector;
		}

		template<typename T>
		SafePtr<T> AddComponent()
		{
#if USE_SMARTPTR
			std::shared_ptr<T> ret = std::make_shared<T>();
			//Component* component = static_cast<Component*>(ret.get());
			ret->Set(this);
			if (activate)
				ret->Begin();
			components.push_back(ret);
			return ret.get();
#else
			T* ret = new T();
			Component* component = static_cast<Component*>(ret);
			component->Set(this);
			if (activate)
				component->Begin();
			components.push_back(ret);
			return ret;
#endif
		}


		template<typename T>
		T* GetComponent()
		{
			for (auto& i : components)
			{
#if USE_SMARTPTR
				if (typeid(T) == typeid(*(i.get())))
				{
					return static_cast<T*>(i.get());
				}
#else
				if (typeid(T) == typeid(*i))
				{
					return static_cast<T*>(i);
				}
#endif
			}
			return nullptr;
		}

		template<typename T>
		std::vector<T*> GetComponents()
		{
			std::vector<T*> ret;
			for (auto& i : components)
			{
				if (typeid(T) == typeid(*(i.get())))
				{
					ret.push_back(static_cast<T*>(i.get()));
				}
			}
			return ret;
		}

		void Activate();
		void DeActivate();

		bool GetActivate()const { return activate; }

		/// <summary>
		/// このアクターが存在するシーン
		/// </summary>
		/// <returns></returns>
		IScene& GetScene() { return scene; }

		void Destroy();

	public:
		Transform transform;

	private:
		IScene& scene;

		bool activate = false;

#if USE_SMARTPTR
		std::list<std::shared_ptr<Component>> components;
#else
		std::list<Component*> components;
#endif
		//全てのアクター
		static std::map<uintptr_t, Actor*> actors;

		static std::queue<sf::ref::Ref<sf::Actor>> destroyActors;
		static std::mutex destroyActorsMtx;

		friend IScene;
	};
}