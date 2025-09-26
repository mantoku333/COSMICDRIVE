#pragma once
#include <functional>
#include <vector>
namespace sf
{
	namespace del
	{
		template<typename T>
		class DELEGATE
		{
		public:
			DELEGATE(T* _instance)
			{
				instance = _instance;
			}

			void operator=(void (T::* f)())
			{
				funcs.clear();
				funcs.push_back(f);
			}

			void operator+=(void (T::* f)())
			{
				funcs.push_back(f);
			}

			void operator-=(void (T::* f)())
			{
				auto it = std::find(funcs.begin(), funcs.end(), f);

				if (it != funcs.end())
				{
					funcs.erase(it);
				}
			}

			void operator()()
			{
				Invoke();
			}

			void Invoke()
			{
				for (auto& i : funcs) {
					(instance->*i)();
				}
			}

		private:
			T* instance;
			std::vector<void (T::*)()> funcs;
		};

		//デリゲート
		template<typename T = void>
		class Delegate
		{
		public:
			void Invoke(T v)
			{
				for (auto& i : func) {
					i(v);
				}
			}
			void operator()(T v)
			{
				Invoke(v);
			}

			void operator=(std::function<void(T)> f)
			{
				func.clear();
				func.push_back(f);
			}

			void operator+=(std::function<void(T)> f)
			{
				func.push_back(f);
			}
			void operator-=(std::function<void(T)> f)
			{
				auto it = std::find_if(func.begin(), func.end(), [&f](const std::function<void(T)>& v)
					{
						return (typeid(f).hash_code() == typeid(v).hash_code());
					});

				if (it != func.end())
				{
					func.erase(it);
				}
			}

		private:
			std::vector<std::function<void(T)>> func;
		};

		//デリゲート(戻り値無し)
		class VDelegate
		{
		public:
			void Invoke()
			{
				for (auto& i : func) {
					i();
				}
			}
			void operator()()
			{
				Invoke();
			}

			void operator=(std::function<void()> f)
			{
				func.clear();
				func.push_back(f);
			}

			void operator+=(std::function<void()> f)
			{
				func.push_back(f);
			}
			void operator-=(std::function<void()> f)
			{
				auto it = std::find_if(func.begin(), func.end(), [&f](const std::function<void()>& v)
					{
						return (typeid(f).hash_code() == typeid(v).hash_code());
					});

				if (it != func.end())
				{
					func.erase(it);
				}
			}

		private:
			std::vector<std::function<void()>> func;
		};


	}

	//#define Delegate(ClassName, DelegateName) sf::del::DELEGATE<ClassName> DelegateName = this
}