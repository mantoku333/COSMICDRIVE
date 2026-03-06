#include "Device.h"

sf::dx::Device::~Device()
{
	Release();
}

void sf::dx::Device::Release()
{
	SAFE_RELEASE(d3dDevice);
}

ID3D11DeviceContext* sf::dx::Device::CreateDeferredContext() const
{
	ID3D11DeviceContext* context;

	HRESULT hr = d3dDevice->CreateDeferredContext(0, &context);

	if (FAILED(hr))
	{
		throw std::runtime_error("DeferredContextの作成に失敗しました");
	}

	return context;
}

HRESULT sf::dx::Device::CreateVertexBuffer(const void* vertex, int vSize, int byteSize, ID3D11Buffer** vertexBuffer)
{
	// 頂点バッファを作成する
	// ※頂点バッファ→VRAMに頂点データを置くための機能
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = byteSize * vSize;// 確保するバッファサイズを指定
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;// 頂点バッファ作成を指定
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subResourceData{};
	subResourceData.pSysMem = vertex;// VRAMに送るデータを指定
	subResourceData.SysMemPitch = 0;
	subResourceData.SysMemSlicePitch = 0;

	HRESULT hr = d3dDevice->CreateBuffer(&bufferDesc, &subResourceData, vertexBuffer);

	return hr;
}

HRESULT sf::dx::Device::CreateIndexBuffer(const std::vector<WORD>& index, ID3D11Buffer** indexBuffer)
{
	//インデックスバッファの作成
	//頂点インデックスを格納するDirectXの機能
	D3D11_BUFFER_DESC ibDesc{};
	ibDesc.ByteWidth = sizeof(WORD) * index.size();
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA irData{};
	irData.pSysMem = index.data();
	irData.SysMemPitch = 0;
	irData.SysMemSlicePitch = 0;

	HRESULT hr = d3dDevice->CreateBuffer(&ibDesc, &irData, indexBuffer);

	return hr;
}
