#include "TitleCanvas.h"
#include "SceneChangeComponent.h"
#include "SelectScene.h"
#include "EditScene.h"

// 必要なインクルード
#include "DirectX11.h"       // デバイス取得用
#include "SInput.h"          // 入力取得用
#include "DWriteContext.h"   // フォント描画用

using namespace app::test;
using namespace sf;

void TitleCanvas::Begin()
{
	// 基底クラスのBeginを必ず呼び出す
	sf::ui::Canvas::Begin();

	// DirectXデバイスの取得
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto context = dx11->GetMainDevice().GetDevice();

	// ---------------------------------------------------------
	// ★復活：名前・学校名の表示
	// ---------------------------------------------------------
	auto titleText = AddUI<sf::ui::TextImage>();
	titleText->transform.SetPosition(Vector3(-550, -300, 0));
	titleText->transform.SetScale(Vector3(10, 2, 0));

	titleText->Create(
		context,
		L"萬徳倫功",
		L"851ゴチカクット",
		80.0f,
		D2D1::ColorF(D2D1::ColorF::Tomato),
		1024, 256);


	// ---------------------------------------------------------
	// 1. タイトルロゴ (テキスト化)
	// ---------------------------------------------------------
	titleLogo = AddUI<sf::ui::TextImage>();
	titleLogo->transform.SetPosition(Vector3(0, 300, 0)); // 上の方に配置
	titleLogo->transform.SetScale(Vector3(15, 5, 0));     // 大きく表示

	titleLogo->Create(
		context,
		L"†COSMIC 滅†",         // タイトル文字列
		L"851ゴチカクット",              // フォント名
		120.0f,                 // フォントサイズ
		D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 色
		1024, 256               // テクスチャサイズ
	);

	// ---------------------------------------------------------
	// 2. プレイボタン (テキスト化)
	// ---------------------------------------------------------
	playButton = AddUI<sf::ui::TextImage>();
	playButton->transform.SetPosition(Vector3(0, -100, 0)); // 中央
	playButton->transform.SetScale(Vector3(4, 1.5f, 0));

	playButton->Create(
		context,
		L"PLAY",
		L"851ゴチカクット",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// ---------------------------------------------------------
	// 3. エディットボタン (テキスト化)
	// ---------------------------------------------------------
	editButton = AddUI<sf::ui::TextImage>();
	editButton->transform.SetPosition(Vector3(0, -400, 0)); // 下の方
	editButton->transform.SetScale(Vector3(4, 1.5f, 0));

	editButton->Create(
		context,
		L"EDIT",
		L"851ゴチカクット",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// ---------------------------------------------------------
	// 初期設定
	// ---------------------------------------------------------
	selectedButton = 1; // 初期選択はPLAYにしておく
	UpdateButtonSelection();

	// シーンのスタンバイ
	if (scene.isNull()) {
		scene = SelectScene::StandbyScene();
	}
	if (sceneEdit.isNull()) {
		sceneEdit = EditScene::StandbyScene();
	}

	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void TitleCanvas::Update(const sf::command::ICommand& command)
{
	HandleInput(command);
	UpdateButtonSelection();
}

void TitleCanvas::HandleInput(const sf::command::ICommand& command)
{
	// --- マウス操作 ---
	Vector2 mousePos = GetMousePosition();
	bool isEditHovered = IsButtonHovered(mousePos, editButton);
	bool isPlayHovered = IsButtonHovered(mousePos, playButton);

	// マウスが乗ったら選択状態を切り替える
	if (isEditHovered) {
		selectedButton = 0;
	}
	else if (isPlayHovered) {
		selectedButton = 1;
	}

	// 左クリックで決定
	if (SInput::Instance().GetMouseDown(0)) {
		if (isEditHovered || isPlayHovered) {
			OnButtonPressed();
		}
	}

	// --- キーボード操作 ---
	const int BUTTON_COUNT = 2;

	// 上下キーまたはWSキーで選択切り替え
	if (SInput::Instance().GetKeyDown(Key::KEY_UP) || SInput::Instance().GetKeyDown(Key::KEY_W)) {
		selectedButton = (selectedButton - 1 + BUTTON_COUNT) % BUTTON_COUNT;
	}
	else if (SInput::Instance().GetKeyDown(Key::KEY_DOWN) || SInput::Instance().GetKeyDown(Key::KEY_S)) {
		selectedButton = (selectedButton + 1) % BUTTON_COUNT;
	}

	// スペースキーまたはエンターキーで決定
	if (SInput::Instance().GetKeyDown(Key::SPACE)) {
		OnButtonPressed();
	}
}

void TitleCanvas::UpdateButtonSelection()
{
	// 選択されているボタンを強調表示（色とサイズを変更）

	// 色定義
	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, 1.0f); // 黄色（不透明）
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f); // 灰色（半透明）

	if (selectedButton == 0) {
		// --- Edit選択中 ---
		editButton->material.SetColor(colorSelected);
		editButton->transform.SetScale(Vector3(4.5f, 1.7f, 0)); // 少し大きく

		playButton->material.SetColor(colorNormal);
		playButton->transform.SetScale(Vector3(4.0f, 1.5f, 0)); // 普通サイズ
	}
	else {
		// --- Play選択中 ---
		playButton->material.SetColor(colorSelected);
		playButton->transform.SetScale(Vector3(4.5f, 1.7f, 0));

		editButton->material.SetColor(colorNormal);
		editButton->transform.SetScale(Vector3(4.0f, 1.5f, 0));
	}
}

void TitleCanvas::OnButtonPressed()
{
	if (selectedButton == 0) {
		ShowEditScene();
	}
	else {
		ShowSongSelectScene();
	}
}

bool TitleCanvas::IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button)
{
	if (!button) return false;

	Vector3 pos = button->transform.GetPosition();
	Vector3 scale = button->transform.GetScale();

	// TextImageの当たり判定サイズ調整
	// ※見た目に合わせて数値を調整してください (ここでは 幅80, 高さ40 を基準)
	float width = scale.x * 80.0f;
	float height = scale.y * 40.0f;

	float left = pos.x - width * 0.5f;
	float right = pos.x + width * 0.5f;
	float top = pos.y + height * 0.5f;
	float bottom = pos.y - height * 0.5f;

	return (mousePos.x >= left && mousePos.x <= right &&
		mousePos.y >= bottom && mousePos.y <= top);
}

Vector2 TitleCanvas::GetMousePosition()
{
	POINT mousePoint;
	GetCursorPos(&mousePoint);
	HWND hwnd = GetActiveWindow();
	ScreenToClient(hwnd, &mousePoint);

	// 画面中心を原点(0,0)とし、Y軸を上がプラスになるよう変換
	float uiX = static_cast<float>(mousePoint.x) - screenWidth * 0.5f;
	float uiY = screenHeight * 0.5f - static_cast<float>(mousePoint.y);

	return Vector2(uiX, uiY);
}

void TitleCanvas::ShowSongSelectScene()
{
	if (scene->StandbyThisScene()) {
		scene->Activate();
	}
	auto actor = actorRef.Target();
	if (actor) {
		actor->GetScene().DeActivate();
	}
}

void TitleCanvas::ShowEditScene()
{
	if (sceneEdit->StandbyThisScene()) {
		sceneEdit->Activate();
	}
	auto actor = actorRef.Target();
	if (actor) {
		actor->GetScene().DeActivate();
	}
}

int TitleCanvas::GetSelectedButton() const
{
	return selectedButton;
}

void TitleCanvas::SetSelectedButton(int buttonIndex)
{
	if (buttonIndex >= 0 && buttonIndex <= 1) {
		selectedButton = buttonIndex;
		UpdateButtonSelection();
	}
}