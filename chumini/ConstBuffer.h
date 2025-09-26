#pragma once
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		//定数バッファ基底クラス(D3D)
		class IConstBuffer :public D3D
		{
		public:
			~IConstBuffer();

			HRESULT Create(int slot, UINT size);

			void Set(int slot)const;

			void Release()override;

		protected:
			void SetGPUBase(const void* data, ID3D11DeviceContext* d3dContext)const;

		private:
			ID3D11Buffer* buffer = nullptr;

		};

		//定数バッファ(D3D)
		template<typename T>
		class ConstBuffer :public IConstBuffer
		{
		public:
			HRESULT Create(int slot)
			{
				return IConstBuffer::Create(slot, sizeof(T));
			}

			void SetGPU(const T& data, ID3D11DeviceContext* d3dContext)const
			{
				SetGPUBase(static_cast<const void*>(&data), d3dContext);
			}
			void SetGPU(const T* data, ID3D11DeviceContext* d3dContext)const
			{
				SetGPUBase(static_cast<const void*>(data), d3dContext);
			}
		};
	}
}