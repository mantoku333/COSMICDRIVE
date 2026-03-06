#pragma once

#include "VertexBuffer.h"
#include <vector>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#pragma comment(lib,"assimp-vc143-mtd.lib")

namespace sf
{
	//アニメーション用ボーンのウェイトクラス
	class Weight
	{
	public:
		std::string boneName;

		float weight = 0.0f;

		int	vertexindex = -1;
	};

	//アニメーション用ボーンクラス
	class Node
	{
	public:
		~Node();

		void Create(const aiNode* node);

	public:
		std::string boneName;

		std::vector<Node*> children;

	private:
		Node* parent = nullptr;
	};

	//アニメーションのボーンクラス
	class Bone
	{
	public:
		void Set(const aiBone* bone);

		/// <summary>
		/// 他のボーンデータをコピーする
		/// </summary>
		/// <param name="other"></param>
		void Copy(const Bone& other);

	private:
		static DirectX::XMMATRIX aiMtxToDxMtx(aiMatrix4x4 aimatrix);

	public:
		//ボーン名
		std::string boneName;

		//計算結果となる親子関係を考慮した行列
		DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();

		//自分の事だけを考えた行列、ローカル行列
		DirectX::XMMATRIX animationMatrix = DirectX::XMMatrixIdentity();

		//ボーンオフセット行列
		DirectX::XMMATRIX offsetMatrix = DirectX::XMMatrixIdentity();

		//ボーンマップの中で先頭から何番目か
		int index = -1;

		std::vector<Weight> weights;
	};

	//1つのサブセットを扱うクラス
	class Subset
	{
	public:
		void Create(const aiMesh* mesh);

		//頂点配列取得
		const std::vector<dx::Vertex3D>& GetVertices()const { return vertices; }

		//インデックス配列取得
		const std::vector<WORD>& GetIndices()const { return indices; }

		//頂点数
		unsigned int VertexNum()const { return static_cast<unsigned int>(vertices.size()); }

		//インデックス数
		unsigned int IndexNum()const { return static_cast<unsigned int>(indices.size()); }

		//マテリアルインデックス
		unsigned int MaterialIndex()const { return materialIdx; }

		const std::vector<Bone>& BoneData()const { return boneData; }

	public:
		//開始インデックス
		unsigned int indexBase = 0;

		//頂点ベース
		unsigned int vertexBase = 0;

	private:
		//頂点配列作成関数
		void GetVerticesFromaiMesh(const aiMesh* aimesh);

		//インデックス配列作成関数
		void GetIndicesFromaiMesh(const aiMesh* aimesh);

	private:
		//サブセット名
		std::string name;

		//マテリアルインデックス
		unsigned int materialIdx = 0;

		//頂点配列
		std::vector<dx::Vertex3D> vertices;

		//インデックス配列
		std::vector<WORD> indices;

		std::vector<Bone> boneData;
	};

	//サブセットを集めたデータクラス
	class SubsetData
	{
	public:
		void Create(const aiScene* scene);

		const std::vector<Subset>& GetData()const { return datas; }
		const Subset& GetData(int i)const { return datas[i]; }

		const std::vector<dx::Vertex3D>& GetVertices()const { return vertices; }
		const std::vector<WORD>& GetIndices()const { return indices; }

	private:
		//サブセットの開始位置を計算する関数
		void CalcSubsetBases();

	private:
		std::vector<Subset> datas;

		//頂点配列(ウェイトデータ等は定まっていない)
		std::vector<dx::Vertex3D> vertices;

		//インデックス配列
		std::vector<WORD> indices;

	};
}
