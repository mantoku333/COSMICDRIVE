#pragma once
#include "RTV.h"

namespace sf
{
	namespace dx
	{
		//オフスクリーンレンダーターゲットクラス(D3D)
		class OffScreenRTV :public RTV
		{
		public:
			~OffScreenRTV();

			void Release()override;

			HRESULT Create(
				int width,
				int height,
				DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);	//リソースのフォーマット

			ID3D11ShaderResourceView* GetSRV()const { return srv; }

			void SetTexture(ID3D11DeviceContext* d3dContext, int idx)const;

		private:
			ID3D11ShaderResourceView* srv = nullptr;
		};
	}
}
