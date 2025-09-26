#include "GeometryShader.h"
#include "DirectX11.h"

sf::dx::shader::GeometryShader::~GeometryShader()
{
	Release();
}

void sf::dx::shader::GeometryShader::Release()
{
	SAFE_RELEASE(s);
}

HRESULT sf::dx::shader::GeometryShader::Make(void* pData, UINT size)
{
	Release();

	return DirectX11::Instance()->GetMainDevice().GetDevice()->CreateGeometryShader(pData, size, NULL, &s);
}

void sf::dx::shader::GeometryShader::SetGPU(ID3D11DeviceContext* d3dContext)
{
	d3dContext->GSSetShader(s, NULL, 0);
}
