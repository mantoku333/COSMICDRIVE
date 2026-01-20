#include "DirectX11.h"

void sf::dx::DirectX11::Create(HWND _hwnd)
{
	//メインデバイス作成
	mainDevice.Create(_hwnd);

	//オンスクリーンレンダーターゲットの作成
	onScreen.Create();

	int w = 1920;
	int h = 1080;

	rb2d.Create(w, h);
	rb3d.Create(w, h);

	const int downSamplingLevel = 8;
	downX.Create(w / downSamplingLevel, h);
	downY.Create(w / downSamplingLevel, h / downSamplingLevel);

	shadowRTV.Create(w, h);

	depth.Create(w, h);

	context = mainDevice.CreateDeferredContext();

	LoadShader();
	
	sampler.Create();
	sampler.SetGPU(mainDevice);


	wBuffer.Create(0);
	vpBuffer.Create(1);
	mtlBuffer.Create(2);
	motionBuffer.Create(3);
	lineBuffer.Create(4);
	systemBuffer.Create(5);
	uvRectBuffer.Create(6);

	alphaBlend.Create(D3D11_BLEND_INV_SRC_ALPHA);
	addBlend.Create(D3D11_BLEND_ONE);

	hwnd = _hwnd;

	SetViewPort();
}

void sf::dx::DirectX11::SetNextRenderingDoubleBuffer3D()
{
	mainDevice.GetContext()->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	rb3d.NextBuffer();
	rb3d.ClearBuffer(mainDevice);
}

void sf::dx::DirectX11::SetRenderingDoubleBuffer3DSkyBox()
{
	alphaBlend.SetGPU(mainDevice);
	rb3d.SetRBuffer(mainDevice, nullptr);
}

// 3D rendering setup (resets states to avoid Live2D artifacts)
void sf::dx::DirectX11::SetRenderingDoubleBuffer3D()
{
	auto* context = mainDevice.GetContext();
	if (context) {
		context->RSSetState(nullptr);
		float bf[4] = { 0,0,0,0 };
		context->OMSetBlendState(nullptr, bf, 0xffffffff);
		context->OMSetDepthStencilState(nullptr, 0);
	}
	rb3d.SetRBuffer(mainDevice, depth);
}

void sf::dx::DirectX11::SetRenderingDoubleBuffer2D()
{
	//rb2d.NextBuffer();
	rb2d.ClearBuffer(mainDevice);
	rb2d.SetRBuffer(mainDevice, nullptr);
}

void sf::dx::DirectX11::OnScreenRendering()
{

	//オンスクリーンを塗りつぶし
	mainDevice.ClearRTV(onScreen.GetRTV());

	//オンスクリーンRTVを設定
	ID3D11RenderTargetView* rtv[] =
	{
		onScreen.GetRTV(),
	};
	mainDevice.GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, nullptr);

	for (int i = 0; i < BUFFERSIZE; i++)
		rb3d.Get(0).GetBaseColorRTV().SetTexture(mainDevice, i);

	rb3d.Get().GetNormalRTV().SetTexture(mainDevice, 4);
	rb2d.GetBaseColorRTV().SetTexture(mainDevice, 5);
	rb2d.GetAlphaRTV().SetTexture(mainDevice, 6);

	mainDevice.GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	vsNone.SetGPU(mainDevice);
	gsOnScreen.SetGPU(mainDevice);
	psOnScreen.SetGPU(mainDevice);

	mainDevice.GetContext()->Draw(1, 0);

	ID3D11ShaderResourceView* srv[]
	{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
	};
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);

	PostProcessing();
}

void sf::dx::DirectX11::SetViewPort()
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	SetViewPort(rect.right - rect.left, rect.bottom - rect.top);
}

void sf::dx::DirectX11::SetViewPort(int w, int h)
{
	D3D11_VIEWPORT m_viewport{};

	// ビューポートの設定
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = w;
	m_viewport.Height = h;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	mainDevice.GetContext()->RSSetViewports(1, &m_viewport);
}

void sf::dx::DirectX11::LoadShader()
{
	vs3d.LoadCSO("Assets\\Shader\\vs3d.cso");
	vsMotion.LoadCSO("Assets\\Shader\\vsMotion.cso");
	vsNone.LoadCSO("Assets\\Shader\\vsNone.cso");

	gs2d.LoadCSO("Assets\\Shader\\gs2d.cso");
	gsCube.LoadCSO("Assets\\Shader\\gsCube.cso");
	gsOnScreen.LoadCSO("Assets\\Shader\\gsOnScreen.cso");
	gsDebugLine.LoadCSO("Assets\\Shader\\gsDebugLine.cso");
	gsDownX.LoadCSO("Assets\\Shader\\gsDownSamplingX.cso");
	gsDownY.LoadCSO("Assets\\Shader\\gsDownSamplingY.cso");

	ps3d.LoadCSO("Assets\\Shader\\ps3d.cso");
	ps2d.LoadCSO("Assets\\Shader\\ps2d.cso");
	psOnScreen.LoadCSO("Assets\\Shader\\psOnScreen.cso");
	psDebugLine.LoadCSO("Assets\\Shader\\psDebugLine.cso");
	psShadow.LoadCSO("Assets\\Shader\\psShadow.cso");
	psBlur.LoadCSO("Assets\\Shader\\psBlur.cso");
	psPostProcess.LoadCSO("Assets\\Shader\\psPostProcess.cso");
}

void sf::dx::DirectX11::PostProcessing()
{
	DownSamplingX();
	DownSamplingY();

	ID3D11RenderTargetView* rtv[] =
	{
		onScreen.GetRTV(),
	};
	mainDevice.GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, nullptr);

	ID3D11ShaderResourceView* srv[] =
	{
		downY.GetSRV(),
	};
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);

	gsOnScreen.SetGPU(mainDevice);
	psPostProcess.SetGPU(mainDevice);

	addBlend.SetGPU(mainDevice.GetContext());
	mainDevice.GetContext()->Draw(1, 0);
	alphaBlend.SetGPU(mainDevice.GetContext());

	srv[0] = nullptr;
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);
}

void sf::dx::DirectX11::DownSamplingX()
{
	ID3D11RenderTargetView* rtv[] =
	{
		downX.GetRTV(),
	};
	mainDevice.GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, nullptr);

	ID3D11ShaderResourceView* srv[] =
	{
		rb3d.Get().GetLuminanceRTV().GetSRV(),
	};
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);

	gsDownX.SetGPU(mainDevice.GetContext());
	psBlur.SetGPU(mainDevice.GetContext());

	mainDevice.GetContext()->Draw(1, 0);
	srv[0] = nullptr;
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);
}

void sf::dx::DirectX11::DownSamplingY()
{
	ID3D11RenderTargetView* rtv[] =
	{
		downY.GetRTV(),
	};
	mainDevice.GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, nullptr);

	ID3D11ShaderResourceView* srv[] =
	{
		downX.GetSRV(),
	};

	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);

	gsDownY.SetGPU(mainDevice.GetContext());
	psBlur.SetGPU(mainDevice.GetContext());

	mainDevice.GetContext()->Draw(1, 0);
	srv[0] = nullptr;
	mainDevice.GetContext()->PSSetShaderResources(0, ARRAYSIZE(srv), srv);
}
