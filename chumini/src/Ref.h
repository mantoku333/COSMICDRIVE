#pragma once
#include <cstdint>
#include "AutoRef.h"
namespace sf
{
	namespace ref
	{
		namespace regacy
		{
			//リファレンス型(regacy)
			class Ref
			{
			public:
				Ref();

				/// <summary>
				/// リファレンスの取得
				/// </summary>
				/// <returns></returns>
				const uintptr_t& GetRef()const { return ref; }

			private:
				const uintptr_t ref;
			};
		}
	}
}