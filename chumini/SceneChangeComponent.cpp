#include "SceneChangeComponent.h"
#include "DynamicScene.h"
#include "Actor.h"  // GetActor() の戻り値を扱うために必要

void app::test::SceneChangeComponent::Begin() {
    updateCommand.Bind(std::bind(&SceneChangeComponent::Update, this));
}

void app::test::SceneChangeComponent::ChangeScene(sf::SafePtr<sf::IScene> scene) {
    // ! 演算子の代わりに isNull() を使用
    if (scene.isNull()) return;

    this->nextScene = scene; // メンバ変数の nextScene (または scene) に代入
    // isChanging フラグ等があればここで true にする
}

void app::test::SceneChangeComponent::Update() {
    // 1. シーンがセットされていなければ何もしない
    if (nextScene.isNull()) return;

    // 2. シーンをスタンバイ状態にする (まだアクティブでなく、読み込みも終わっていない場合)
    if (!nextScene->IsActivate() && !nextScene->StandbyThisScene()) {
        // ロード中なので次のフレームへ
        return;
    }

    // 3. 読み込みが完了していたらアクティブ化
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