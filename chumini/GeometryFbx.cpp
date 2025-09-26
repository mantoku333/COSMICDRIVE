#include "GeometryFbx.h"
#include "Debug.h"

void sf::geometry::GeometryFbx::LoadFbx(std::string path)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		path.c_str(),
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_ConvertToLeftHanded |
		aiProcess_PopulateArmatureData);


	if (scene == nullptr)
	{
		std::runtime_error("FBXモデルの読み込みに失敗しました:" + path);
		return;
	}


	subsetData.Create(scene);

	const std::vector<dx::Vertex3D> vertex = subsetData.GetVertices();

	std::vector<dx::Vertex3D> vertices(vertex.size());

	for (int i = 0; i < vertices.size(); i++)
	{
		vertices[i].pos = vertex[i].pos;
		vertices[i].color = vertex[i].color;
		vertices[i].nor = vertex[i].nor;
		vertices[i].uv = vertex[i].uv;
	}

	//ボーンマップ初期化
	CreateBone(scene->mRootNode);

	//ボーンマップの中で先頭から何番目かを設定
	int n = 0;
	for (auto& i : boneMap) {
		i.second.index = n;
		n++;
	}

	int subsetid = 0;
	for (auto& bones : subsetData.GetData()) {

		int vertexbase = subsetData.GetData(subsetid).vertexBase;

		for (auto& bone : bones.BoneData())
		{
			for (auto& w : bone.weights) {
				int& idx = vertices[vertexbase + w.vertexindex].boneCnt;
				vertices[vertexbase + w.vertexindex].boneWeight[idx] = w.weight;
				vertices[vertexbase + w.vertexindex].boneIndex[idx] = boneMap[w.boneName].index;

				idx++;
				assert(idx <= 4);
			}

			boneMap[bone.boneName].offsetMatrix = bone.offsetMatrix;
		}
		subsetid++;				// 次のサブセットへ
	}

	rootNode.Create(scene->mRootNode);

	vertexBuffer.Create(vertices);
	indexBuffer.Create(subsetData.GetIndices());

	LoadTextures(scene);


	importer.FreeScene();
}

void sf::geometry::GeometryFbx::Draw(const Material& mtl)
{
	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	mtl.SetGPU(dx11->GetMainDevice());

	vertexBuffer.SetGPU(dx11->GetMainDevice());
	indexBuffer.SetGPU(dx11->GetMainDevice());

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dx11->vs3d.SetGPU(dx11->GetMainDevice());
	dx11->gsNone.SetGPU(dx11->GetMainDevice());
	dx11->ps3d.SetGPU(dx11->GetMainDevice());

	const std::vector<Subset>& subsets = subsetData.GetData();

	for (auto& i : subsets) {
		textures[i.MaterialIndex()].SetGPU(0, dx11->GetMainDevice());
		dx11->GetMainDevice().GetContext()->DrawIndexed(i.IndexNum(), i.indexBase, i.vertexBase);
	}
}

void sf::geometry::GeometryFbx::LoadTextures(const aiScene* scene)
{
	textures.resize(scene->mNumTextures);

	for (int i = 0; i < scene->mNumTextures; i++)
	{
		textures[i].LoadTexture(scene->mTextures[i]);
	}
}

void sf::geometry::GeometryFbx::CreateBone(const aiNode* node)
{
	Bone bone{};

	//ボーン名で参照できるように空のボーン情報をセットする
	boneMap[node->mName.C_Str()] = bone;

	for (unsigned int n = 0; n < node->mNumChildren; n++)
	{
		CreateBone(node->mChildren[n]);
	}
}
