#pragma once
#include "RenderingBuffer.h"
namespace sf
{
	namespace rbuffer
	{
		//3D用レンダリングバッファ
		class RenderingBuffer3D :public RenderingBuffer
		{
		public:
			HRESULT Create(int w, int h);

			void ClearBuffer(ID3D11DeviceContext* d3dContext)override;
			void SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)override;


			const dx::OffScreenRTV& GetNormalRTV()const { return normalRTV; }

		private:
			//法線
			dx::OffScreenRTV normalRTV;
		};


		//3D用レンダリングダブルバッファバッファ
		class RenderingDoubleBuffer3D
		{
		public:
			HRESULT Create(int w, int h);

			void NextBuffer();

			void ClearBuffer(ID3D11DeviceContext* d3dContext);
			void SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth);

			/// <summary>
			/// 現在のレンダリングバッファを取得
			/// </summary>
			/// <returns></returns>
			RenderingBuffer3D& Get() { return buffers[idx]; }

			/// <summary>
			/// pre前のレンダリングバッファを取得
			/// </summary>
			/// <param name="pre"></param>
			/// <returns></returns>
			RenderingBuffer3D& Get(int pre);

			int GetIdx()const { return idx; }

		private:
			RenderingBuffer3D buffers[BUFFERSIZE];

			int idx = 0;
		};
	}
}