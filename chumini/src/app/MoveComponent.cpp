#include "MoveComponent.h"
#include <fstream>

//// 例: ファイルから座標を読み込む関数
//Vector3 LoadPositionFromFile() {
//	std::ifstream ifs("position.txt");
//	Vector3 pos;
//	ifs >> pos.x >> pos.y >> pos.z;
//	return pos;
//}
//
//// 例: ファイルに座標を保存する関数
//void SavePositionToFile(const Vector3& pos) {
//	std::ofstream ofs("position.txt");
//	ofs << pos.x << " " << pos.y << " " << pos.z;
//}

void app::test::MoveComponent::Begin()
{
	updateCommand.Bind(std::bind(&MoveComponent::Update, this, std::placeholders::_1));
	// ここで座標を読み込んでセット
	//actorRef.Target()->transform.SetPosition(LoadPositionFromFile());

}

void app::test::MoveComponent::Update(const sf::command::ICommand& command)
{
	const float speed = 1.0f;

	//// 右方向に0.1ずつ移動
	//auto position = actorRef.Target()->transform.GetPosition(); // 座標取得
	//position.x += 0.01f;
	//actorRef.Target()->transform.SetPosition(position); // 座標反映

	//Vector3 imguiPos = actorRef.Target()->transform.GetPosition();
	//static bool posChanged = false;
	//if (ImGui::DragFloat3("Position", &imguiPos.x, 0.1f)) {
	//	actorRef.Target()->transform.SetPosition(imguiPos);
	//	posChanged = true;
	//}
	//if (posChanged) {
	//	SavePositionToFile(imguiPos); // ここで保存
	//	posChanged = false;
	//}







	//if (SInput::Instance().GetKey(Key::KEY_W))
	//{
	//	//Wキーが押された時の処理

	//	//座標の取得
	//	auto position = actorRef.Target()->transform.GetPosition();

	//	//数値の計算
	//	position += Vector3::Forward * speed * sf::Time::DeltaTime();

	//	//座標の反映
	//	actorRef.Target()->transform.SetPosition(position);
	//}
	//if (SInput::Instance().GetKey(Key::KEY_A))
	//{
	//	//Aキーが押された時の処理

	//	//座標の取得
	//	auto position = actorRef.Target()->transform.GetPosition();

	//	//数値の計算
	//	position += Vector3::Left * speed * sf::Time::DeltaTime();

	//	//座標の反映
	//	actorRef.Target()->transform.SetPosition(position);
	//}
	//if (SInput::Instance().GetKey(Key::KEY_S))
	//{
	//	//Sキーが押された時の処理

	//	//座標の取得
	//	auto position = actorRef.Target()->transform.GetPosition();

	//	//数値の計算
	//	position += Vector3::Back * speed * sf::Time::DeltaTime();

	//	//座標の反映
	//	actorRef.Target()->transform.SetPosition(position);
	//}
	//if (SInput::Instance().GetKey(Key::KEY_D))
	//{
	//	//Dキーが押された時の処理

	//	//座標の取得
	//	auto position = actorRef.Target()->transform.GetPosition();

	//	//数値の計算
	//	position += Vector3::Right * speed * sf::Time::DeltaTime();

	//	//座標の反映
	//	actorRef.Target()->transform.SetPosition(position);
	//}

	if (SInput::Instance().GetKeyDown(Key::SPACE))
	{
		command.UnBind();
	}
}
