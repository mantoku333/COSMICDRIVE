#include "Texture.h"
#include "DirectX11.h"
#include "Debug.h"
#include "stb_image.h"
#include <DirectXTex.h>
#pragma comment(lib,"directxtex.lib")

sf::ITexture::~ITexture()
{
	Release();
}

void sf::ITexture::Release()
{
	SAFE_RELEASE(srv);
}

void sf::ITexture::SetGPU(int slot, ID3D11DeviceContext* d3dContext) const
{
	d3dContext->PSSetShaderResources(slot, 1, &srv);
}

bool sf::Texture::LoadTextureFromFile(std::string path)
{
	SAFE_RELEASE(srv);

	bool sts = true;
	unsigned char* pixels;

	int m_bpp;					// BPP

	// 画像読み込み
	pixels = stbi_load(path.c_str(), &w, &h, &m_bpp, 4);
	if (pixels == nullptr) {
		debug::Debug::LogError("テクスチャの読み込みに失敗しました:" + path);
		return false;
	}

	// テクスチャ2Dリソース生成
	ID3D11Texture2D* pTexture = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// RGBA
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subResource{};
	subResource.pSysMem = pixels;
	subResource.SysMemPitch = desc.Width * 4;			// RGBA = 4 bytes per pixel
	subResource.SysMemSlicePitch = 0;

	ID3D11Device* device = dx::DirectX11::Instance()->GetMainDevice();

	HRESULT hr = device->CreateTexture2D(&desc, &subResource, &pTexture);
	if (FAILED(hr)) {
		stbi_image_free(pixels);

		debug::Debug::LogError("テクスチャの読み込みに失敗しました:" + path);
		return false;
	}

	// SRV生成
	hr = device->CreateShaderResourceView(pTexture, nullptr, &srv);
	if (FAILED(hr)) {
		stbi_image_free(pixels);
		debug::Debug::LogError("テクスチャの読み込みに失敗しました:" + path);
		return false;
	}

	SAFE_RELEASE(pTexture);

	// ピクセルイメージ解放
	stbi_image_free(pixels);

	fileName = path;

	debug::Debug::LogSafety("テクスチャの読み込みに成功しました:" + path);

	return true;
}

bool sf::TextureMemory::LoadTexture(const aiTexture* texture)
{
	ID3D11Device* d3dDevice = dx::DirectX11::Instance()->GetMainDevice().GetDevice();

	// テクスチャが圧縮されていない場合のみ処理
	if (texture->mHeight != 0)
	{
		debug::Debug::LogError("テクスチャの読み込みに失敗しました");
		return false;
	}

	// aiTexture の画像データを stbi_load_from_memory で読み込む
	int width, height, channels;
	unsigned char* pixelData = stbi_load_from_memory(
		reinterpret_cast<unsigned char*>(texture->pcData), // 画像データへのポインタ
		static_cast<int>(texture->mWidth), // データのサイズ
		&width, &height, &channels, 4 // 4 チャンネル (RGBA) で読み込む
	);

	if (!pixelData) {
		debug::Debug::LogError("テクスチャの読み込みに失敗しました");
		return false; // 画像の読み込みに失敗
	}

	// テクスチャの説明構造体を設定
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// 初期データの設定
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixelData;
	initData.SysMemPitch = width * 4;

	// ID3D11Texture2D を作成
	ID3D11Texture2D* pTexture = nullptr;
	HRESULT hr = d3dDevice->CreateTexture2D(&textureDesc, &initData, &pTexture);

	// ピクセルデータの解放
	stbi_image_free(pixelData);

	if (FAILED(hr) || !pTexture) {
		debug::Debug::LogError("テクスチャの読み込みに失敗しました");
		return false;
	}

	// シェーダーリソースビューの説明構造体を設定
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// ID3D11ShaderResourceView を作成
	ID3D11ShaderResourceView* pSRV = nullptr;
	hr = d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &srv);

	// テクスチャを解放
	pTexture->Release();

	if (FAILED(hr)) {
		debug::Debug::LogError("テクスチャの読み込みに失敗しました");
		return false;
	}

	debug::Debug::LogSafety("メモリ上のテクスチャの読み込みに成功しました");

	return true;
}
