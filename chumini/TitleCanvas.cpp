#include "TitleCanvas.h"
#include "SceneChangeComponent.h"
#include "SelectScene.h"
#include "EditScene.h"
#include "LoadingScene.h"

// 必要なインクルード
#include "DirectX11.h"       // デバイス取得用
#include "SInput.h"          // 入力取得用
#include "DWriteContext.h"   // フォント描画用
#include <filesystem>
#include "App.h"

using namespace app::test;
using namespace sf;


// ヘルパー関数 (SelectCanvasからコピー)
static std::wstring Utf8ToWstring(const std::string& str) {
	if (str.empty()) return L"";
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wstr(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
	return wstr;
}

static std::string Utf8ToShiftJis(const std::string& utf8Str) {
	std::wstring wstr = Utf8ToWstring(utf8Str);
	if (wstr.empty()) return "";
	int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string str(sizeNeeded, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
	if (!str.empty() && str.back() == '\0') str.pop_back();
	return str;
}

void TitleCanvas::InitializeJacketFlow() {
	namespace fs = std::filesystem;
	std::string rootPath = "Songs";
	std::vector<std::string> jacketPaths;

	// 1. フォルダ走査（既存ロジック）
	if (fs::exists(rootPath)) {
		for (const auto& dir : fs::directory_iterator(rootPath)) {
			if (!dir.is_directory()) continue;
			for (const auto& file : fs::directory_iterator(dir.path())) {
				std::string ext = file.path().extension().string();
				if (ext == ".png" || ext == ".jpg") {
					jacketPaths.push_back(file.path().string());
					break;
				}
			}
		}
	}

	// 2. テクスチャロード
	jacketTextures.resize(jacketPaths.size());
	for (size_t i = 0; i < jacketPaths.size(); ++i) {
		jacketTextures[i].LoadTextureFromFile(Utf8ToShiftJis(jacketPaths[i]));
	}
	if (jacketTextures.empty()) return;

	// 3. 枚数とループ幅の計算
	int minImages = static_cast<int>(2400.0f / jacketInterval) + 2;
	int numUI = std::max(static_cast<int>(jacketTextures.size()), minImages);
	totalWidth = numUI * jacketInterval;

	for (int i = 0; i < numUI; ++i) {
		sf::Texture* tex = &jacketTextures[i % jacketTextures.size()];
		float startX = (-totalWidth * 0.5f) + (i * jacketInterval);

		// --- 上段の生成 ---
		auto* imgTop = AddUI<sf::ui::Image>();
		imgTop->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgTop->material.texture = tex;
		scrollingJacketsTop.push_back({ imgTop, startX });

		// --- 下段の生成 ---
		auto* imgBottom = AddUI<sf::ui::Image>();
		imgBottom->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgBottom->material.texture = tex;
		scrollingJacketsBottom.push_back({ imgBottom, startX });
	}
}

void TitleCanvas::Begin()
{
	// 基底クラスのBeginを必ず呼び出す
	sf::ui::Canvas::Begin();

	// 背景ジャケット初期化
	InitializeJacketFlow();

	// DirectXデバイスの取得
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto context = dx11->GetMainDevice().GetDevice();

	if (auto actor = actorRef.Target()) {
		auto* changer = actor->GetComponent<SceneChangeComponent>();
		if (changer) {
			sceneChanger = changer;
		}
	}

	// ---------------------------------------------------------
	// 名前・学校名の表示
	// ---------------------------------------------------------
	auto titleText = AddUI<sf::ui::TextImage>();
	titleText->transform.SetPosition(Vector3(-650, -400, 0));
	titleText->transform.SetScale(Vector3(10, 2, 0));

	titleText->Create(
		context,
		L"萬徳倫功",
		L"Assets/Fonts/Hangyaku.ttf",
		100.0f,
		D2D1::ColorF(D2D1::ColorF::Tomato),
		1024, 256);


	// ---------------------------------------------------------
	// 1. タイトルロゴ
	// ---------------------------------------------------------
	//titleLogo = AddUI<sf::ui::TextImage>();
	//titleLogo->transform.SetPosition(Vector3(0, 250, 0)); // 上の方に配置
	//titleLogo->transform.SetScale(Vector3(15, 5, 0));     // 大きく表示

	//titleLogo->Create(
	//	context,
	//	L"COSMIC DRIVE",         // タイトル文字列
	//	L"851ゴチカクット",               // フォント名
	//	120.0f,                   // フォントサイズ
	//	D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 色
	//	1500, 256               // テクスチャサイズ
	//);

	// ---------------------------------------------------------
	// 2. プレイボタン
	// ---------------------------------------------------------
	playButton = AddUI<sf::ui::TextImage>();
	playButton->transform.SetPosition(Vector3(0, -100, 0)); // 中央
	playButton->transform.SetScale(Vector3(4, 1.5f, 0));

	playButton->Create(
		context,
		L"PLAY",
		L"Assets/Fonts/ゴチカクット.ttf",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// ---------------------------------------------------------
	// 3. EXITボタン (EDITから変更)
	// ---------------------------------------------------------
	exitButton = AddUI<sf::ui::TextImage>();
	exitButton->transform.SetPosition(Vector3(0, -300, 0)); // 下の方
	exitButton->transform.SetScale(Vector3(4, 1.5f, 0));

	exitButton->Create(
		context,
		L"EXIT",
		L"Assets/Fonts/ゴチカクット.ttf",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// ---------------------------------------------------------
	// 初期設定
	// ---------------------------------------------------------
	selectedButton = 1; // 初期選択はPLAYにしておく
	UpdateButtonSelection();

	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void TitleCanvas::Update(const sf::command::ICommand& command)
{
	animationTimer += sf::Time::DeltaTime();
	float dt = sf::Time::DeltaTime();

	if (totalWidth <= 0.0f) return;

	// 境界線（画面端より少し外側）
	float wrapLimit = totalWidth * 0.5f;

	// --- 上段：左から右へ (L -> R) ---
	for (auto& sj : scrollingJacketsTop) {
		sj.posX += jacketSpeedTop * dt;
		// 右に突き抜けたら左へ
		if (sj.posX > wrapLimit) {
			sj.posX -= totalWidth;
		}
		// Y=0.0の位置に表示 (元のレイアウトに復帰)
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, 0.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
	}

	// --- 下段：右から左へ (R -> L) ---
	for (auto& sj : scrollingJacketsBottom) {
		sj.posX += jacketSpeedBottom * dt; // 負の値なので左へ進む

		if (sj.posX < -wrapLimit) {
			sj.posX += totalWidth;
		}

		// Y=-250の位置に表示
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, -250.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.7f, 0.7f, 0.7f, 1.0f });
	}

	HandleInput(command);
	UpdateButtonSelection();
}

void TitleCanvas::HandleInput(const sf::command::ICommand& command)
{
	// --- マウス操作 ---
	Vector2 mousePos = GetMousePosition();
	bool isExitHovered = IsButtonHovered(mousePos, exitButton);
	bool isPlayHovered = IsButtonHovered(mousePos, playButton);

	// マウスが乗ったら選択状態を切り替える
	if (isExitHovered) {
		selectedButton = 0;
	}
	else if (isPlayHovered) {
		selectedButton = 1;
	}

	// 左クリックで決定
	if (SInput::Instance().GetMouseDown(0)) {
		if (isExitHovered || isPlayHovered) {
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
	float sine = std::sin(animationTimer * 7.0f);
	float scaleOffset = 0.1f * sine;
	float alphaAnim = 0.5f + 0.5f * ((sine + 1.0f) * 0.5f);

	// 基本サイズ
	Vector3 scaleNormal(4.0f, 1.5f, 0);       // 非選択時
	Vector3 scaleSelectedBase(4.5f, 1.7f, 0); // 選択時の基準

	Vector3 scaleSelectedAnim = Vector3(
		scaleSelectedBase.x + scaleOffset,
		scaleSelectedBase.y + scaleOffset * 0.4f,
		0
	);

	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, alphaAnim);
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f);

	if (selectedButton == 0) {
		exitButton->transform.SetScale(scaleSelectedAnim);
		exitButton->material.SetColor(colorSelected);
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);
	}
	else {
		playButton->transform.SetScale(scaleSelectedAnim);
		playButton->material.SetColor(colorSelected);
		exitButton->transform.SetScale(scaleNormal);
		exitButton->material.SetColor(colorNormal);
	}
}

void TitleCanvas::OnButtonPressed()
{
	if (selectedButton == 0) {
		// GetMain() を経由してインスタンスを取得し、Exit() を呼ぶ
		app::Application* mainApp = app::Application::GetMain();
		if (mainApp) {
			mainApp->Exit();
		}
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

	float uiX = static_cast<float>(mousePoint.x) - screenWidth * 0.5f;
	float uiY = screenHeight * 0.5f - static_cast<float>(mousePoint.y);

	return Vector2(uiX, uiY);
}

void TitleCanvas::ShowSongSelectScene()
{
	if (!sceneChanger.isNull()) {
		LoadingScene::SetLoadingType(LoadingType::Common);
		sceneChanger->ChangeScene(SelectScene::StandbyScene());
	}
}