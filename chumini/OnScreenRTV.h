#pragma once
#include "RTV.h"

namespace sf
{
	namespace dx
	{
		//オンスクリーンレンダーターゲットクラス(D3D)
		class OnScreenRTV :public RTV
		{
		public:
			HRESULT Create();
		};
	}
}
