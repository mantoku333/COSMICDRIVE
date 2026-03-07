#pragma once
#include "Shader.h"

namespace sf
{
	namespace dx
	{
		namespace shader
		{
			//ジオメトリシェーダー(D3D)
			class GeometryShader :public Shader
			{
			public:
				~GeometryShader();

				void Release()override;

				HRESULT Make(void* pData, UINT size)override;

				void SetGPU(ID3D11DeviceContext* d3dContext)override;

			private:
				ID3D11GeometryShader* s = nullptr;
			};
		}
	}
}