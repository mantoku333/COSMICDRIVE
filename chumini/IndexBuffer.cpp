#include "IndexBuffer.h"
#include "DirectX11.h"
sf::dx::IndexBuffer::~IndexBuffer()
{
	Release();
}

void sf::dx::IndexBuffer::Release()
{
	SAFE_RELEASE(buffer);
}

HRESULT sf::dx::IndexBuffer::Create(const std::vector<WORD>& indcies)
{
	iSize = indcies.size();

	return DirectX11::Instance()->GetMainDevice().CreateIndexBuffer(indcies, &buffer);
}

void sf::dx::IndexBuffer::SetGPU(ID3D11DeviceContext* d3dContext) const
{
	d3dContext->IASetIndexBuffer(buffer, DXGI_FORMAT_R16_UINT, 0);
}
