#include "ConstBuffer.h"
#include "DirectX11.h"
#include "Debug.h"

sf::dx::IConstBuffer::~IConstBuffer()
{
	Release();
}

HRESULT sf::dx::IConstBuffer::Create(int slot, UINT size)
{
	HRESULT hr{};

	// 定数バッファ作成
	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.ByteWidth = size;// 今から作る定数バッファのサイズ
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;// 定数バッファ作成を指定
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	ID3D11Device* d3dDevice = DirectX11::Instance()->GetMainDevice();
	ID3D11DeviceContext* d3dContext = DirectX11::Instance()->GetMainDevice();

	hr = d3dDevice->CreateBuffer(&cbDesc, NULL, &buffer);

	if (FAILED(hr))
	{
		throw std::runtime_error("定数バッファ作成に失敗しました");

		return hr;
	}

	sf::debug::Debug::LogSafety("定数バッファ作成に成功しました");

	Set(slot);

	return hr;
}

void sf::dx::IConstBuffer::Set(int slot) const
{
	ID3D11DeviceContext* d3dContext = DirectX11::Instance()->GetMainDevice();

	d3dContext->CSSetConstantBuffers(slot, 1, &buffer);
	d3dContext->VSSetConstantBuffers(slot, 1, &buffer);
	d3dContext->GSSetConstantBuffers(slot, 1, &buffer);
	d3dContext->PSSetConstantBuffers(slot, 1, &buffer);
}

void sf::dx::IConstBuffer::Release()
{
	SAFE_RELEASE(buffer);
}

void sf::dx::IConstBuffer::SetGPUBase(const void* data, ID3D11DeviceContext* d3dContext) const
{
	d3dContext->UpdateSubresource(buffer, 0, NULL, data, 0, 0);
}
