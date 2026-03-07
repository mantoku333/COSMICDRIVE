#pragma once
#include "D3D.h"

namespace sf
{
	namespace dx
	{
		//スワップチェイン(D3D)
		class SwapChain :public D3D
		{
		public:
			virtual ~SwapChain();

			void Release()override;

			IDXGISwapChain* GetSwapChain()const { return d3dSwapchain; }

			void Flip();

		protected:
			IDXGISwapChain* d3dSwapchain = nullptr;
		};
	}
}
