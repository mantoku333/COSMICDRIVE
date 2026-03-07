#include "DynamicScene.h"
#include "ChangeMotionComponent.h"

void app::test::DynamicScene::Init()
{
	////アクター生成
	//auto a_sphere = Instantiate();

	////座標設定
	//a_sphere.Target()->transform.SetPosition(Vector3(0, 2, 0));

	////メッシュコンポーネントの追加
	//auto m_sphere = a_sphere.Target()->AddComponent<sf::Mesh>();

	////形状の設定
	//m_sphere->SetGeometry(g_sphere);

	////マテリアル色変更
	//m_sphere->material.diffuseColor = DirectX::XMFLOAT4(1, 0, 0, 1);



	//モデル読み込み
	g_human.Load();

	//モーション読み込み
	ma_human.LoadMotions();

	//アクター生成
	auto a_human = Instantiate();

	//スケール設定
	a_human.Target()->transform.SetScale(Vector3::One * 0.01f);

	//メッシュコンポーネントの追加
	auto m_human = a_human.Target()->AddComponent<sf::Mesh>();

	//形状の設定
	m_human->SetGeometry(g_human);

	//モーションコンポーネント追加
	auto mo_human = a_human.Target()->AddComponent<sf::motion::Motion>();

	//モーションコンポーネントにモーションアレイを設定
	mo_human->SetGeomeyryMotion(g_human, ma_human);

	//モーション切り替えコンポーネント追加
	a_human.Target()->AddComponent<ChangeMotionComponent>();
}
