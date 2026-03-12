#include "Image.h"
#include "DirectX11.h"
#include <iostream>

sf::ui::Image::Image()
{

}

void sf::ui::Image::Draw()
{
	if (!isVisible) {
		return;
	}

	dx::DirectX11* dx11 = dx::DirectX11::Instance();
	// ★ デバイス/コンテキストの有効性チェック（シーン遷移時のクラッシュ防止）
	if (!dx11) return;
	auto* context = dx11->GetMainDevice().GetContext();
	if (!context) return;

	if (material.texture.isNull())
	{
		material.SetGPU(dx11->GetMainDevice());
	}
	else
	{
		material.texture->SetGPU(0, dx11->GetMainDevice());
		material.SetGPU(dx11->GetMainDevice(), true);
	}

	WorldMatrixBuffer mtx;
	mtx.mtx = GetMatrix();
	mtx.mtx = DirectX::XMMatrixTranspose(mtx.mtx);
	dx11->wBuffer.SetGPU(mtx, dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	dx11->vsNone.SetGPU(dx11->GetMainDevice());
	dx11->gs2d.SetGPU(dx11->GetMainDevice());
	dx11->ps2d.SetGPU(dx11->GetMainDevice());


	// SetUV用のスケール・オフセットを計算
	UVRect uvRect;
	uvRect.uvScale = { uvRight - uvLeft, uvBottom - uvTop };
	uvRect.uvOffset = { uvLeft, uvTop };
	// 定数バッファにセット
	dx11->uvRectBuffer.SetGPU(uvRect, dx11->GetMainDevice().GetContext());

	dx11->GetMainDevice().GetContext()->Draw(1, 0);
}

void sf::ui::Image::SetUV(float u0, float v0, float u1, float v1) {
	uvLeft = u0;
	uvTop = v0;
	uvRight = u1;
	uvBottom = v1;
}

void sf::ui::Image::SetVisible(bool visible)
{
	isVisible = visible;
}

bool sf::ui::Image::IsVisible() const
{
	return isVisible;
}
