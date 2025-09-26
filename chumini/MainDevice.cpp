#include "MainDevice.h"

HRESULT sf::dx::MainDevice::Create(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);

	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	// デバイス、スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = w;
	swapChainDesc.BufferDesc.Height = h;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;
	//swapChainDesc.Windowed = FALSE;

	//DirectX11を使用
	D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&d3dSwapchain,
		&d3dDevice,
		&m_FeatureLevel,
		&d3dContext);

	if (FAILED(hr))
	{
		throw std::runtime_error("デバイス、スワップチェイン作成に失敗しました");
	}

	return hr;
}
