#include "Context.h"

sf::dx::Context::~Context()
{
	Release();
}

void sf::dx::Context::Release()
{
	SAFE_RELEASE(d3dContext);
}

void sf::dx::Context::SetContext(ID3D11DeviceContext* v)
{
	Release();

	d3dContext = v;
}

void sf::dx::Context::ClearRTV(ID3D11RenderTargetView* rtv)
{
	const FLOAT color[4]{ 0,0,0,1 };

	ClearRTV(rtv, color);
}

void sf::dx::Context::ClearRTV(ID3D11RenderTargetView* rtv, const FLOAT color[4])
{
	d3dContext->ClearRenderTargetView(rtv, color);
}

void sf::dx::Context::operator=(ID3D11DeviceContext* v)
{
	SetContext(v);
}
