#include "TitleCanvas.h"
#include "SceneChangeComponent.h"
#include "SelectScene.h"
#include "EditScene.h"

void app::test::TitleCanvas::Begin()
{
    //基底クラスのBeginを必ず呼び出す
    sf::ui::Canvas::Begin();

    // テクスチャの読み込み
    textureEditButton.LoadTextureFromFile("Assets\\Texture\\edit.png");
    texturePlayButton.LoadTextureFromFile("Assets\\Texture\\play.png");
    textureTitleLogo.LoadTextureFromFile("Assets\\Texture\\title.png");

    // タイトルロゴの追加
    titleLogo = AddUI<sf::ui::Image>();
    titleLogo->transform.SetPosition(Vector3(-200, 350, 0));
    titleLogo->transform.SetScale(Vector3(9, 2, 0));
    titleLogo->material.texture = &textureTitleLogo;

    // プレイボタンの追加
    playButton = AddUI<sf::ui::Image>();
    playButton->transform.SetPosition(Vector3(0, 100, 0));
    playButton->transform.SetScale(Vector3(3, 1, 0));
    playButton->material.texture = &texturePlayButton;

    // エディットボタンの追加
    editButton = AddUI<sf::ui::Image>();
    editButton->transform.SetPosition(Vector3(0, -50, 0));
    editButton->transform.SetScale(Vector3(3, 1, 0));
    editButton->material.texture = &textureEditButton;

    

    // 初期選択状態の設定
    selectedButton = 0; // 0: エディット, 1: プレイ
    UpdateButtonSelection();


    // シーンがメモリ上に存在しなければ
    if (scene.isNull())
    {
        //シーンをスタンバイ状態にする
        scene = SelectScene::StandbyScene();
    }

    if (sceneEdit.isNull())
    {
        //EDITシーンをスタンバイ
        sceneEdit = EditScene::StandbyScene();
    }


    updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void app::test::TitleCanvas::Update(const sf::command::ICommand& command)
{
    // キー入力処理
    HandleInput(command);

    // ボタン選択状態の更新
    UpdateButtonSelection();
}

void app::test::TitleCanvas::HandleInput(const sf::command::ICommand& command)
{
    // マウス座標を取得
    Vector2 mousePos = GetMousePosition();

    // エディットボタンの当たり判定
    bool isEditButtonHovered = IsButtonHovered(mousePos, editButton);
    // プレイボタンの当たり判定
    bool isPlayButtonHovered = IsButtonHovered(mousePos, playButton);

    // ホバー状態でボタン選択を変更
    if (isEditButtonHovered && selectedButton != 0) {
        selectedButton = 0;
    }
    else if (isPlayButtonHovered && selectedButton != 1) {
        selectedButton = 1;
    }

    // マウス左クリックで決定（SInputシステムを使用）
    if (SInput::Instance().GetMouseDown(0)) { // 0 = 左クリック
        if (isEditButtonHovered) {
            selectedButton = 0;
            OnButtonPressed();
        }
        else if (isPlayButtonHovered) {
            selectedButton = 1;
            OnButtonPressed();
        }
    }

    const int BUTTON_COUNT = 2; // ボタンの総数

    // キーボード操作
    if (SInput::Instance().GetKeyDown(Key::KEY_UP) || SInput::Instance().GetKeyDown(Key::KEY_W)) {
        selectedButton = (selectedButton - 1 + BUTTON_COUNT) % BUTTON_COUNT;
    }
    else if (SInput::Instance().GetKeyDown(Key::KEY_DOWN) || SInput::Instance().GetKeyDown(Key::KEY_S)) {
        selectedButton = (selectedButton + 1) % BUTTON_COUNT;
    }

    // Enterキーまたはスペースキーで決定
    if (SInput::Instance().GetKeyDown(Key::SPACE)) {
        OnButtonPressed();
    }
}

bool app::test::TitleCanvas::IsButtonHovered(const Vector2& mousePos, sf::ui::Image* button)
{
    // ボタンの位置とスケールを取得
    Vector3 buttonPos = button->transform.GetPosition();
    Vector3 buttonScale = button->transform.GetScale();

    // ボタンの境界を計算（UI座標系での矩形）
    float buttonWidth = buttonScale.x * 100;  // 基準サイズに合わせて調整
    float buttonHeight = buttonScale.y * 50;  // 基準サイズに合わせて調整

    float left = buttonPos.x - buttonWidth * 0.5f;
    float right = buttonPos.x + buttonWidth * 0.5f;
    float top = buttonPos.y + buttonHeight * 0.5f;
    float bottom = buttonPos.y - buttonHeight * 0.5f;

    // マウス座標がボタン範囲内にあるかチェック
    bool isInside = (mousePos.x >= left && mousePos.x <= right &&
        mousePos.y >= bottom && mousePos.y <= top);

    return isInside;
}

Vector2 app::test::TitleCanvas::GetMousePosition()
{
    // Win32 APIを使ってマウス座標を取得
    POINT mousePoint;
    GetCursorPos(&mousePoint);

    // ウィンドウのクライアント領域座標に変換
    HWND hwnd = GetActiveWindow(); // または適切なウィンドウハンドルを取得
    ScreenToClient(hwnd, &mousePoint);

    float uiX = static_cast<float>(mousePoint.x) - screenWidth * 0.5f;  // 画面中央を原点とする場合
    float uiY = screenHeight * 0.5f - static_cast<float>(mousePoint.y); // Y軸を反転する場合

    return Vector2(uiX, uiY);
}

void app::test::TitleCanvas::UpdateButtonSelection()
{
    if (selectedButton == 0) {
        // エディットボタンが選択されている場合  

        // スケール変更：選択されたボタンを大きく表示  
        editButton->transform.SetScale(Vector3(3.2f, 1.1f, 0));  // 選択時：大きく  
        playButton->transform.SetScale(Vector3(3.0f, 1.0f, 0));  // 非選択時：通常サイズ  

        // 色の変更：選択されたボタンを明るく、非選択を暗く  
        editButton->material.SetColor(DirectX::XMFLOAT4(1.3f, 1.3f, 1.0f, 1.0f));  // 選択時：明るい黄色味  
        playButton->material.SetColor(DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 0.3f));   // 非選択時：暗いグレー  
    }
    else {
        // プレイボタンが選択されている場合  

        // スケール変更  
        editButton->transform.SetScale(Vector3(3.0f, 1.0f, 0));   // 非選択時：通常サイズ  
        playButton->transform.SetScale(Vector3(3.2f, 1.1f, 0));  // 選択時：大きく  

        // 色の変更  
        playButton->material.SetColor(DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));   // 非選択時：暗いグレー  
        editButton->material.SetColor(DirectX::XMFLOAT4(1.3f, 1.3f, 1.0f, 0.3f));   // 選択時：明るい黄色味
    }
}

void app::test::TitleCanvas::OnButtonPressed()
{
    if (selectedButton == 0) {
        // EDITシーンへ
        ShowEditScene();
    }
    else {
        // SELECTシーンへ
        ShowSongSelectScene();
    }
}

void app::test::TitleCanvas::ShowSongSelectScene()
{
    // シーンの読み込みが完了していたら
    if (scene->StandbyThisScene())
    {
        // シーンを実体化させる
        scene->Activate();
    }
    
    // 今いるシーンを消す
    auto actor = actorRef.Target();
    if (!actor) return;
    auto* thisscene = &actor->GetScene();

    thisscene->DeActivate();
}

void app::test::TitleCanvas::ShowEditScene()
{
    // ★ EDITシーンの起動
    if (sceneEdit->StandbyThisScene()) {
        sceneEdit->Activate();
    }

    // 今いるシーンを消す
    auto actor = actorRef.Target();
    if (!actor) return;
    auto* thisscene = &actor->GetScene();
    thisscene->DeActivate();
}

int app::test::TitleCanvas::GetSelectedButton() const
{
    return selectedButton;
}

void app::test::TitleCanvas::SetSelectedButton(int buttonIndex)
{
    if (buttonIndex >= 0 && buttonIndex <= 1) {
        selectedButton = buttonIndex;
        UpdateButtonSelection();
    }
}