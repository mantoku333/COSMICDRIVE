#include "TitleCanvas.h"
#include "TitleScene.h"
#include "sf/AppModel.h"
#include "Rendering/D3D11/CubismRenderer_D3D11.hpp"

#include "DirectX11.h"
#include "SInput.h"
#include "DWriteContext.h"
#include <filesystem>
#include "App.h"
#include "StringUtils.h"

using namespace app::test;
using namespace sf;

using sf::util::Utf8ToWstring;
using sf::util::Utf8ToShiftJis;

/// ジャケットスクロール表示の初期化
/// Songsフォルダからジャケット画像を読み込み、上段・下段の2列スクロールを構築する
void TitleCanvas::InitializeJacketFlow() {
	namespace fs = std::filesystem;
	std::string rootPath = "Songs";

	std::vector<std::string> jacketPaths;

	// Songsフォルダ以下のジャケット画像を収集
	if (fs::exists(rootPath)) {
		for (const auto& file : fs::recursive_directory_iterator(rootPath)) {
			{
                if (!file.is_regular_file()) continue;

				std::string ext = file.path().extension().string();
				if (ext == ".png" || ext == ".jpg") {
					jacketPaths.push_back(file.path().string());
				}
			}
		}
	}

	// ジャケットテクスチャを読み込み
	jacketTextures.resize(jacketPaths.size());
	for (size_t i = 0; i < jacketPaths.size(); ++i) {
		jacketTextures[i].LoadTextureFromFile(jacketPaths[i]);
	}
	if (jacketTextures.empty()) return;

	// 画面を埋めるために必要な最小数を計算
	int minImages = static_cast<int>(2400.0f / jacketInterval) + 2;
	int numUI = std::max(static_cast<int>(jacketTextures.size()), minImages);
	totalWidth = numUI * jacketInterval;

	// 上段・下段のスクロールジャケットを生成
	for (int i = 0; i < numUI; ++i) {
		sf::Texture* tex = &jacketTextures[i % jacketTextures.size()];
		float startX = (-totalWidth * 0.5f) + (i * jacketInterval);

		// 上段ジャケット
		auto* imgTop = AddUI<sf::ui::Image>();
		imgTop->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgTop->material.texture = tex;
		scrollingJacketsTop.push_back({ imgTop, startX });

		// 下段ジャケット
		auto* imgBottom = AddUI<sf::ui::Image>();
		imgBottom->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgBottom->material.texture = tex;
		scrollingJacketsBottom.push_back({ imgBottom, startX });
	}
}

/// 初期化処理 - Live2D初期化、ジャケットフロー構築、UI要素の生成
void TitleCanvas::Begin()
{
	// Live2Dマネージャーの初期化
	Live2DManager::GetInstance()->Initialize(); 

	// キャンバスの基底初期化
	sf::ui::Canvas::Begin();

	// ジャケットスクロールの初期化
	InitializeJacketFlow();

	// DirectX11デバイスの取得
	auto* dx11 = sf::dx::DirectX11::Instance();
	ID3D11Device* device = dx11->GetMainDevice().GetDevice();
	ID3D11DeviceContext* context = dx11->GetMainDevice().GetContext();

	// Live2Dモデルのパス確認（現在はコメントアウト中）
	namespace fs = std::filesystem;
	std::string dir = "Assets/Live2D/Hiyori/";
	std::string name = "Hiyori.model3.json";
	std::string fullPath = dir + name;

	// ===== UI要素の生成 =====

	// 開発版ラベル
	auto titleText = AddUI<sf::ui::TextImage>();
	titleText->transform.SetPosition(Vector3(-650, -450, 0));
	titleText->transform.SetScale(Vector3(10, 2, 0));

	titleText->Create(
		device,
		L"dev",
		L"Assets/Fonts/Hangyaku.ttf",
		100.0f,
		D2D1::ColorF(D2D1::ColorF::Tomato),
		1024, 256);

	// PLAYボタン（中央）
	playButton = AddUI<sf::ui::TextImage>();
	playButton->transform.SetPosition(Vector3(0, -300, 0)); 
	playButton->transform.SetScale(Vector3(4, 1.5f, 0));

	playButton->Create(
		device,
		L"PLAY",
		L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// EXITボタン（右）
	exitButton = AddUI<sf::ui::TextImage>();
	exitButton->transform.SetPosition(Vector3(500, -300, 0)); 
	exitButton->transform.SetScale(Vector3(4, 1.5f, 0));

	exitButton->Create(
		device,
		L"EXIT",
		L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		150.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// CONFIGボタン（左）
	configButton = AddUI<sf::ui::TextImage>();
	configButton->transform.SetPosition(Vector3(-500, -300, 0));
	configButton->transform.SetScale(Vector3(4, 1.5f, 0));

	configButton->Create(
		device,
		L"CONFIG",
		L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		130.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		600, 128
	);

	// 背景画像の読み込みと配置
	if (backTexture.LoadTextureFromFile("Assets/Texture/BACK.png")) {
		whiteBacking = AddUI<sf::ui::Image>();
		whiteBacking->transform.SetPosition(Vector3(-13, 10.0f, 5.0f)); 
		whiteBacking->transform.SetScale(Vector3(4.9f, 2.5f, 0));       
		whiteBacking->material.texture = &backTexture;
		whiteBacking->material.SetColor({ 1, 1, 1, 1 }); 
	}
	else {
		OutputDebugStringA("TitleCanvas: FAILED to load Assets/Texture/BACK.png\n");
	}

	// 更新コマンドをバインド
	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

/// 毎フレーム更新処理
void TitleCanvas::Update(const sf::command::ICommand& command)
{
	animationTimer += sf::Time::DeltaTime();
	float dt = sf::Time::DeltaTime();

	// Live2Dモデルの更新
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}
	
	// ジャケットスクロールの更新
	UpdateJacketScrolling(dt);
	
	// 入力検知とPresenterへの通知
	DetectInputAndNotify(command);
	
	// ボタンハイライトの更新
	if (presenter) {
		UpdateButtonHighlight(presenter->GetSelectedButton());
	}
}

// =================================================================
// ジャケットスクロール更新
// =================================================================

/// ジャケットを上段・下段で逆方向にスクロールさせる
void TitleCanvas::UpdateJacketScrolling(float dt)
{
	if (totalWidth <= 0.0f) return;

	// ラップ境界の計算
	float wrapLimit = totalWidth * 0.5f;

	// 上段：右方向にスクロール
	for (auto& sj : scrollingJacketsTop) {
		sj.posX += jacketSpeedTop * dt;
		if (sj.posX > wrapLimit) {
			sj.posX -= totalWidth;
		}
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, 0.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
	}

	// 下段：左方向にスクロール
	for (auto& sj : scrollingJacketsBottom) {
		sj.posX += jacketSpeedBottom * dt;
		if (sj.posX < -wrapLimit) {
			sj.posX += totalWidth;
		}
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, -250.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.7f, 0.7f, 0.7f, 1.0f });
	}
}

// =================================================================
// 入力検知
// =================================================================

/// マウス・キーボードの入力を検知し、Presenterに委譲する
void TitleCanvas::DetectInputAndNotify(const sf::command::ICommand& command)
{
	if (!presenter) return;
	
	// マウスホバー判定
	Vector2 mousePos = GetMousePosition();
	bool isExitHovered = IsButtonHovered(mousePos, exitButton);
	bool isPlayHovered = IsButtonHovered(mousePos, playButton);
	bool isConfigHovered = IsButtonHovered(mousePos, configButton);

	// ホバー中のボタンをPresenterに通知
	if (isConfigHovered) {
		presenter->OnSelectButton(TitleButton::Config);
	}
	else if (isPlayHovered) {
		presenter->OnSelectButton(TitleButton::Play);
	}
	else if (isExitHovered) {
		presenter->OnSelectButton(TitleButton::Exit);
	}

	// マウスクリック時の確定処理
	if (SInput::Instance().GetMouseDown(0)) {
		if (isExitHovered || isPlayHovered || isConfigHovered) {
			presenter->OnConfirm();
		}
	}

	// キーボード入力はTitleScene::Updateで処理
}

// =================================================================
// ボタンハイライト表示更新
// =================================================================

/// 選択中のボタンを拡大・点滅させ、非選択ボタンをグレーアウトする
void TitleCanvas::UpdateButtonHighlight(TitleButton selected)
{
	float sine = std::sin(animationTimer * 7.0f);
	float scaleOffset = 0.1f * sine;
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}

	float alphaAnim = 0.5f + 0.5f * ((sine + 1.0f) * 0.5f);

	// 基本サイズ
	Vector3 scaleNormal(4.0f, 1.5f, 0);       // 非選択時
	Vector3 scaleSelectedBase(4.5f, 1.7f, 0);  // 選択時

	Vector3 scaleSelectedAnim = Vector3(
		scaleSelectedBase.x + scaleOffset,
		scaleSelectedBase.y + scaleOffset * 0.4f,
		0
	);

	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, alphaAnim);
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f);

	// 選択状態に応じてスケールと色を切り替え
	switch (selected) {
	case TitleButton::Config:
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);
		configButton->transform.SetScale(scaleSelectedAnim);
		configButton->material.SetColor(colorSelected);
		exitButton->transform.SetScale(scaleNormal);
		exitButton->material.SetColor(colorNormal);
		break;
	case TitleButton::Play:
		playButton->transform.SetScale(scaleSelectedAnim);
		playButton->material.SetColor(colorSelected);
		configButton->transform.SetScale(scaleNormal);
		configButton->material.SetColor(colorNormal);
		exitButton->transform.SetScale(scaleNormal);
		exitButton->material.SetColor(colorNormal);
		break;
	case TitleButton::Exit:
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);
		configButton->transform.SetScale(scaleNormal);
		configButton->material.SetColor(colorNormal);
		exitButton->transform.SetScale(scaleSelectedAnim);
		exitButton->material.SetColor(colorSelected);
		break;
	}
}

/// ボタンのホバー判定（矩形当たり判定）
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

/// マウス座標をUI座標系（画面中央原点）に変換する
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

/// デストラクタ - Live2Dモデルを解放する
TitleCanvas::~TitleCanvas()
{
	if (_hiyoriModel) {
		delete _hiyoriModel;
		_hiyoriModel = nullptr;
	}
}

/// 描画処理 - キャンバスの描画を実行する
void TitleCanvas::Draw()
{
	sf::ui::Canvas::Draw();

	// Live2D描画は現在無効化中（Live2DComponentに移行済み）
}
