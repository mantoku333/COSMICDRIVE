#pragma once
#include "SafePtr.h"
#include "Texture.h"
#include "ConstBuffer.h"
#include <assimp/material.h>
#include <assimp/texture.h>
namespace sf
{
	//マテリアルクラス
	class Material
	{
	public:
		/// <summary>
		/// GPUへマテリアル情報を送る
		/// </summary>
		/// <param name="d3dContext"></param>
		/// <param name="diffuseTexture">diffuseテクスチャの有無</param>
		/// <param name="normalTexture">法線テクスチャの有無</param>
		void SetGPU(ID3D11DeviceContext* d3dContext, bool diffuseTexture = false, bool normalTexture = false)const;

		void SetColor(const DirectX::XMFLOAT4& color);

		const DirectX::XMFLOAT4& GetColor() const;

	public:
		DirectX::XMFLOAT4 diffuseColor = DirectX::XMFLOAT4(1, 1, 1, 1);

		bool shadow = false;

		sf::SafePtr<Texture> texture;
	};
}