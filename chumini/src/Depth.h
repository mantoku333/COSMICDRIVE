#pragma once
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		//ê[ìx(D3D)
		class Depth :D3D
		{
		public:
			virtual ~Depth();

			virtual HRESULT Create(int x, int y);

			virtual void Release()override;

			operator ID3D11DepthStencilView* ()const { return depthStencilView; }

		protected:
			ID3D11Texture2D* depthStencilTexture = nullptr;
			ID3D11DepthStencilView* depthStencilView = nullptr;
		};
	}
}