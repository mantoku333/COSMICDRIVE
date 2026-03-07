#pragma once
#include "OffScreenRTV.h"

#define BUFFERSIZE	(4)

namespace sf
{
	namespace rbuffer
	{
		//レンダリングバッファ基底クラス
		class RenderingBuffer
		{
		public:
			virtual ~RenderingBuffer() {}

			//RBufferの塗りつぶし
			virtual void ClearBuffer(ID3D11DeviceContext* d3dContext) = 0;

			//RBufferのセット
			virtual void SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth) = 0;

		public:
			const dx::OffScreenRTV& GetBaseColorRTV()const { return baseColorRTV; }
			const dx::OffScreenRTV& GetLuminanceRTV()const { return luminanceRTV; }
			const dx::OffScreenRTV& GetAlphaRTV()const { return alphaRTV; }

		protected:
			//基本色
			dx::OffScreenRTV baseColorRTV;

			//輝度抽出
			dx::OffScreenRTV luminanceRTV;

			//αレンダーターゲット
			dx::OffScreenRTV alphaRTV;
		};
	}
}