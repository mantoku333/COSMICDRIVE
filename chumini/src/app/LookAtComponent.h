// LookAtComponent.h
#pragma once
#include "App.h"
#include "Camera.h"

namespace app::camera
{
    class LookAtComponent : public sf::Component
    {
    public:
        // Begin() で毎フレームの更新コマンドをバインド
        void Begin() override;

        // 外部から設定する注視対象プレイヤー
        sf::ref::Ref<sf::Actor> playerRef;
        Vector3 offset{ 0.0f, 50.0f, 0.0f };

    private:
        // 毎フレーム呼ばれる
        void Update(const sf::command::ICommand& command);

        // 更新用コマンド
        sf::command::Command<> updateCommand;
    };
}
