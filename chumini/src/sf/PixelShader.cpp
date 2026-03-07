#include "PixelShader.h"
#include "DirectX11.h"

bool sf::dx::shader::PixelShader::lock = false;

sf::dx::shader::PixelShader::~PixelShader()
{
	Release();
}

void sf::dx::shader::PixelShader::Release()
{
	SAFE_RELEASE(s);
}

HRESULT sf::dx::shader::PixelShader::Make(void* pData, UINT size)
{
	Release();

	return DirectX11::Instance()->GetMainDevice().GetDevice()->CreatePixelShader(pData, size, NULL, &s);
}

void sf::dx::shader::PixelShader::SetGPU(ID3D11DeviceContext* d3dContext)
{
	if (!lock)
		d3dContext->PSSetShader(s, NULL, 0);
}

void sf::dx::shader::PixelShader::Lock()
{
	lock = true;
}

void sf::dx::shader::PixelShader::UnLock()
{
	lock = false;
}
