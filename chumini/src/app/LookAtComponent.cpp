#include "LookAtComponent.h"
#include <sstream>   // 追加

void app::camera::LookAtComponent::Begin()
{

    // 文字列を組み立て
    std::ostringstream oss;
    oss << "Offset("
        << offset.x << "," << offset.y << "," << offset.z;

    // 組み立てた文字列をログ出力
    sf::debug::Debug::Log(oss.str());

    // Update() を毎フレーム呼ぶようバインド
    updateCommand.Bind(std::bind(&LookAtComponent::Update, this, std::placeholders::_1));

}

void app::camera::LookAtComponent::Update(const sf::command::ICommand& /*command*/)
{
    auto camActor = actorRef.Target();
    auto plyActor = playerRef.Target();
    if (!camActor || !plyActor) return;

    // プレイヤー位置取得
    Vector3 ppos = plyActor->transform.GetPosition();

    // カメラをプレイヤーの真上かつ背後に配置
    camActor->transform.SetPosition(ppos + offset);

    // プレイヤーを注視
    sf::Camera::main->LookAt(ppos);
    sf::Camera::main->SetGPU();
}
