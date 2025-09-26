#pragma once
#include <vector>
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		//デバイス(D3D)
		class Device :public D3D
		{
		public:
			virtual ~Device();

			void Release()override;

			ID3D11DeviceContext* CreateDeferredContext()const;

			ID3D11Device* GetDevice()const { return d3dDevice; }

			/// <summary>
			/// 頂点バッファの作成
			/// </summary>
			/// <param name="vertex"></param>
			/// <param name="vSize"></param>
			/// <param name="byteSize"></param>
			/// <param name="vertexBuffer"></param>
			/// <returns></returns>
			HRESULT CreateVertexBuffer(const void* vertex, int vSize, int byteSize, ID3D11Buffer** vertexBuffer);

			/// <summary>
			/// インデックスバッファの作成
			/// </summary>
			/// <param name="index"></param>
			/// <param name="indexBuffer"></param>
			/// <returns></returns>
			HRESULT CreateIndexBuffer(const std::vector<WORD>& index, ID3D11Buffer** indexBuffer);

		protected:
			ID3D11Device* d3dDevice = nullptr;
		};
	}
}
