#include "Sampler.h"
#include "DirectX11.h"

sf::dx::Sampler::~Sampler()
{
	Release();
}

void sf::dx::Sampler::Release()
{
	SAFE_RELEASE(d3dSampler);
}

HRESULT sf::dx::Sampler::Create(D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v)
{
	/*
		---サンプラーのパラメータ解説---

	〇D3D11_TEXTURE_ADDRESS_WRAP:
	テクスチャ情報はUV座標に限らず存在し、正転を繰り返す

	〇D3D11_TEXTURE_ADDRESS_MIRROR:
	テクスチャ情報はUV座標に限らず存在し、正転,反転を繰り返す

	〇D3D11_TEXTURE_ADDRESS_CLAMP:
	テクスチャ情報はUV座標0.0f～1.0fのみ存在する

	〇D3D11_TEXTURE_ADDRESS_BORDER:
	UV座標0.0f～1.0fの範囲のみクリッピングされる。その他の領域は描画されない

	〇D3D11_TEXTURE_ADDRESS_MIRROR_ONCE:
	テクスチャ情報はUV座標-1.0f～1.0fに存在し、反転,正転して存在する
	*/

	D3D11_SAMPLER_DESC smpDesc;
	::ZeroMemory(&smpDesc, sizeof(D3D11_SAMPLER_DESC));
	smpDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	smpDesc.AddressU = u;		//U座標のパラメータ
	smpDesc.AddressV = v;		//V座標のパラメータ
	smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	HRESULT hr = DirectX11::Instance()->GetMainDevice().GetDevice()->CreateSamplerState(&smpDesc, &d3dSampler);

	if (FAILED(hr))
	{
		throw std::runtime_error("サンプラーの作成に失敗しました");
		return hr;
	}

	return hr;
}

void sf::dx::Sampler::SetGPU(ID3D11DeviceContext* d3dContext) const
{
	d3dContext->VSSetSamplers(0, 1, &d3dSampler);
	d3dContext->PSSetSamplers(0, 1, &d3dSampler);
	//d3dContext->GSSetSamplers(0, 1, &d3dSampler);
	//d3dContext->HSSetSamplers(0, 1, &d3dSampler);
	//d3dContext->DSSetSamplers(0, 1, &d3dSampler);
}
