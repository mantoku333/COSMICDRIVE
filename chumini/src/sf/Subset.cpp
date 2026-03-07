#include "Subset.h"
#include "Material.h"
void sf::Bone::Set(const aiBone* bone)
{
	boneName = bone->mName.C_Str();

	offsetMatrix = aiMtxToDxMtx(bone->mOffsetMatrix);

	weights.resize(bone->mNumWeights);

	for (int i = 0; i < bone->mNumWeights; i++)
	{
		weights[i].boneName = boneName;

		aiVertexWeight weight = bone->mWeights[i];

		weights[i].vertexindex = weight.mVertexId;
		weights[i].weight = weight.mWeight;
	}
}

void sf::Bone::Copy(const Bone& other)
{
	boneName = other.boneName;
	matrix = other.matrix;
	animationMatrix = other.animationMatrix;
	offsetMatrix = other.offsetMatrix;
	index = other.index;

	weights.resize(other.weights.size());
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i].boneName = other.weights[i].boneName;
		weights[i].vertexindex = other.weights[i].vertexindex;
		weights[i].weight = other.weights[i].weight;
	}
}


// assimp行列をDIRECTXの行列に変換する
DirectX::XMMATRIX sf::Bone::aiMtxToDxMtx(aiMatrix4x4 aimatrix) {

	DirectX::XMMATRIX mtx = {
	   aimatrix.a1,aimatrix.b1,aimatrix.c1,aimatrix.d1,
	   aimatrix.a2,aimatrix.b2,aimatrix.c2,aimatrix.d2,
	   aimatrix.a3,aimatrix.b3,aimatrix.c3,aimatrix.d3,
	   aimatrix.a4,aimatrix.b4,aimatrix.c4,aimatrix.d4
	};

	return mtx;
}

void sf::Subset::Create(const aiMesh* mesh)
{
	// メッシュ単位で頂点データを取得
	GetVerticesFromaiMesh(mesh);

	//メッシュ単位でインデックスデータ取得
	GetIndicesFromaiMesh(mesh);

	// マテリアルインデックスを保存
	materialIdx = mesh->mMaterialIndex;

	// サブセット名をセットする
	name = std::string(mesh->mName.C_Str());

	//ボーン情報取得
	boneData.resize(mesh->mNumBones);
	for (int i = 0; i < mesh->mNumBones; i++)
	{
		boneData[i].Set(mesh->mBones[i]);
	}
}

void sf::Subset::GetVerticesFromaiMesh(const aiMesh* aimesh)
{
	// 頂点数分ループ
	for (unsigned int vidx = 0; vidx < aimesh->mNumVertices; vidx++) {

		dx::Vertex3D v;

		v.pos.x = aimesh->mVertices[vidx].x * 1.0f;
		v.pos.y = aimesh->mVertices[vidx].y * 1.0f;
		v.pos.z = aimesh->mVertices[vidx].z * 1.0f;

		// 法線あり？
		if (aimesh->HasNormals()) {
			v.nor.x = aimesh->mNormals[vidx].x;
			v.nor.y = aimesh->mNormals[vidx].y;
			v.nor.z = aimesh->mNormals[vidx].z;
		}
		else {
			v.nor = DirectX::XMFLOAT3(0, 0, 0);
		}

		// テクスチャあり？
		if (aimesh->HasTextureCoords(0)) {
			v.uv.x = aimesh->mTextureCoords[0][vidx].x;				// 20230809 bug fix by ts
			v.uv.y = aimesh->mTextureCoords[0][vidx].y;				// 20230809 bug fix by ts
		}
		else {
			v.uv = DirectX::XMFLOAT2(0, 0);
		}

		// 頂点カラーあり？
		if (aimesh->HasVertexColors(0)) {
			v.color.x = aimesh->mColors[0][vidx].r;
			v.color.y = aimesh->mColors[0][vidx].g;
			v.color.z = aimesh->mColors[0][vidx].b;
			v.color.w = aimesh->mColors[0][vidx].a;
		}
		else {
			v.color = DirectX::XMFLOAT4(1, 1, 1, 1);		// 無かった場合は白
		}

		// 頂点データを保存
		vertices.emplace_back(v);
	}
}

void sf::Subset::GetIndicesFromaiMesh(const aiMesh* aimesh)
{
	// 面数分ループ
	for (unsigned int f = 0; f < aimesh->mNumFaces; f++)
	{
		const aiFace* face = &aimesh->mFaces[f];
		assert(face->mNumIndices <= 3);

		// インデックスデータを保存
		indices.push_back(face->mIndices[0]);
		indices.push_back(face->mIndices[1]);
		indices.push_back(face->mIndices[2]);
	}
}


void sf::SubsetData::Create(const aiScene* scene)
{
	datas.resize(scene->mNumMeshes);

	std::vector<Material> materials;
	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		const aiMaterial* material = scene->mMaterials[i];

		Material mtl;

		aiColor3D color(0.0f, 0.0f, 0.0f);
		if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			mtl.diffuseColor = DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f);
		}
		materials.push_back(mtl);
	}

	// メッシュ数分ループ
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {

		//対象のaiMesh
		const aiMesh* mesh = scene->mMeshes[i];

		//サブセット作成
		datas[i].Create(scene->mMeshes[i]);

		int start = vertices.size();
		//頂点データを既存のデータの後ろにコピー
		const std::vector<dx::Vertex3D>& vertexData = datas[i].GetVertices();
		vertices.insert(vertices.end(), vertexData.begin(), vertexData.end());

		//ディフューズカラーを頂点カラーとして取り扱う
		for (int j = start; j < vertices.size(); j++)
		{
			vertices[j].color = materials[datas[i].MaterialIndex()].diffuseColor;
		}

		//インデックスデータを既存のデータの後ろにコピー
		const std::vector<WORD>& indexData = datas[i].GetIndices();
		indices.insert(indices.end(), indexData.begin(), indexData.end());
	}

	CalcSubsetBases();
}

void sf::SubsetData::CalcSubsetBases()
{
	int vCnt = 0;
	int iCnt = 0;

	//サブセットループ
	for (auto& i : datas) {

		//前までの頂点数、インデックス数を保存
		i.vertexBase = vCnt;
		i.indexBase = iCnt;

		//頂点数、インデックス数を加算
		vCnt += i.VertexNum();
		iCnt += i.IndexNum();
	}
}

sf::Node::~Node()
{
	for (auto& i : children) {
		delete i;
	}
}

void sf::Node::Create(const aiNode* node)
{
	this->boneName = node->mName.C_Str();

	children.resize(node->mNumChildren);

	for (int i = 0; i < node->mNumChildren; i++)
	{
		children[i] = new Node();
		children[i]->parent = this;
		children[i]->Create(node->mChildren[i]);
	}
}
