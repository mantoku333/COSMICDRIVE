#pragma once
#include "DirectX11.h"
#include "Texture.h"
namespace app
{
	class SkyBox
	{
	public:
		void Init();
		void Draw();

	private:
		sf::dx::shader::GeometryShader gs;
		sf::dx::shader::PixelShader ps;
		sf::Texture texture;
	};
}