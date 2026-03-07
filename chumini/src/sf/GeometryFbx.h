#pragma once
#include "Geometry.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Subset.h"
#include "Texture.h"
#include "Material.h"

namespace sf
{
	namespace geometry
	{
		//FBXモデル用ジオメトリクラス
		class GeometryFbx :public Geometry
		{
		public:
			void LoadFbx(std::string path);

			virtual void Draw(const Material& mtl)override;

			const std::map<std::string, Bone>& GetBoneMap()const { return boneMap; }

			const Node& GetRootNode()const { return rootNode; }

		protected:
			virtual void LoadTextures(const aiScene* scene);

		private:
			void CreateBone(const aiNode* node);

		protected:
			dx::VertexBuffer<dx::Vertex3D> vertexBuffer;
			dx::IndexBuffer indexBuffer;
			SubsetData subsetData;

			std::vector<TextureMemory> textures;

		private:
			std::map<std::string, Bone> boneMap;

			Node rootNode;
		};
	}
}