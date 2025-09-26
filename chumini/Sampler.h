#pragma once
#include "D3D.h"
namespace sf
{
	namespace dx
	{
		//サンプラー(D3D)
		class Sampler:public D3D
		{
		public:
			~Sampler();

			void Release()override;

			/*
			---サンプラーのパラメータ解説---

			〇D3D11_TEXTURE_ADDRESS_WRAP:
			テクスチャ情報はUV座標に限らず存在し、正転を繰り返す

			〇D3D11_TEXTURE_ADDRESS_MIRROR:
			テクスチャ情報はUV座標に限らず存在し、正転,反転を繰り返す

			〇D3D11_TEXTURE_ADDRESS_CLAMP:
			テクスチャ情報はUV座標0.0f～1.0fのみ存在する

			〇D3D11_TEXTURE_ADDRESS_BORDER:
			UV座標0.0f～1.0fの範囲のみクリッピングされる。その他の領域は描画されない

			〇D3D11_TEXTURE_ADDRESS_MIRROR_ONCE:
			テクスチャ情報はUV座標-1.0f～1.0fに存在し、反転,正転して存在する
			*/
			HRESULT Create(D3D11_TEXTURE_ADDRESS_MODE u = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MODE v = D3D11_TEXTURE_ADDRESS_WRAP);

			void SetGPU(ID3D11DeviceContext* d3dContext)const;

		private:
			ID3D11SamplerState* d3dSampler = nullptr;
		};
	}
}