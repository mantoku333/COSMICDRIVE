#include "SelectScene.h"
#include "SelectCanvas.h"
#include "SInput.h"
#include "Debug.h"
#include "IngameScene.h"
#include "TitleScene.h"
#include "LoadingScene.h"
#include "Config.h"

namespace app::test {

void SelectScene::Init()
{
    sf::debug::Debug::Log("SelectScene: Init");

    uiManagerActor = Instantiate();
    selectCanvas = uiManagerActor.Target()->AddComponent<SelectCanvas>();
    sceneChanger = uiManagerActor.Target()->AddComponent<SceneChangeComponent>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    // MVP: CanvasにPresenterを設定
    if (selectCanvas.Get()) {
        selectCanvas->SetPresenter(this);
    }

    // 状態初期化
    targetIndex = 0;
    inputCooldown = 0.0f;
    isGenreSelectMode = false;
    showRatingDetail = false;

    updateCommand.Bind(std::bind(&SelectScene::Update, this, std::placeholders::_1));
}

void SelectScene::Update(const sf::command::ICommand& command)
{
    // 入力クールダウン更新
    if (inputCooldown > 0.0f) {
        inputCooldown -= sf::Time::DeltaTime();
    }

    // 入力処理
    ProcessInput();
}

// =================================================================
// MVP: 入力処理（メインループから呼ばれる）
// =================================================================
void SelectScene::ProcessInput()
{
    if (inputCooldown > 0.0f) return;
    SInput& input = SInput::Instance();

    if (app::test::gGameConfig.isControllerMode) {
         // --- Controller Mode ---
         if (input.GetKeyDown(Key::KEY_J)) {
              if (isGenreSelectMode) OnGenrePrevious();
              else OnSelectPrevious();
         }
         if (input.GetKeyDown(Key::KEY_K)) {
              if (isGenreSelectMode) OnGenreNext();
              else OnSelectNext();
         }
         if (input.GetKeyDown(Key::KEY_D)) {
              OnConfirmSelection();
         }
         if (input.GetKeyDown(Key::KEY_N)) {
              if (isGenreSelectMode) OnModeDown();
              else OnModeUp();
         }
         if (input.GetKeyDown(Key::KEY_V)) {
              OnToggleRatingDetail();
         }
    }
    else {
        // --- Keyboard Mode (Standard) ---
        // モード切り替え (上下)
        if (input.GetKeyDown(Key::KEY_UP) || input.GetKeyDown(Key::KEY_W)) {
            OnModeUp();
        }
        if (input.GetKeyDown(Key::KEY_DOWN) || input.GetKeyDown(Key::KEY_S)) {
            OnModeDown();
        }

        // 左右操作 (モード依存)
        if (input.GetKeyDown(Key::KEY_RIGHT) || input.GetKeyDown(Key::KEY_D)) {
            if (isGenreSelectMode) {
                OnGenreNext();
            } else {
                OnSelectNext();
            }
        }
        if (input.GetKeyDown(Key::KEY_LEFT) || input.GetKeyDown(Key::KEY_A)) {
            if (isGenreSelectMode) {
                OnGenrePrevious();
            } else {
                OnSelectPrevious();
            }
        }

        // 決定キャンセル
        if (input.GetKeyDown(Key::SPACE) || input.GetKeyDown(Key::KEY_Z)) {
            OnConfirmSelection();
        }
        if (input.GetKeyDown(Key::ESCAPE) || input.GetKeyDown(Key::KEY_X)) {
            OnCancelSelection();
        }

        // レート詳細表示トグル
        if (input.GetKeyDown(Key::KEY_R)) {
            OnToggleRatingDetail();
        }
    }
}

// =================================================================
// MVP: 入力ハンドラ
// =================================================================

void SelectScene::OnSelectNext()
{
    if (!selectCanvas.Get()) return;
    int songCount = selectCanvas->GetSongCount();
    if (songCount <= 0) return;
    
    targetIndex = (targetIndex + 1) % songCount;
    selectCanvas->SetTargetIndex(targetIndex);
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnSelectPrevious()
{
    if (!selectCanvas.Get()) return;
    int songCount = selectCanvas->GetSongCount();
    if (songCount <= 0) return;
    
    targetIndex = (targetIndex - 1 + songCount) % songCount;
    selectCanvas->SetTargetIndex(targetIndex);
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnConfirmSelection()
{
    if (!selectCanvas.Get()) return;
    const SongInfo& song = selectCanvas->GetSelectedSong();
    NavigateToGame(song);
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnCancelSelection()
{
    NavigateToTitle();
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnModeUp()
{
    if (!isGenreSelectMode) {
        isGenreSelectMode = true;
        if (selectCanvas.Get()) {
            selectCanvas->SetGenreSelectMode(true);
        }
        inputCooldown = INPUT_DELAY;
    }
}

void SelectScene::OnModeDown()
{
    if (isGenreSelectMode) {
        isGenreSelectMode = false;
        if (selectCanvas.Get()) {
            selectCanvas->SetGenreSelectMode(false);
        }
        inputCooldown = INPUT_DELAY;
    }
}

void SelectScene::OnGenreNext()
{
    if (selectCanvas.Get()) {
        selectCanvas->ChangeGenre(1);
        targetIndex = 0;  // ジャンル変更時はインデックスリセット
    }
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnGenrePrevious()
{
    if (selectCanvas.Get()) {
        selectCanvas->ChangeGenre(-1);
        targetIndex = 0;
    }
    inputCooldown = INPUT_DELAY;
}

void SelectScene::OnToggleRatingDetail()
{
    showRatingDetail = !showRatingDetail;
    if (selectCanvas.Get()) {
        selectCanvas->SetShowRatingDetail(showRatingDetail);
    }
    inputCooldown = INPUT_DELAY;
}

// =================================================================
// MVP: シーン遷移
// =================================================================

void SelectScene::NavigateToGame(const SongInfo& song)
{
    if (sceneChanger.isNull()) return;

    auto next = IngameScene::StandbyScene();
    if (next) {
        next->SetSelectedSong(song);
    }

    LoadingScene::SetLoadingType(LoadingType::InGame);
    LoadingScene::SetNextSongInfo(song);

    sceneChanger->ChangeScene(next);
}

void SelectScene::NavigateToTitle()
{
    if (sceneChanger.isNull()) return;
    sceneChanger->ChangeScene(TitleScene::StandbyScene());
}

} // namespace app::test