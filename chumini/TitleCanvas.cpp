#include "TitleCanvas.h"
#include "AppModel.h"
#include "SceneChangeComponent.h"
#include "SelectScene.h"
#include "EditScene.h"
#include "LoadingScene.h"
#include "ConfigScene.h"
#include "Rendering/D3D11/CubismRenderer_D3D11.hpp"

// 必要なインクルード
#include "DirectX11.h"       // デバイス取得用
#include "SInput.h"          // 入力取得用
#include "DWriteContext.h"   // フォント描画用
#include <filesystem>
#include "App.h"
#include "StringUtils.h"     // 文字コード変換ユーティリティ

using namespace app::test;
using namespace sf;

// sf::util の関数を使用（ローカル関数は削除）
using sf::util::Utf8ToWstring;
using sf::util::Utf8ToShiftJis;

void TitleCanvas::InitializeJacketFlow() {
	namespace fs = std::filesystem;
	std::string rootPath = "Songs";
		// Live2D Setup
	// Live2D Setup
	// Note: StartUp call is in Begin(), so we should not create AppModel here if it's called before Begin logic.
	// But actually InitializeJacketFlow is called FROM Begin. 
	// We will move AppModel creation to Begin to ensure it happens AFTER Live2DManager::Initialize.
	/* 
	if (!_hiyoriModel) {
		_hiyoriModel = new AppModel();
		_hiyoriModel->LoadAssets(device, "Assets/Live2D/Hiyori", "Hiyori.model3.json");
	}
	*/

	std::vector<std::string> jacketPaths;

	// 1. フォルダ走査（既存ロジック）
	if (fs::exists(rootPath)) {
		for (const auto& file : fs::recursive_directory_iterator(rootPath)) {
			{
				// Removed outer loop
                if (!file.is_regular_file()) continue;

				std::string ext = file.path().extension().string();
				if (ext == ".png" || ext == ".jpg") {
					jacketPaths.push_back(file.path().string());
					// break; // Removed to allow finding all jackets
				}
			}
		}
	}

	// 2. テクスチャロード
	jacketTextures.resize(jacketPaths.size());
	for (size_t i = 0; i < jacketPaths.size(); ++i) {
		jacketTextures[i].LoadTextureFromFile(jacketPaths[i]);
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
	// 1. Live2D全体の初期化 (最優先)
	Live2DManager::GetInstance()->Initialize(); 

	// 2. 基底クラスのBeginを呼び出す
	sf::ui::Canvas::Begin();

	// 3. 背景ジャケット初期化 (Live2Dモデル生成ロジックはここから削除済み)
	InitializeJacketFlow();

	// DirectXデバイスの取得
	auto* dx11 = sf::dx::DirectX11::Instance();
	ID3D11Device* device = dx11->GetMainDevice().GetDevice();
	ID3D11DeviceContext* context = dx11->GetMainDevice().GetContext();

	Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
	Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device); // ★重要: シェーダー生成

	// 4. Live2Dモデルの生成 (必ずInitializeの後に)
	// _hiyoriModel = new AppModel();
	// -------------------- 確認用コード 開始 --------------------
	namespace fs = std::filesystem;

	// あなたが LoadAssets に渡しているパス
	std::string dir = "Assets/Live2D/Hiyori/";
	std::string name = "Hiyori.model3.json";
	std::string fullPath = dir + name;

	// ... debug logs ...

	// _hiyoriModel->LoadAssets(device, dir.c_str(), name.c_str());

	/*
	auto renderer = _hiyoriModel->GetMyRenderer();

	if (renderer != nullptr) {
		renderer->Initialize(_hiyoriModel->GetModel());
	}
	else {
		// 読み込み失敗の可能性大
		OutputDebugStringA("【エラー】モデルの読み込みに失敗しました。renderer が nullptr です。\n");
	}
	*/

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
	titleText->transform.SetPosition(Vector3(-650, -450, 0));
	titleText->transform.SetScale(Vector3(10, 2, 0));

	titleText->Create(
		device,
		L"dev",
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
	// Position: Center (0)
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

	// ---------------------------------------------------------
	// 3. EXITボタン
	// ---------------------------------------------------------
	exitButton = AddUI<sf::ui::TextImage>();
	// Position: Right side, height -300
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

	// ---------------------------------------------------------
	// 4. CONFIGボタン
	// ---------------------------------------------------------
	configButton = AddUI<sf::ui::TextImage>();
	// Position: Left (-500)
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

	// ... (Backing code omitted/unchanged) ...

// ... inside HandleInput ...

	// --- キーボード操作 ---
	// 0: CONFIG, 1: PLAY, 2: EXIT
	
	if (SInput::Instance().GetKeyDown(Key::KEY_LEFT) || SInput::Instance().GetKeyDown(Key::KEY_A)) {
		selectedButton = (selectedButton - 1 + 3) % 3;
	}
	else if (SInput::Instance().GetKeyDown(Key::KEY_RIGHT) || SInput::Instance().GetKeyDown(Key::KEY_D)) {
		selectedButton = (selectedButton + 1) % 3;
	}

	// ---------------------------------------------------------
	// 4. Backing Image (Using Assets/Texture/BACK.png)
	// ---------------------------------------------------------
	// Load the texture provided by user
	if (backTexture.LoadTextureFromFile("Assets/Texture/BACK.png")) {
		OutputDebugStringA("TitleCanvas: Loaded BACK.png successfully.\n");
		whiteBacking = AddUI<sf::ui::Image>();
		
		// Position: Lowered from 250 -> 200 based on user feedback
		whiteBacking->transform.SetPosition(Vector3(-13, 10.0f, 5.0f)); 
		
		// Scale: Increased from 3.0 -> 4.5 based on "Motto Ookiku"
		whiteBacking->transform.SetScale(Vector3(4.9f, 2.5f, 0));       
		
		whiteBacking->material.texture = &backTexture;
		whiteBacking->material.SetColor({ 1, 1, 1, 1 }); 
	}
	else {
		OutputDebugStringA("TitleCanvas: FAILED to load Assets/Texture/BACK.png\n");
	}


	// ---------------------------------------------------------
	// 初期設定
	// ---------------------------------------------------------
	selectedButton = 1; // 初期選択はPLAY (1)
	UpdateButtonSelection();

	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void TitleCanvas::Update(const sf::command::ICommand& command)
{
	animationTimer += sf::Time::DeltaTime();
	float dt = sf::Time::DeltaTime();

	// ★追加: Live2Dモデルの更新
	if (_hiyoriModel) {
		_hiyoriModel->Update();
		OutputDebugStringA("Model Updated\n");
	}
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
	bool isConfigHovered = IsButtonHovered(mousePos, configButton);

	// マウスが乗ったら選択状態を切り替える (0: CONFIG, 1: PLAY, 2: EXIT)
	if (isConfigHovered) {
		selectedButton = 0;
	}
	else if (isPlayHovered) {
		selectedButton = 1;
	}
	else if (isExitHovered) {
		selectedButton = 2;
	}

	// 左クリックで決定
	if (SInput::Instance().GetMouseDown(0)) {
		if (isExitHovered || isPlayHovered || isConfigHovered) {
			OnButtonPressed();
		}
	}

	// --- キーボード操作 ---
	const int BUTTON_COUNT = 3;

	// 左右キーまたはADキーで選択切り替え
	// Left/A -> 戻る (Index--)
	if (SInput::Instance().GetKeyDown(Key::KEY_LEFT) || SInput::Instance().GetKeyDown(Key::KEY_A)) {
		selectedButton = (selectedButton - 1 + BUTTON_COUNT) % BUTTON_COUNT;
	}
	// Right/D -> 進む (Index++)
	else if (SInput::Instance().GetKeyDown(Key::KEY_RIGHT) || SInput::Instance().GetKeyDown(Key::KEY_D)) {
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
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}

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

	// 0: CONFIG, 1: PLAY, 2: EXIT
	if (selectedButton == 0) {
		// CONFIG selected
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);

		configButton->transform.SetScale(scaleSelectedAnim);
		configButton->material.SetColor(colorSelected);

		exitButton->transform.SetScale(scaleNormal);
		exitButton->material.SetColor(colorNormal);
	}
	else if (selectedButton == 1) {
		// PLAY selected
		playButton->transform.SetScale(scaleSelectedAnim);
		playButton->material.SetColor(colorSelected);

		configButton->transform.SetScale(scaleNormal);
		configButton->material.SetColor(colorNormal);

		exitButton->transform.SetScale(scaleNormal);
		exitButton->material.SetColor(colorNormal);
	}
	else {
		// EXIT selected
		playButton->transform.SetScale(scaleNormal);
		playButton->material.SetColor(colorNormal);

		configButton->transform.SetScale(scaleNormal);
		configButton->material.SetColor(colorNormal);

		exitButton->transform.SetScale(scaleSelectedAnim);
		exitButton->material.SetColor(colorSelected);
	}
}

void TitleCanvas::OnButtonPressed()
{
	if (selectedButton == 0) {
		// CONFIG
		if (!sceneChanger.isNull()) {
			LoadingScene::SetLoadingType(LoadingType::Common);
			sceneChanger->ChangeScene(ConfigScene::StandbyScene());
		}
	}
	else if (selectedButton == 1) {
		// PLAY
		ShowSongSelectScene();
	}
	else if (selectedButton == 2) {
		// EXIT
		app::Application* mainApp = app::Application::GetMain();
		if (mainApp) {
			mainApp->Exit();
		}
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

TitleCanvas::~TitleCanvas()
{
	if (_hiyoriModel) {
		delete _hiyoriModel;
		_hiyoriModel = nullptr;
	}
}
void TitleCanvas::Draw()
{
	// 1. 親クラス（UIなど）を描画
	// ここでGPUの設定がUI用（Live2Dには不向きな状態）書き換わってしまっています
	sf::ui::Canvas::Draw();

	//// 2. Live2Dの描画
	//if (_hiyoriModel)
	//{
	//	auto* dx11 = sf::dx::DirectX11::Instance();
	//	auto device = dx11->GetMainDevice().GetDevice();
	//	auto context = dx11->GetMainDevice().GetContext();

	//	// =========================================================
	//	// ★★★ 【対策1】トポロジーの強制リセット ★★★
	//	// 前の処理が「TriangleStrip」などを使っているとLive2Dは壊れます。
	//	// 必ず「TriangleList」に戻す必要があります。
	//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	// =========================================================

	//	// =========================================================
	//	// ★★★ 【対策2】ブレンドステート（透明処理）の強制有効化 ★★★
	//	// これがないと、透明なポリゴンが「見えない」か「黒く」なります。
	//	D3D11_BLEND_DESC blendDesc = {};
	//	blendDesc.AlphaToCoverageEnable = FALSE;
	//	blendDesc.IndependentBlendEnable = FALSE;
	//	blendDesc.RenderTarget[0].BlendEnable = TRUE;             // ブレンド有効
	//	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//	ID3D11BlendState* blendState = nullptr;
	//	if (SUCCEEDED(device->CreateBlendState(&blendDesc, &blendState)))
	//	{
	//		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//		context->OMSetBlendState(blendState, blendFactor, 0xffffffff);
	//		blendState->Release(); // セットしたら即解放してOK
	//	}
	//	// =========================================================

	//	// =========================================================
	//	// ★★★ 【対策3】カリング（裏面削除）なし ★★★
	//	D3D11_RASTERIZER_DESC rasterDesc = {};
	//	rasterDesc.FillMode = D3D11_FILL_SOLID;
	//	rasterDesc.CullMode = D3D11_CULL_NONE; // 両面描画
	//	rasterDesc.FrontCounterClockwise = FALSE;
	//	rasterDesc.DepthClipEnable = TRUE;

	//	ID3D11RasterizerState* rasterState = nullptr;
	//	if (SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &rasterState)))
	//	{
	//		context->RSSetState(rasterState);
	//		rasterState->Release();
	//	}
	//	// =========================================================

	//	// 【対策4】奥行き判定無効（手前に描画）
	//	// Note: passing nullptr to OMSetDepthStencilState resets to DEFAULT, which is Depth Enabled.
	//	// We must explicitly create a disabled state.
	//	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	//	depthDesc.DepthEnable = FALSE; // Disable depth test to draw on top
	//	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	//	depthDesc.StencilEnable = FALSE;

	//	ID3D11DepthStencilState* depthState = nullptr;
	//	if (SUCCEEDED(device->CreateDepthStencilState(&depthDesc, &depthState)))
	//	{
	//		context->OMSetDepthStencilState(depthState, 0);
	//		depthState->Release();
	//	}

	//	// 【対策5】余計なシェーダーをオフ
	//	context->GSSetShader(nullptr, nullptr, 0);
	//	context->HSSetShader(nullptr, nullptr, 0);
	//	context->DSSetShader(nullptr, nullptr, 0);

	//	// ---------------------------------------------------------
	//	// ここから描画実行
	//	// ---------------------------------------------------------
 //       // 現在のビューポートを取得してLive2Dに渡す
 //       UINT numViewports = 1;
 //       D3D11_VIEWPORT vp;
 //       context->RSGetViewports(&numViewports, &vp);
 //       
 //       Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

 //       // ---------------------------------------------------------
 //       // 【重要】レンダリングステートのクリーンアップ
 //       // UI描画によるシザー矩形（切り抜き）やステンシル設定が残っていると、
 //       // モデルが意図しない形でマスクされたり非表示になったりします。
 //       // ここで明示的にリセットします。
 //       // ---------------------------------------------------------
 //       
 //       // 1. シザー矩形の無効化 (全画面描画許可)
 //       context->RSSetScissorRects(0, nullptr);

	//	auto renderer = _hiyoriModel->GetMyRenderer();
	//	if (!renderer) {
	//		OutputDebugStringA("Renderer is null in Draw!\n");
	//	}
	//	else {
 //           // 2. Live2D標準のレンダーステート（ブレンド、深度、ステンシル等）を適用
 //           renderer->SetDefaultRenderState();
 //           
 //           // ★追加策: 深度・ステンシルバッファのクリア
 //           // 現在バインドされているDSVを取得してクリアします（API標準の方法）
 //           ID3D11RenderTargetView* rtv = nullptr;
 //           ID3D11DepthStencilView* dsv = nullptr;
 //           context->OMGetRenderTargets(1, &rtv, &dsv);
 //           
 //           if (dsv) {
 //               // UI描画で汚れたバッファをクリア
 //               context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
 //               dsv->Release(); // Getで参照カウントが増えているので解放
 //           }
 //           if (rtv) {
 //               rtv->Release(); // Getで参照カウントが増えているので解放
 //           }
 //           
 //           // ブレンドステートは SetDefaultRenderState (Normal時の設定) に任せるか、必要ならここで手動作成してセットする
 //           // renderer->SetBlendState は private なので呼べない
 //           
	//		// 行列の構築
	//		Csm::CubismMatrix44 matrix;
	//		matrix.LoadIdentity();

 //           // モデルのキャンバスサイズを取得 (通常は 2000x2000 程度)
 //           float modelCanvasW = _hiyoriModel->GetModel()->GetCanvasWidth();
 //           float modelCanvasH = _hiyoriModel->GetModel()->GetCanvasHeight();
 //           
 //           if (modelCanvasH == 0) modelCanvasH = 2000.0f; // fallback

 //           // 画面全体(-1 ~ 1 -> 幅2.0)にモデル全体(Width/Height)を収める
 //           // アスペクト比維持
 //           float aspect = vp.Width / vp.Height;
 //           
 //           // アスペクト補正を入れて、画面中央に表示してみる
 //           matrix.LoadIdentity();
 //           
 //           // 安全策: 少し小さめに表示 (1.5 / canvasH)
 //           float scale = 1.6f / modelCanvasH; 
 //           matrix.Scale(scale / aspect, scale); 
 //           // 縦位置調整 (少し下に下げる？)
 //            matrix.Translate(0.0f, -0.5f); 

	//		_hiyoriModel->Draw(device, context, matrix);
	//	}

	//	Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);
	//}
}