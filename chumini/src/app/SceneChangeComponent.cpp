#include "SceneChangeComponent.h"
#include "DynamicScene.h"
#include "Actor.h"
#include "LoadingScene.h"

void app::test::SceneChangeComponent::Begin() {

    updateCommand.Bind(std::bind(&SceneChangeComponent::Update, this, std::placeholders::_1));
}

void app::test::SceneChangeComponent::ChangeScene(sf::SafePtr<sf::IScene> next) {
    if (next.isNull()) return;

    // LoadingScene に「次に行く場所」を預ける
    LoadingScene::SetNextScene(next);

    // LoadingScene を即座に起動
    // (演出が必要なら、LoadingScene側が開始時にフェードインすればいい)
    auto loader = LoadingScene::StandbyScene();
    loader->Activate();

    // 自身のシーンを終了
    if (auto owner = actorRef.Target()) {
        owner->GetScene().DeActivate();
    }
}

void app::test::SceneChangeComponent::Update(const sf::command::ICommand&) {
    // シーンがセットされていなければ何もしない
    if (nextScene.isNull()) return;

    // シーンをスタンバイ状態にする (まだアクティブでなく、読み込みも終わっていない場合)
    if (!nextScene->IsActivate() && !nextScene->StandbyThisScene()) {
        // ロード中なので次のフレームへ
        return;
    }

    // 読み込みが完了していたらアクティブ化
    if (nextScene->StandbyThisScene()) {
        // 現在のシーン（自分自身が所属しているシーン）を終了させる
        if (auto actor = actorRef.Target()) {
            auto& currentScene = actor->GetScene();
            currentScene.DeActivate();
        }

        // 新しいシーンを実体化させる
        nextScene->Activate();

        // 遷移完了したのでポインタをクリア
        nextScene = nullptr;
    }
}