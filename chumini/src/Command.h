#pragma once
#include <functional>
#include <mutex>
#include <map>
#include "Ref.h"
#include "JobSystem.h"
namespace sf
{
	namespace command
	{

#define COMMANDIDX_MAX	(8)

		//更新コマンド基底クラス
		class ICommand :public ref::autoRef<ICommand>
		{
		public:
			ICommand(unsigned int order);
			~ICommand();

			void Bind(std::function<void(const ICommand&)> function);
			void UnBind() const;

			unsigned int GetUpdateOrder()const { return updateOrder; }

			void Call()const;

			static void CallAll();

		private:
			std::function<void(const ICommand&)> fun = [](const ICommand&) {};

			unsigned int updateOrder = 0;

			static std::map<uintptr_t, ICommand*> commands[COMMANDIDX_MAX];
			static std::mutex commandMapMtx;
			static jobsystem::JobSystem commandJob;
		};

		/// <summary>
		/// 更新コマンド
		/// </summary>
		/// <typeparam name="UpdateOrder"></typeparam>
		template<unsigned int UpdateOrder = 1>
		class Command :public ICommand
		{
		public:
			Command() :ICommand(UpdateOrder) {}
		};
	}
}