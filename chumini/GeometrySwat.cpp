#include "GeometrySwat.h"

void app::test::GeometrySwat::Load()
{
	//Swatモデルの読み込み
	LoadFbx("Assets\\Model\\Swat.fbx");
}

void app::test::GeometrySwat::Draw(const sf::Material& mtl)
{
	sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();

	//マテリアル設定(diffuseテクスチャ,法線テクスチャあり)
	mtl.SetGPU(dx11->GetMainDevice(), true, true);

	//各バッファの設定
	vertexBuffer.SetGPU(dx11->GetMainDevice());
	indexBuffer.SetGPU(dx11->GetMainDevice());

	//トポロジーとシェーダー設定
	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dx11->vsMotion.SetGPU(dx11->GetMainDevice());
	dx11->gsNone.SetGPU(dx11->GetMainDevice());
	dx11->ps3d.SetGPU(dx11->GetMainDevice());

	const std::vector<sf::Subset>& subsets = subsetData.GetData();

	//サブセット別描画処理(ラムダ式)
	auto draw = [](const sf::Subset& subset)
		{
			sf::dx::DirectX11::Instance()->
				GetMainDevice().
				GetContext()->
				DrawIndexed(subset.IndexNum(), subset.indexBase, subset.vertexBase);
		};


	//サブセット0:ヘルメット
	//サブセット1:顔
	//サブセット2:体

	textures[0].SetGPU(0, dx11->GetMainDevice());	//diffuseテクスチャ
	textures[1].SetGPU(1, dx11->GetMainDevice());	//法線テクスチャ
	draw(subsets[0]);//ヘルメット
	draw(subsets[2]);//体

	textures[3].SetGPU(0, dx11->GetMainDevice());	//diffuseテクスチャ
	textures[4].SetGPU(1, dx11->GetMainDevice());	//法線テクスチャ
	draw(subsets[1]);//顔
}

void app::test::GeometrySwat::LoadTextures(const aiScene* scene)
{
	//テクスチャは6枚
	const int textureNum = 6;

	textures.resize(textureNum);

	textures[0].LoadTexture(scene->mTextures[0]);//ベースカラー(体)
	textures[1].LoadTexture(scene->mTextures[1]);//法線カラー(体)
	textures[2].LoadTexture(scene->mTextures[2]);//謎カラー(体)

	textures[3].LoadTexture(scene->mTextures[3]);//ベースカラー(顔)
	textures[4].LoadTexture(scene->mTextures[4]);//法線カラー(顔)
	textures[5].LoadTexture(scene->mTextures[5]);//謎カラー(顔)
}
