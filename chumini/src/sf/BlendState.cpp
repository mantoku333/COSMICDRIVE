#include "BlendState.h"
#include "DirectX11.h"

sf::dx::BlendState::~BlendState()
{
	Release();
}

HRESULT sf::dx::BlendState::Create(D3D11_BLEND blend)
{
	ID3D11Device* d3dDevice = DirectX11::Instance()->GetMainDevice();

	D3D11_BLEND_DESC bs;
	ZeroMemory(&bs, sizeof(D3D11_BLEND_DESC));
	bs.AlphaToCoverageEnable = FALSE;
	bs.IndependentBlendEnable = FALSE;
	bs.RenderTarget[0].BlendEnable = TRUE;
	bs.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bs.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bs.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bs.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bs.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bs.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bs.RenderTarget[0].DestBlend = blend;


	HRESULT hr = d3dDevice->CreateBlendState(&bs, &blendState);

	if (FAILED(hr))
	{
		throw std::runtime_error("ブレンドステート作成失敗");
	}

	return hr;
}

void sf::dx::BlendState::SetGPU(ID3D11DeviceContext* d3dContext) const
{
	d3dContext->OMSetBlendState(blendState, NULL, 0xffffffff);
}

void sf::dx::BlendState::Release()
{
	SAFE_RELEASE(blendState);
}
