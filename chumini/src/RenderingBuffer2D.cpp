#include "RenderingBuffer2D.h"
#include "Debug.h"

HRESULT sf::rbuffer::RenderingBuffer2D::Create(int w, int h)
{
	HRESULT hr = baseColorRTV.Create(w, h);

	if (FAILED(hr))return hr;

	hr = luminanceRTV.Create(w, h);

	if (FAILED(hr))return hr;

	hr = alphaRTV.Create(w, h);

	if (FAILED(hr))return hr;

	return hr;
}

void sf::rbuffer::RenderingBuffer2D::ClearBuffer(ID3D11DeviceContext* d3dContext)
{
	const FLOAT black[4]{ 0,0,0,1 };

	d3dContext->ClearRenderTargetView(baseColorRTV.GetRTV(), black);
	d3dContext->ClearRenderTargetView(alphaRTV.GetRTV(), black);
}

void sf::rbuffer::RenderingBuffer2D::SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)
{
	ID3D11RenderTargetView* rtv[]
	{
		baseColorRTV.GetRTV(),
		alphaRTV.GetRTV(),
	};

	d3dContext->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, depth);
}

HRESULT sf::rbuffer::RenderingDoubleBuffer2D::Create(int w, int h)
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

void sf::rbuffer::RenderingDoubleBuffer2D::NextBuffer()
{
	//次のバッファ
	idx++;
	idx %= BUFFERSIZE;
}

void sf::rbuffer::RenderingDoubleBuffer2D::ClearBuffer(ID3D11DeviceContext* d3dContext)
{
	Get().ClearBuffer(d3dContext);
}

void sf::rbuffer::RenderingDoubleBuffer2D::SetRBuffer(ID3D11DeviceContext* d3dContext, ID3D11DepthStencilView* depth)
{
	ID3D11ShaderResourceView* srv[]
	{
		nullptr
	};
	d3dContext->PSSetShaderResources(0, 1, srv);
	Get().SetRBuffer(d3dContext, depth);

	GetPrev().GetBaseColorRTV().SetTexture(d3dContext, 0);
}

sf::rbuffer::RenderingBuffer2D& sf::rbuffer::RenderingDoubleBuffer2D::GetPrev()
{
	int i = idx - 1;

	if (i < 0)
	{
		i = BUFFERSIZE - 1;
	}

	return buffers[i];
}
