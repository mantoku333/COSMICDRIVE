#pragma once
#include "D3D.h"

#include <assimp/texture.h>

namespace sf
{
	//テクスチャリソースクラス
	class ITexture :public dx::D3D
	{
	public:
		virtual ~ITexture();

		void Release()override;

		void SetGPU(int slot, ID3D11DeviceContext* d3dContext) const;

	//protected:
		ID3D11ShaderResourceView* srv = nullptr;
	};

	//外部ファイルから読み込んだテクスチャクラス
	class Texture :public ITexture
	{
	public:
		bool LoadTextureFromFile(std::string path);

		int GetWidth()const { return w; }
		int GetHeight()const { return h; }

	private:
		int w = 0;
		int h = 0;

		std::string fileName;
	};

	//メモリから読み込んだテクスチャクラス
	class TextureMemory :public ITexture
	{
	public:
		bool LoadTexture(const aiTexture* texture);
	};
}