#pragma once
#include "RenderingBuffer.h"
namespace sf
{
	namespace rbuffer
	{
		//2D用レンダリングバッファ
		class RenderingBuffer2D :public RenderingBuffer
		{
		public:
			HRESULT Create(int w, int h);

			void ClearBuffer(ID3D11DeviceContext* d3dContext)override;
			void SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)override;
		};

		//2D用レンダリングダブルバッファバッファ
		class RenderingDoubleBuffer2D
		{
		public:
			HRESULT Create(int w, int h);

			void NextBuffer();
			void ClearBuffer(ID3D11DeviceContext* d3dContext);
			void SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth);

			RenderingBuffer2D& Get() { return buffers[idx]; }
			RenderingBuffer2D& GetPrev();
			RenderingBuffer2D& Get(int i) { return buffers[i]; }

			int GetIdx()const { return idx; }

		private:
			RenderingBuffer2D buffers[BUFFERSIZE];

			int idx = 0;
		};
	}
}