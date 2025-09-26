#include "OffScreenRTV.h"
#include "DirectX11.h"
#include "sf.h"

sf::dx::OffScreenRTV::~OffScreenRTV()
{
	Release();
}

void sf::dx::OffScreenRTV::Release()
{
	SAFE_RELEASE(rtv);
	SAFE_RELEASE(srv);
}

HRESULT sf::dx::OffScreenRTV::Create(int width, int height, DXGI_FORMAT format)
{
	SAFE_RELEASE(rtv);

	ID3D11Device* d3dDevice = sf::dx::DirectX11::Instance()->GetMainDevice().GetDevice();

	ID3D11Texture2D* _pTexture = nullptr;

	D3D11_TEXTURE2D_DESC rtDesc;
	memset(&rtDesc, 0, sizeof(rtDesc));
	rtDesc.Width = width;
	rtDesc.Height = height;
	rtDesc.MipLevels = 1;
	rtDesc.Format = format;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.ArraySize = 1;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;

	HRESULT hr = d3dDevice->CreateTexture2D(&rtDesc, 0, &_pTexture);
	if (FAILED(hr))
	{
		throw std::runtime_error("オフスクリーンレンダーターゲットビューの作成に失敗しました");
		return hr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = rtDesc.MipLevels;
	hr = d3dDevice->CreateShaderResourceView(_pTexture, &srvDesc, &srv);
	if (FAILED(hr))
	{
		throw std::runtime_error("オフスクリーンレンダーターゲットビューの作成に失敗しました");
		return hr;
	}

	hr = d3dDevice->CreateRenderTargetView(_pTexture, NULL, &rtv);
	if (FAILED(hr))
	{
		throw std::runtime_error("オフスクリーンレンダーターゲットビューの作成に失敗しました");
		return hr;
	}

	SAFE_RELEASE(_pTexture);


	sf::debug::Debug::LogSafety("オフスクリーンレンダーターゲットビューの作成に成功しました");

	return S_OK;
}

void sf::dx::OffScreenRTV::SetTexture(ID3D11DeviceContext* d3dContext, int idx) const
{
	d3dContext->PSSetShaderResources(idx, 1, &srv);
}
