#include "FollowCameraComponent.h"
#include "Camera.h"
#include "sf/Time.h"
#include <DirectXMath.h>
#include <sstream>

using namespace DirectX;

void app::test::FollowCameraComponent::Begin()
{
    updateCommand.Bind(std::bind(&FollowCameraComponent::Update, this, std::placeholders::_1));

    //actorRef.Target()->transform.SetPosition({ 0.0f, 0.0f, -10.0f });
    //sf::Camera::main->LookAt({ 0.0f, 0.0f,  0.0f });
}

void app::test::FollowCameraComponent::Update(const sf::command::ICommand& command)
{
    if (!actorRef.Target()) return;

    // --- マウス移動量取得 ---
    POINT pt;
    GetCursorPos(&pt); // Windowsのスクリーン座標で取得
    // 必要に応じてクライアント座標へ変換する場合はScreenToClientを使う

    Vector2 mousePos;
    mousePos.x = static_cast<float>(pt.x);
    mousePos.y = static_cast<float>(pt.y);

    if (firstUpdate) {
        prevMousePos = mousePos;
        firstUpdate = false;
    }

    Vector2 mouseDelta = mousePos - prevMousePos;
    prevMousePos = mousePos;

    // 感度調整
    const float sensitivity = 0.2f;
    cameraYaw += mouseDelta.x * sensitivity;
    cameraPitch += mouseDelta.y * sensitivity;

    // ピッチ制限
    cameraPitch = std::clamp(cameraPitch, -60.0f, 80.0f);

    // --- プレイヤーの位置取得 ---
    Vector3 playerPos = actorRef.Target()->transform.GetPosition();

    // --- カメラのオフセット計算 ---
    float distance = 5.0f;
    float height = 0.0f; // 高さはピッチで調整するので0

    // 回転行列でカメラ位置を算出
    float yawRad = XMConvertToRadians(cameraYaw);
    float pitchRad = XMConvertToRadians(cameraPitch);

    // 球面座標系でカメラ位置を計算
    Vector3 offset;
    offset.x = distance * std::cos(pitchRad) * std::sin(yawRad);
    offset.y = distance * std::sin(pitchRad);
    offset.z = distance * std::cos(pitchRad) * std::cos(yawRad);

    Vector3 camPos = playerPos + offset;

    // 緩やかに追従（リニア補間）
    Vector3 nowPos = sf::Camera::main->GetPosition();
    float followSpeed = 2.5f;
    nowPos += (camPos - nowPos) * followSpeed * sf::Time::DeltaTime();
    sf::Camera::main->SetPosition(nowPos);

    // プレイヤーを見る
    sf::Camera::main->LookAt(playerPos);
}
