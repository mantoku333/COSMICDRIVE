#include "GeometryCube.h"
#include "DirectX11.h"

void sf::geometry::GeometryCube::Draw(const Material& mtl)
{
	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	

	mtl.SetGPU(dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	dx11->vsNone.SetGPU(dx11->GetMainDevice());
	dx11->gsCube.SetGPU(dx11->GetMainDevice());
	dx11->ps3d.SetGPU(dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->Draw(1, 0);
}
