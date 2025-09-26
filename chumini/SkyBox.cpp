#include "SkyBox.h"

void app::SkyBox::Init()
{
	gs.LoadCSO("Assets\\Shader\\gsSkyBox.cso");
	ps.LoadCSO("Assets\\Shader\\psSkyBox.cso");

	texture.LoadTextureFromFile("Assets\\SkyBox.png");
}

void app::SkyBox::Draw()
{
	sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	dx11->vsNone.SetGPU(dx11->GetMainDevice());
	gs.SetGPU(dx11->GetMainDevice());
	ps.SetGPU(dx11->GetMainDevice());

	texture.SetGPU(0, dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->Draw(1, 0);
}
