#include "Material.h"
#include "DirectX11.h"

void sf::Material::SetGPU(ID3D11DeviceContext* d3dContext, bool diffuseTexture, bool normalTexture) const
{
	mtl material;
	material.diffuseColor = diffuseColor;



	material.textureEnable.x = diffuseTexture;
	material.textureEnable.y = normalTexture;
	material.textureEnable.z = shadow;

	if (!texture.isNull())
	{
		material.textureEnable.x = true;
		texture->SetGPU(0, dx::DirectX11::Instance()->GetMainDevice());
	}

	dx::DirectX11::Instance()->mtlBuffer.SetGPU(material, d3dContext);
}

void sf::Material::SetColor(const DirectX::XMFLOAT4& color)
{
	diffuseColor = color;
}

const DirectX::XMFLOAT4& sf::Material::GetColor() const
{
	return diffuseColor;
}

