#pragma once
#include "UI.h"
#include "Material.h"

struct UVRect {
	DirectX::XMFLOAT2 uvScale;    // float2に対応
	DirectX::XMFLOAT2 uvOffset;   // float2に対応
};

namespace sf
{
	namespace ui
	{
		//UI描画クラス
		class Image :public UI
		{
		public:
			
			Image();

			void Draw()override;

			void SetVisible(bool visible);
			bool IsVisible() const;

		public:
			Material material;

			void SetUV(float u0, float v0, float u1, float v1);

			// UV範囲（デフォルトは全体）
			float uvLeft = 0.0f, uvTop = 0.0f, uvRight = 1.0f, uvBottom = 1.0f;

			bool isVisible = true;
			
		};
	}
}