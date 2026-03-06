#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class GeometrySwat :public sf::geometry::GeometryFbx
		{
		public:
			//読み込み
			void Load();

			//描画(オーバーライド)
			void Draw(const sf::Material& mtl)override;

		protected:
			//全テクスチャの読み込み(オーバーライド)
			void LoadTextures(const aiScene* scene)override;
		};
	}
}