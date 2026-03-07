#include "VertexShader.h"
#include "DirectX11.h"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

sf::dx::shader::VertexShader::~VertexShader()
{
	Release();
}

void sf::dx::shader::VertexShader::Release()
{
	SAFE_RELEASE(s);
	SAFE_RELEASE(inputLayout);
}

HRESULT sf::dx::shader::VertexShader::Make(void* pData, UINT size)
{
	Release();

	HRESULT hr = DirectX11::Instance()->GetMainDevice().GetDevice()->CreateVertexShader(pData, size, NULL, &s);

	if (FAILED(hr))
	{
		return hr;
	}

	return MakeInputLayout(pData, size);
}

HRESULT sf::dx::shader::VertexShader::MakeInputLayout(void* pData, UINT size)
{
	/*
	シェーダ作成時にシェーダリフレクションを通してインプットレイアウトを取得
	セマンティクスの配置などから識別子を作成
	識別子が登録済→再利用、なければ新規作成
	https://blog.techlab-xe.net/dxc-shader-reflection/
	*/

	ID3D11ShaderReflection* pReflection;
	D3D11_SHADER_DESC shaderDesc;
	D3D11_INPUT_ELEMENT_DESC* pInputDesc;
	D3D11_SIGNATURE_PARAMETER_DESC sigDesc;

	DXGI_FORMAT formats[][4] =
	{
		{
			DXGI_FORMAT_R32_UINT,
			DXGI_FORMAT_R32G32_UINT,
			DXGI_FORMAT_R32G32B32_UINT,
			DXGI_FORMAT_R32G32B32A32_UINT,
		}, {
			DXGI_FORMAT_R32_SINT,
			DXGI_FORMAT_R32G32_SINT,
			DXGI_FORMAT_R32G32B32_SINT,
			DXGI_FORMAT_R32G32B32A32_SINT,
		}, {
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_R32G32_FLOAT,
			DXGI_FORMAT_R32G32B32_FLOAT,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
		},
	};

	HRESULT hr = D3DReflect(pData, size, IID_PPV_ARGS(&pReflection));
	if (FAILED(hr)) { return hr; }

	pReflection->GetDesc(&shaderDesc);

	//インプットパラメータがないなら、インプットレイアウトを作成しない
	if (shaderDesc.InputParameters == 0)
	{
		return hr;
	}

	pInputDesc = new D3D11_INPUT_ELEMENT_DESC[shaderDesc.InputParameters];
	for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
	{
		pReflection->GetInputParameterDesc(i, &sigDesc);
		pInputDesc[i].SemanticName = sigDesc.SemanticName;
		pInputDesc[i].SemanticIndex = sigDesc.SemanticIndex;

		// http://marupeke296.com/TIPS_No17_Bit.html
		BYTE elementCount = sigDesc.Mask;
		elementCount = (elementCount & 0x05) + ((elementCount >> 1) & 0x05);
		elementCount = (elementCount & 0x03) + ((elementCount >> 2) & 0x03);

		switch (sigDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_UINT32:
			pInputDesc[i].Format = formats[0][elementCount - 1];
			break;
		case D3D_REGISTER_COMPONENT_SINT32:
			pInputDesc[i].Format = formats[1][elementCount - 1];
			break;
		case D3D_REGISTER_COMPONENT_FLOAT32:
			pInputDesc[i].Format = formats[2][elementCount - 1];
			break;
		}

		//POSITIONのフォーマットはこれでないといけない
		if (i == 0)
		{
			pInputDesc[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}

		pInputDesc[i].InputSlot = 0;
		pInputDesc[i].AlignedByteOffset = i == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		pInputDesc[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		pInputDesc[i].InstanceDataStepRate = 0;
	}

	hr = DirectX11::Instance()->GetMainDevice().GetDevice()->CreateInputLayout(
		pInputDesc, shaderDesc.InputParameters,
		pData, size, &inputLayout
	);

	delete[] pInputDesc;

	return S_OK;
}

void sf::dx::shader::VertexShader::SetGPU(ID3D11DeviceContext* d3dContext)
{
	d3dContext->VSSetShader(s, NULL, 0);
	d3dContext->IASetInputLayout(inputLayout);
}
