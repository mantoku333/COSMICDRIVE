#pragma once
#include "Device.h"
#include "Context.h"
#include "SwapChain.h"

namespace sf
{
	namespace dx
	{
		//メインデバイスクラス(D3D)
		class MainDevice :
			public Device,
			public Context,
			public SwapChain
		{
		public:
			HRESULT Create(HWND hwnd);

			operator ID3D11Device* ()const { return this->GetDevice(); }
			operator ID3D11DeviceContext* ()const { return this->GetContext(); }
			operator IDXGISwapChain* ()const { return this->GetSwapChain(); }
		};
	}
}
