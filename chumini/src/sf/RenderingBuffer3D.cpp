#include "RenderingBuffer3D.h"
#include "Debug.h"

HRESULT sf::rbuffer::RenderingBuffer3D::Create(int w, int h)
{
	HRESULT hr = baseColorRTV.Create(w, h);

	if (FAILED(hr))return hr;

	hr = luminanceRTV.Create(w, h);

	if (FAILED(hr))return hr;

	hr = normalRTV.Create(w, h);

	if (FAILED(hr))return hr;

	hr = alphaRTV.Create(w, h);

	if (FAILED(hr))return hr;

	return hr;
}

void sf::rbuffer::RenderingBuffer3D::ClearBuffer(ID3D11DeviceContext* d3dContext)
{
	const FLOAT black[4]{ 0,0,0,1 };

	d3dContext->ClearRenderTargetView(baseColorRTV.GetRTV(), black);
	d3dContext->ClearRenderTargetView(normalRTV.GetRTV(), black);
	d3dContext->ClearRenderTargetView(alphaRTV.GetRTV(), black);
	d3dContext->ClearRenderTargetView(luminanceRTV.GetRTV(), black);
}

void sf::rbuffer::RenderingBuffer3D::SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)
{
	ID3D11RenderTargetView* rtv[]
	{
		baseColorRTV.GetRTV(),
		normalRTV.GetRTV(),
		alphaRTV.GetRTV(),
		luminanceRTV.GetRTV(),
	};

	d3dContext->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, depth);
}

HRESULT sf::rbuffer::RenderingDoubleBuffer3D::Create(int w, int h)
{
	HRESULT hr;

	for (int i = 0; i < BUFFERSIZE; i++)
	{
		HRESULT hr = buffers[i].Create(w, h);

		if (FAILED(hr))
		{
			return hr;
		}
	}

	return S_OK;
}

void sf::rbuffer::RenderingDoubleBuffer3D::NextBuffer()
{
	//次のバッファ
	idx++;
	idx %= BUFFERSIZE;
}

void sf::rbuffer::RenderingDoubleBuffer3D::ClearBuffer(ID3D11DeviceContext* d3dContext)
{
	Get().ClearBuffer(d3dContext);
}

void sf::rbuffer::RenderingDoubleBuffer3D::SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)
{
	ID3D11ShaderResourceView* srv[]
	{
		nullptr,
		nullptr,
		Get(1).GetBaseColorRTV().GetSRV(),
	};
	d3dContext->PSSetShaderResources(0, ARRAYSIZE(srv), srv);
	Get().SetRBuffer(d3dContext, depth);
}

sf::rbuffer::RenderingBuffer3D& sf::rbuffer::RenderingDoubleBuffer3D::Get(int pre)
{
	int i = idx - pre;

	while (i < 0)
	{
		i += BUFFERSIZE;
	}

	return buffers[i];
}
