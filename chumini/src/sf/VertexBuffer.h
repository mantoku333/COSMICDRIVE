#pragma once
#include <vector>
#include "D3D.h"
#include "DirectX11.h"
namespace sf
{
	namespace dx
	{
		//3D頂点構造体
		struct Vertex3D
		{
			DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0, 0, 0);
			DirectX::XMFLOAT3 nor = DirectX::XMFLOAT3(0, 0, 0);
			DirectX::XMFLOAT2 uv = DirectX::XMFLOAT2(0, 0);
			DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1, 1, 1, 1);
			int boneIndex[4]{};
			float boneWeight[4]{};
			int boneCnt = 0;
		};


		//頂点バッファ(D3D)
		template<typename T>
		class VertexBuffer :public D3D
		{
		public:
			~VertexBuffer()
			{
				Release();
			}

			void Release()override
			{
				SAFE_RELEASE(buffer);
			}

			/// <summary>
			/// 頂点バッファの作成
			/// </summary>
			/// <param name="vertices"></param>
			/// <returns></returns>
			HRESULT Create(const std::vector<T>& vertices)
			{
				return DirectX11::Instance()->GetMainDevice().CreateVertexBuffer(vertices.data(), vertices.size(), sizeof(T), &buffer);
			}

			void SetGPU(ID3D11DeviceContext* d3dContext)const
			{
				const UINT strides = sizeof(T);
				const UINT offsets = 0;
				d3dContext->IASetVertexBuffers(0, 1, &buffer, &strides, &offsets);
			}

		private:
			ID3D11Buffer* buffer = nullptr;
		};
	}
}