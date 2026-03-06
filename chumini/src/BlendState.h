#pragma once
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		//ブレンドステート(D3D)
		class BlendState :public D3D
		{
		public:
			~BlendState();

			HRESULT Create(D3D11_BLEND blend);

			void SetGPU(ID3D11DeviceContext* d3dContext)const;

			void Release()override;

		private:
			ID3D11BlendState* blendState = nullptr;
		};
	}
}