#pragma once
#include "Component.h"
#include "Geometry.h"
#include "Material.h"
#include "Motion.h"
namespace sf
{
	//メッシュクラス(UnityのMeshRenderer)
	class Mesh :public Component
	{
	public:
		~Mesh();

		void Activate()override;
		void DeActivate()override;

		void SetGeometry(sf::ref::Ref<sf::geometry::Geometry> ref) { geometryRef = ref; }

		void SetMotion(const sf::motion::Motion* _motion) { motion = _motion; }

		static void DrawAll();
		static void DrawShadowAll();

	protected:
		virtual DirectX::XMMATRIX WorldMatrix()const;

	private:
		void Draw();

	public:
		Material material;

	private:
		sf::ref::Ref<sf::geometry::Geometry> geometryRef;

		const sf::motion::Motion* motion = nullptr;

		static std::vector<Mesh*> meshes;
	};
}