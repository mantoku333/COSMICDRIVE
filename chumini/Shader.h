#pragma once
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		namespace shader
		{
			//シェーダー基底クラス
			class Shader :public D3D
			{
			public:
				HRESULT LoadCSO(std::string fileName);

				virtual HRESULT Make(void* pData, UINT size) = 0;

				virtual void SetGPU(ID3D11DeviceContext* d3dContext) = 0;
			};
		}
	}
}