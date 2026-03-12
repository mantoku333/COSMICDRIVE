#include "TitleCanvas.h"
#include "TitleScene.h"  // MVP: Presenterに委譲
#include "sf/AppModel.h"
#include "Rendering/D3D11/CubismRenderer_D3D11.hpp"

// 依存ヘッダ
#include "DirectX11.h"
#include "SInput.h"          // 入力取得用
#include "DWriteContext.h"   // フォント描画用
#include <filesystem>
#include "App.h"
#include "StringUtils.h"     // 文字コード変換ユーティリティ

using namespace app::test;
using namespace sf;

// ユーティリティを利用
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

	// 条件分岐
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

	// ジャケット表示を更新
	jacketTextures.resize(jacketPaths.size());
	for (size_t i = 0; i < jacketPaths.size(); ++i) {
		jacketTextures[i].LoadTextureFromFile(jacketPaths[i]);
	}
	if (jacketTextures.empty()) return;

	// ジャケット表示を更新
	int minImages = static_cast<int>(2400.0f / jacketInterval) + 2;
	int numUI = std::max(static_cast<int>(jacketTextures.size()), minImages);
	totalWidth = numUI * jacketInterval;

	for (int i = 0; i < numUI; ++i) {
		sf::Texture* tex = &jacketTextures[i % jacketTextures.size()];
		float startX = (-totalWidth * 0.5f) + (i * jacketInterval);

		// UI要素を生成
		auto* imgTop = AddUI<sf::ui::Image>();
		imgTop->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgTop->material.texture = tex;
		scrollingJacketsTop.push_back({ imgTop, startX });

		// UI要素を生成
		auto* imgBottom = AddUI<sf::ui::Image>();
		imgBottom->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgBottom->material.texture = tex;
		scrollingJacketsBottom.push_back({ imgBottom, startX });
	}
}

void TitleCanvas::Begin()
{
	// Live2D関連処理
	Live2DManager::GetInstance()->Initialize(); 

	// 処理本体
	sf::ui::Canvas::Begin();

	// ジャケット表示を更新
	InitializeJacketFlow();

	// 処理本体
	auto* dx11 = sf::dx::DirectX11::Instance();
	ID3D11Device* device = dx11->GetMainDevice().GetDevice();
	ID3D11DeviceContext* context = dx11->GetMainDevice().GetContext();

	// 名前空間定義
	// Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
	// Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device);

	// 名前空間定義
	// _hiyoriModel = new AppModel();
	// -------------------- 確認用コード開始 --------------------
	namespace fs = std::filesystem;

	// Live2D関連処理
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
		// Live2D関連処理
		OutputDebugStringA("[Error] Failed to initialize Live2D model: renderer is nullptr.\n");
	}
	*/

	// UI要素を生成

	// ---------------------------------------------------------
	// UI要素を生成
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
	// 補助処理
	// titleLogo->transform.SetScale(Vector3(15, 5, 0)); // 大きく表示

	//titleLogo->Create(
	//	context,
	// 補助処理
	// 補助処理
	// 120.0f, // フォントサイズ
	//	D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 濶ｲ
	// UI要素を生成
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

	// 条件分岐

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
	// 処理本体
	// ---------------------------------------------------------
	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void TitleCanvas::Update(const sf::command::ICommand& command)
{
	animationTimer += sf::Time::DeltaTime();
	float dt = sf::Time::DeltaTime();

	// 条件分岐
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}
	
	// ジャケット表示を更新
	UpdateJacketScrolling(dt);
	
	// 処理本体
	DetectInputAndNotify(command);
	
	// 条件分岐
	if (presenter) {
		UpdateButtonHighlight(presenter->GetSelectedButton());
	}
}

// =================================================================
// ジャケット表示を更新
// =================================================================
void TitleCanvas::UpdateJacketScrolling(float dt)
{
	if (totalWidth <= 0.0f) return;

	// 処理本体
	float wrapLimit = totalWidth * 0.5f;

	// ループ処理
	for (auto& sj : scrollingJacketsTop) {
		sj.posX += jacketSpeedTop * dt;
		if (sj.posX > wrapLimit) {
			sj.posX -= totalWidth;
		}
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, 0.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
	}

	// ループ処理
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
// 処理本体
// =================================================================
void TitleCanvas::DetectInputAndNotify(const sf::command::ICommand& command)
{
	if (!presenter) return;
	
	// --- マウス操作 ---
	Vector2 mousePos = GetMousePosition();
	bool isExitHovered = IsButtonHovered(mousePos, exitButton);
	bool isPlayHovered = IsButtonHovered(mousePos, playButton);
	bool isConfigHovered = IsButtonHovered(mousePos, configButton);

	// マウスが乗ったらPresenterに通知
	if (isConfigHovered) {
		presenter->OnSelectButton(TitleButton::Config);
	}
	else if (isPlayHovered) {
		presenter->OnSelectButton(TitleButton::Play);
	}
	else if (isExitHovered) {
		presenter->OnSelectButton(TitleButton::Exit);
	}

	// 条件分岐
	if (SInput::Instance().GetMouseDown(0)) {
		if (isExitHovered || isPlayHovered || isConfigHovered) {
			presenter->OnConfirm();
		}
	}

	// Keyboard input moved to TitleScene::Update
}

// =================================================================
// MVP: ボタンハイライト表示更新
// =================================================================
void TitleCanvas::UpdateButtonHighlight(TitleButton selected)
{
	float sine = std::sin(animationTimer * 7.0f);
	float scaleOffset = 0.1f * sine;
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}

	float alphaAnim = 0.5f + 0.5f * ((sine + 1.0f) * 0.5f);

	// 基本サイズ
	Vector3 scaleNormal(4.0f, 1.5f, 0);       // 髱樣∈謚樊凾
	Vector3 scaleSelectedBase(4.5f, 1.7f, 0);

	Vector3 scaleSelectedAnim = Vector3(
		scaleSelectedBase.x + scaleOffset,
		scaleSelectedBase.y + scaleOffset * 0.4f,
		0
	);

	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, alphaAnim);
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f);

	// 処理本体
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

// 処理本体

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

// 処理本体


TitleCanvas::~TitleCanvas()
{
	if (_hiyoriModel) {
		delete _hiyoriModel;
		_hiyoriModel = nullptr;
	}
}
void TitleCanvas::Draw()
{
	// 処理本体
	// 処理本体
	sf::ui::Canvas::Draw();

	// // 2. Live2Dの描画
	//if (_hiyoriModel)
	//{
	//	auto* dx11 = sf::dx::DirectX11::Instance();
	//	auto device = dx11->GetMainDevice().GetDevice();
	//	auto context = dx11->GetMainDevice().GetContext();

	//	// =========================================================
	// 補助処理
	// 補助処理
	// 補助処理
	//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	// =========================================================

	//	// =========================================================
	// 補助処理
	// 補助処理
	//	D3D11_BLEND_DESC blendDesc = {};
	//	blendDesc.AlphaToCoverageEnable = FALSE;
	//	blendDesc.IndependentBlendEnable = FALSE;
	// 補助処理
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
	// 補助処理
	//	}
	//	// =========================================================

	//	// =========================================================
	// 補助処理
	//	D3D11_RASTERIZER_DESC rasterDesc = {};
	//	rasterDesc.FillMode = D3D11_FILL_SOLID;
	// 補助処理
	//	rasterDesc.FrontCounterClockwise = FALSE;
	//	rasterDesc.DepthClipEnable = TRUE;

	//	ID3D11RasterizerState* rasterState = nullptr;
	//	if (SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &rasterState)))
	//	{
	//		context->RSSetState(rasterState);
	//		rasterState->Release();
	//	}
	//	// =========================================================

	// 補助処理
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

	// 補助処理
	//	context->GSSetShader(nullptr, nullptr, 0);
	//	context->HSSetShader(nullptr, nullptr, 0);
	//	context->DSSetShader(nullptr, nullptr, 0);

	//	// ---------------------------------------------------------
	// 補助処理
	//	// ---------------------------------------------------------
 // 補助処理
 //       UINT numViewports = 1;
 //       D3D11_VIEWPORT vp;
 //       context->RSGetViewports(&numViewports, &vp);
 //       
 //       Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

 //       // ---------------------------------------------------------
 // 補助処理
 // 補助処理
 // 補助処理
 // 補助処理
 //       // ---------------------------------------------------------
 //       
 // 補助処理
 //       context->RSSetScissorRects(0, nullptr);

	//	auto renderer = _hiyoriModel->GetMyRenderer();
	//	if (!renderer) {
	//		OutputDebugStringA("Renderer is null in Draw!\n");
	//	}
	//	else {
 // 補助処理
 //           renderer->SetDefaultRenderState();
 //           
 // 補助処理
 // 補助処理
 //           ID3D11RenderTargetView* rtv = nullptr;
 //           ID3D11DepthStencilView* dsv = nullptr;
 //           context->OMGetRenderTargets(1, &rtv, &dsv);
 //           
 //           if (dsv) {
 // 補助処理
 //               context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
 // 補助処理
 //           }
 //           if (rtv) {
 // 補助処理
 //           }
 //           
 // 補助処理
 // 補助処理
 //           
	// 補助処理
	//		Csm::CubismMatrix44 matrix;
	//		matrix.LoadIdentity();

 // 補助処理
 //           float modelCanvasW = _hiyoriModel->GetModel()->GetCanvasWidth();
 //           float modelCanvasH = _hiyoriModel->GetModel()->GetCanvasHeight();
 //           
 //           if (modelCanvasH == 0) modelCanvasH = 2000.0f; // fallback

 // 補助処理
 // 補助処理
 //           float aspect = vp.Width / vp.Height;
 //           
 // 補助処理
 //           matrix.LoadIdentity();
 //           
 // 補助処理
 //           float scale = 1.6f / modelCanvasH; 
 //           matrix.Scale(scale / aspect, scale); 
 // 補助処理
 //            matrix.Translate(0.0f, -0.5f); 

	//		_hiyoriModel->Draw(device, context, matrix);
	//	}

	//	Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);
	//}
}
