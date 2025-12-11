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
	// 名前・学校名の表示
	// ---------------------------------------------------------
	auto titleText = AddUI<sf::ui::TextImage>();
	titleText->transform.SetPosition(Vector3(-650, -400, 0));
	titleText->transform.SetScale(Vector3(10, 2, 0));

	titleText->Create(
		context,
		L"萬徳倫功",
		L"叛逆明朝",
		100.0f,
		D2D1::ColorF(D2D1::ColorF::Tomato),
		1024, 256);


	// ---------------------------------------------------------
	// 1. タイトルロゴ
	// ---------------------------------------------------------
	titleLogo = AddUI<sf::ui::TextImage>();
	titleLogo->transform.SetPosition(Vector3(0, 250, 0)); // 上の方に配置
	titleLogo->transform.SetScale(Vector3(15, 5, 0));     // 大きく表示

	titleLogo->Create(
		context,
		L"COSMIC DRIVE",         // タイトル文字列
		L"851ゴチカクット",              // フォント名
		120.0f,                 // フォントサイズ
		D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 色
		1500, 256               // テクスチャサイズ
	);

	// ---------------------------------------------------------
	// 2. プレイボタン
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
	// 3. エディットボタン
	// ---------------------------------------------------------
	editButton = AddUI<sf::ui::TextImage>();
	editButton->transform.SetPosition(Vector3(0, -300, 0)); // 下の方
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
	animationTimer += sf::Time::DeltaTime();

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
	// ---------------------------------------------------------
	// アニメーション計算 (sin波)
	// ---------------------------------------------------------
	// 速度: 10.0f
	float sine = std::sin(animationTimer * 7.0f);

	// サイズ変動
	float scaleOffset = 0.1f * sine;

	// 透明度を 0.5 ～ 1.0 の間で揺らす
	// (sine + 1.0f) * 0.5f で 0~1 になる
	float alphaAnim = 0.5f + 0.5f * ((sine + 1.0f) * 0.5f);

	// ---------------------------------------------------------
	// パラメータ定義
	// ---------------------------------------------------------

	// 基本サイズ
	Vector3 scaleNormal(4.0f, 1.5f, 0);       // 非選択時
	Vector3 scaleSelectedBase(4.5f, 1.7f, 0); // 選択時の基準

	// 現在のアニメーション適用後の選択時サイズ
	Vector3 scaleSelectedAnim = Vector3(
		scaleSelectedBase.x + scaleOffset,
		scaleSelectedBase.y + scaleOffset * 0.4f, // 縦の変動は少し控えめに
		0
	);

	// 色定義
	// 選択中：黄色 + アニメーション透明度
	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, alphaAnim);
	// 非選択：灰色（半透明固定）
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f);


	// ---------------------------------------------------------
	// 適用
	// ---------------------------------------------------------
	if (selectedButton == 0) {
		// --- Edit選択中 (アニメーション) ---
		editButton->transform.SetScale(scaleSelectedAnim);
		editButton->material.SetColor(colorSelected);

		// --- Play非選択 (固定) ---
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);
	}
	else {
		// --- Play選択中 (アニメーション) ---
		playButton->transform.SetScale(scaleSelectedAnim);
		playButton->material.SetColor(colorSelected);

		// --- Edit非選択 (固定) ---
		editButton->transform.SetScale(scaleNormal);
		editButton->material.SetColor(colorNormal);
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

	// TextImageの当たり判定
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