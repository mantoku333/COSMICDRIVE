#include "TitleCanvas.h"
#include "TitleScene.h"  // MVP: Presenter蜿ら・
#include "sf/AppModel.h"
#include "Rendering/D3D11/CubismRenderer_D3D11.hpp"

// 蠢・ｦ√↑繧､繝ｳ繧ｯ繝ｫ繝ｼ繝・
#include "DirectX11.h"       // 繝・ヰ繧､繧ｹ蜿門ｾ礼畑
#include "SInput.h"          // 蜈･蜉帛叙蠕礼畑
#include "DWriteContext.h"   // 繝輔か繝ｳ繝域緒逕ｻ逕ｨ
#include <filesystem>
#include "App.h"
#include "StringUtils.h"     // 譁・ｭ励さ繝ｼ繝牙､画鋤繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ

using namespace app::test;
using namespace sf;

// sf::util 縺ｮ髢｢謨ｰ繧剃ｽｿ逕ｨ・医Ο繝ｼ繧ｫ繝ｫ髢｢謨ｰ縺ｯ蜑企勁・・
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

	// 1. 繝輔か繝ｫ繝襍ｰ譟ｻ・域里蟄倥Ο繧ｸ繝・け・・
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

	// 2. 繝・け繧ｹ繝√Ε繝ｭ繝ｼ繝・
	jacketTextures.resize(jacketPaths.size());
	for (size_t i = 0; i < jacketPaths.size(); ++i) {
		jacketTextures[i].LoadTextureFromFile(jacketPaths[i]);
	}
	if (jacketTextures.empty()) return;

	// 3. 譫壽焚縺ｨ繝ｫ繝ｼ繝怜ｹ・・險育ｮ・
	int minImages = static_cast<int>(2400.0f / jacketInterval) + 2;
	int numUI = std::max(static_cast<int>(jacketTextures.size()), minImages);
	totalWidth = numUI * jacketInterval;

	for (int i = 0; i < numUI; ++i) {
		sf::Texture* tex = &jacketTextures[i % jacketTextures.size()];
		float startX = (-totalWidth * 0.5f) + (i * jacketInterval);

		// --- 荳頑ｮｵ縺ｮ逕滓・ ---
		auto* imgTop = AddUI<sf::ui::Image>();
		imgTop->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgTop->material.texture = tex;
		scrollingJacketsTop.push_back({ imgTop, startX });

		// --- 荳区ｮｵ縺ｮ逕滓・ ---
		auto* imgBottom = AddUI<sf::ui::Image>();
		imgBottom->transform.SetScale(Vector3(jacketScale, jacketScale, 1));
		imgBottom->material.texture = tex;
		scrollingJacketsBottom.push_back({ imgBottom, startX });
	}
}

void TitleCanvas::Begin()
{
	// 1. Live2D蜈ｨ菴薙・蛻晄悄蛹・(譛蜆ｪ蜈・
	Live2DManager::GetInstance()->Initialize(); 

	// 2. 蝓ｺ蠎輔け繝ｩ繧ｹ縺ｮBegin繧貞他縺ｳ蜃ｺ縺・
	sf::ui::Canvas::Begin();

	// 3. 閭梧勹繧ｸ繝｣繧ｱ繝・ヨ蛻晄悄蛹・(Live2D繝｢繝・Ν逕滓・繝ｭ繧ｸ繝・け縺ｯ縺薙％縺九ｉ蜑企勁貂医∩)
	InitializeJacketFlow();

	// DirectX繝・ヰ繧､繧ｹ縺ｮ蜿門ｾ・
	auto* dx11 = sf::dx::DirectX11::Instance();
	ID3D11Device* device = dx11->GetMainDevice().GetDevice();
	ID3D11DeviceContext* context = dx11->GetMainDevice().GetContext();

	// 笘・炎髯､: Live2D 繝ｬ繝ｳ繝繝ｩ繝ｼ蛻晄悄蛹悶・ Live2DComponent::LoadModel() 縺ｧ陦後ｏ繧後ｋ縺溘ａ縲√％縺薙〒縺ｮ驥崎､・他縺ｳ蜃ｺ縺励ｒ蜑企勁
	// Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::InitializeConstantSettings(1, device);
	// Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GenerateShader(device);

	// 4. Live2D繝｢繝・Ν縺ｮ逕滓・ (蠢・★Initialize縺ｮ蠕後↓)
	// _hiyoriModel = new AppModel();
	// -------------------- 遒ｺ隱咲畑繧ｳ繝ｼ繝・髢句ｧ・--------------------
	namespace fs = std::filesystem;

	// 縺ゅ↑縺溘′ LoadAssets 縺ｫ貂｡縺励※縺・ｋ繝代せ
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
		// 隱ｭ縺ｿ霎ｼ縺ｿ螟ｱ謨励・蜿ｯ閭ｽ諤ｧ螟ｧ
		OutputDebugStringA("縲舌お繝ｩ繝ｼ縲代Δ繝・Ν縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ縺ｫ螟ｱ謨励＠縺ｾ縺励◆縲Ｓenderer 縺・nullptr 縺ｧ縺吶・n");
	}
	*/

	// SceneChangeComponent縺ｸ縺ｮ蜿ら・縺ｯ蜑企勁・・VP: TitleScene縺ｧ邂｡逅・ｼ・

	// ---------------------------------------------------------
	// 蜷榊燕繝ｻ蟄ｦ譬｡蜷阪・陦ｨ遉ｺ
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
	// 1. 繧ｿ繧､繝医Ν繝ｭ繧ｴ
	// ---------------------------------------------------------
	//titleLogo = AddUI<sf::ui::TextImage>();
	//titleLogo->transform.SetPosition(Vector3(0, 250, 0)); // 荳翫・譁ｹ縺ｫ驟咲ｽｮ
	//titleLogo->transform.SetScale(Vector3(15, 5, 0));     // 螟ｧ縺阪￥陦ｨ遉ｺ

	//titleLogo->Create(
	//	context,
	//	L"COSMIC DRIVE",         // 繧ｿ繧､繝医Ν譁・ｭ怜・
	//	L"851繧ｴ繝√き繧ｯ繝・ヨ",               // 繝輔か繝ｳ繝亥錐
	//	120.0f,                   // 繝輔か繝ｳ繝医し繧､繧ｺ
	//	D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), // 濶ｲ
	//	1500, 256               // 繝・け繧ｹ繝√Ε繧ｵ繧､繧ｺ
	//);

	// ---------------------------------------------------------
	// 2. 繝励Ξ繧､繝懊ち繝ｳ
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
	// 3. EXIT繝懊ち繝ｳ
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
	// 4. CONFIG繝懊ち繝ｳ
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

	// 窶ｻ Begin蜀・・蜈･蜉帛・逅・・蜑企勁・・cene蛛ｴ縺ｧ邂｡逅・ｼ・

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
	// 蛻晄悄險ｭ螳夲ｼ・VP: 迥ｶ諷九・Scene蛛ｴ縺ｧ邂｡逅・ｼ・
	// ---------------------------------------------------------
	updateCommand.Bind(std::bind(&TitleCanvas::Update, this, std::placeholders::_1));
}

void TitleCanvas::Update(const sf::command::ICommand& command)
{
	animationTimer += sf::Time::DeltaTime();
	float dt = sf::Time::DeltaTime();

	// Live2D繝｢繝・Ν縺ｮ譖ｴ譁ｰ
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}
	
	// 繧ｸ繝｣繧ｱ繝・ヨ繧ｹ繧ｯ繝ｭ繝ｼ繝ｫ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ
	UpdateJacketScrolling(dt);
	
	// MVP: 蜈･蜉帶､懷・竊単resenter縺ｫ蟋碑ｭｲ
	DetectInputAndNotify(command);
	
	// MVP: Presenter縺ｮ迥ｶ諷九ｒ蜿ら・縺励※陦ｨ遉ｺ譖ｴ譁ｰ
	if (presenter) {
		UpdateButtonHighlight(presenter->GetSelectedButton());
	}
}

// =================================================================
// 繧ｸ繝｣繧ｱ繝・ヨ繧ｹ繧ｯ繝ｭ繝ｼ繝ｫ繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ
// =================================================================
void TitleCanvas::UpdateJacketScrolling(float dt)
{
	if (totalWidth <= 0.0f) return;

	// 蠅・阜邱夲ｼ育判髱｢遶ｯ繧医ｊ蟆代＠螟門・・・
	float wrapLimit = totalWidth * 0.5f;

	// --- 荳頑ｮｵ・壼ｷｦ縺九ｉ蜿ｳ縺ｸ (L -> R) ---
	for (auto& sj : scrollingJacketsTop) {
		sj.posX += jacketSpeedTop * dt;
		if (sj.posX > wrapLimit) {
			sj.posX -= totalWidth;
		}
		sj.uiImage->transform.SetPosition(Vector3(sj.posX, 0.0f, 10.0f));
		sj.uiImage->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });
	}

	// --- 荳区ｮｵ・壼承縺九ｉ蟾ｦ縺ｸ (R -> L) ---
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
// MVP: 蜈･蜉帶､懷・竊単resenter縺ｫ蟋碑ｭｲ
// =================================================================
void TitleCanvas::DetectInputAndNotify(const sf::command::ICommand& command)
{
	if (!presenter) return;
	
	// --- 繝槭え繧ｹ謫堺ｽ・---
	Vector2 mousePos = GetMousePosition();
	bool isExitHovered = IsButtonHovered(mousePos, exitButton);
	bool isPlayHovered = IsButtonHovered(mousePos, playButton);
	bool isConfigHovered = IsButtonHovered(mousePos, configButton);

	// 繝槭え繧ｹ縺御ｹ励▲縺溘ｉPresenter縺ｫ騾夂衍
	if (isConfigHovered) {
		presenter->OnSelectButton(TitleButton::Config);
	}
	else if (isPlayHovered) {
		presenter->OnSelectButton(TitleButton::Play);
	}
	else if (isExitHovered) {
		presenter->OnSelectButton(TitleButton::Exit);
	}

	// 蟾ｦ繧ｯ繝ｪ繝・け縺ｧ豎ｺ螳・
	if (SInput::Instance().GetMouseDown(0)) {
		if (isExitHovered || isPlayHovered || isConfigHovered) {
			presenter->OnConfirm();
		}
	}

	// Keyboard input moved to TitleScene::Update
}

// =================================================================
// MVP: 繝懊ち繝ｳ繝上う繝ｩ繧､繝郁｡ｨ遉ｺ譖ｴ譁ｰ
// =================================================================
void TitleCanvas::UpdateButtonHighlight(TitleButton selected)
{
	float sine = std::sin(animationTimer * 7.0f);
	float scaleOffset = 0.1f * sine;
	if (_hiyoriModel) {
		_hiyoriModel->Update();
	}

	float alphaAnim = 0.5f + 0.5f * ((sine + 1.0f) * 0.5f);

	// 蝓ｺ譛ｬ繧ｵ繧､繧ｺ
	Vector3 scaleNormal(4.0f, 1.5f, 0);       // 髱樣∈謚樊凾
	Vector3 scaleSelectedBase(4.5f, 1.7f, 0); // 驕ｸ謚樊凾縺ｮ蝓ｺ貅・

	Vector3 scaleSelectedAnim = Vector3(
		scaleSelectedBase.x + scaleOffset,
		scaleSelectedBase.y + scaleOffset * 0.4f,
		0
	);

	auto colorSelected = DirectX::XMFLOAT4(1.0f, 1.0f, 0.2f, alphaAnim);
	auto colorNormal = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.6f);

	// MVP: TitleButton enum繧剃ｽｿ逕ｨ
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

// OnButtonPressed縺ｯ蜑企勁・・VP: TitleScene::ExecuteButtonAction縺ｫ遘ｻ蜍包ｼ・

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

// OnButtonPressed 縺ｨ ShowSongSelectScene 縺ｯ蜑企勁・・VP: TitleScene縺ｫ遘ｻ蜍包ｼ・


TitleCanvas::~TitleCanvas()
{
	if (_hiyoriModel) {
		delete _hiyoriModel;
		_hiyoriModel = nullptr;
	}
}
void TitleCanvas::Draw()
{
	// 1. 隕ｪ繧ｯ繝ｩ繧ｹ・・I縺ｪ縺ｩ・峨ｒ謠冗判
	// 縺薙％縺ｧGPU縺ｮ險ｭ螳壹′UI逕ｨ・・ive2D縺ｫ縺ｯ荳榊髄縺阪↑迥ｶ諷具ｼ画嶌縺肴鋤繧上▲縺ｦ縺励∪縺｣縺ｦ縺・∪縺・
	sf::ui::Canvas::Draw();

	//// 2. Live2D縺ｮ謠冗判
	//if (_hiyoriModel)
	//{
	//	auto* dx11 = sf::dx::DirectX11::Instance();
	//	auto device = dx11->GetMainDevice().GetDevice();
	//	auto context = dx11->GetMainDevice().GetContext();

	//	// =========================================================
	//	// 笘・・笘・縲仙ｯｾ遲・縲代ヨ繝昴Ο繧ｸ繝ｼ縺ｮ蠑ｷ蛻ｶ繝ｪ繧ｻ繝・ヨ 笘・・笘・
	//	// 蜑阪・蜃ｦ逅・′縲卦riangleStrip縲阪↑縺ｩ繧剃ｽｿ縺｣縺ｦ縺・ｋ縺ｨLive2D縺ｯ螢翫ｌ縺ｾ縺吶・
	//	// 蠢・★縲卦riangleList縲阪↓謌ｻ縺吝ｿ・ｦ√′縺ゅｊ縺ｾ縺吶・
	//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	// =========================================================

	//	// =========================================================
	//	// 笘・・笘・縲仙ｯｾ遲・縲代ヶ繝ｬ繝ｳ繝峨せ繝・・繝茨ｼ磯乗・蜃ｦ逅・ｼ峨・蠑ｷ蛻ｶ譛牙柑蛹・笘・・笘・
	//	// 縺薙ｌ縺後↑縺・→縲・乗・縺ｪ繝昴Μ繧ｴ繝ｳ縺後瑚ｦ九∴縺ｪ縺・阪°縲碁ｻ偵￥縲阪↑繧翫∪縺吶・
	//	D3D11_BLEND_DESC blendDesc = {};
	//	blendDesc.AlphaToCoverageEnable = FALSE;
	//	blendDesc.IndependentBlendEnable = FALSE;
	//	blendDesc.RenderTarget[0].BlendEnable = TRUE;             // 繝悶Ξ繝ｳ繝画怏蜉ｹ
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
	//		blendState->Release(); // 繧ｻ繝・ヨ縺励◆繧牙叉隗｣謾ｾ縺励※OK
	//	}
	//	// =========================================================

	//	// =========================================================
	//	// 笘・・笘・縲仙ｯｾ遲・縲代き繝ｪ繝ｳ繧ｰ・郁｣城擇蜑企勁・峨↑縺・笘・・笘・
	//	D3D11_RASTERIZER_DESC rasterDesc = {};
	//	rasterDesc.FillMode = D3D11_FILL_SOLID;
	//	rasterDesc.CullMode = D3D11_CULL_NONE; // 荳｡髱｢謠冗判
	//	rasterDesc.FrontCounterClockwise = FALSE;
	//	rasterDesc.DepthClipEnable = TRUE;

	//	ID3D11RasterizerState* rasterState = nullptr;
	//	if (SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &rasterState)))
	//	{
	//		context->RSSetState(rasterState);
	//		rasterState->Release();
	//	}
	//	// =========================================================

	//	// 縲仙ｯｾ遲・縲大･･陦後″蛻､螳夂┌蜉ｹ・域焔蜑阪↓謠冗判・・
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

	//	// 縲仙ｯｾ遲・縲台ｽ呵ｨ医↑繧ｷ繧ｧ繝ｼ繝繝ｼ繧偵が繝・
	//	context->GSSetShader(nullptr, nullptr, 0);
	//	context->HSSetShader(nullptr, nullptr, 0);
	//	context->DSSetShader(nullptr, nullptr, 0);

	//	// ---------------------------------------------------------
	//	// 縺薙％縺九ｉ謠冗判螳溯｡・
	//	// ---------------------------------------------------------
 //       // 迴ｾ蝨ｨ縺ｮ繝薙Η繝ｼ繝昴・繝医ｒ蜿門ｾ励＠縺ｦLive2D縺ｫ貂｡縺・
 //       UINT numViewports = 1;
 //       D3D11_VIEWPORT vp;
 //       context->RSGetViewports(&numViewports, &vp);
 //       
 //       Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::StartFrame(device, context, (csmUint32)vp.Width, (csmUint32)vp.Height);

 //       // ---------------------------------------------------------
 //       // 縲宣㍾隕√代Ξ繝ｳ繝繝ｪ繝ｳ繧ｰ繧ｹ繝・・繝医・繧ｯ繝ｪ繝ｼ繝ｳ繧｢繝・・
 //       // UI謠冗判縺ｫ繧医ｋ繧ｷ繧ｶ繝ｼ遏ｩ蠖｢・亥・繧頑栢縺搾ｼ峨ｄ繧ｹ繝・Φ繧ｷ繝ｫ險ｭ螳壹′谿九▲縺ｦ縺・ｋ縺ｨ縲・
 //       // 繝｢繝・Ν縺梧э蝗ｳ縺励↑縺・ｽ｢縺ｧ繝槭せ繧ｯ縺輔ｌ縺溘ｊ髱櫁｡ｨ遉ｺ縺ｫ縺ｪ縺｣縺溘ｊ縺励∪縺吶・
 //       // 縺薙％縺ｧ譏守､ｺ逧・↓繝ｪ繧ｻ繝・ヨ縺励∪縺吶・
 //       // ---------------------------------------------------------
 //       
 //       // 1. 繧ｷ繧ｶ繝ｼ遏ｩ蠖｢縺ｮ辟｡蜉ｹ蛹・(蜈ｨ逕ｻ髱｢謠冗判險ｱ蜿ｯ)
 //       context->RSSetScissorRects(0, nullptr);

	//	auto renderer = _hiyoriModel->GetMyRenderer();
	//	if (!renderer) {
	//		OutputDebugStringA("Renderer is null in Draw!\n");
	//	}
	//	else {
 //           // 2. Live2D讓呎ｺ悶・繝ｬ繝ｳ繝繝ｼ繧ｹ繝・・繝茨ｼ医ヶ繝ｬ繝ｳ繝峨∵ｷｱ蠎ｦ縲√せ繝・Φ繧ｷ繝ｫ遲会ｼ峨ｒ驕ｩ逕ｨ
 //           renderer->SetDefaultRenderState();
 //           
 //           // 笘・ｿｽ蜉遲・ 豺ｱ蠎ｦ繝ｻ繧ｹ繝・Φ繧ｷ繝ｫ繝舌ャ繝輔ぃ縺ｮ繧ｯ繝ｪ繧｢
 //           // 迴ｾ蝨ｨ繝舌う繝ｳ繝峨＆繧後※縺・ｋDSV繧貞叙蠕励＠縺ｦ繧ｯ繝ｪ繧｢縺励∪縺呻ｼ・PI讓呎ｺ悶・譁ｹ豕包ｼ・
 //           ID3D11RenderTargetView* rtv = nullptr;
 //           ID3D11DepthStencilView* dsv = nullptr;
 //           context->OMGetRenderTargets(1, &rtv, &dsv);
 //           
 //           if (dsv) {
 //               // UI謠冗判縺ｧ豎壹ｌ縺溘ヰ繝・ヵ繧｡繧偵け繝ｪ繧｢
 //               context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
 //               dsv->Release(); // Get縺ｧ蜿ら・繧ｫ繧ｦ繝ｳ繝医′蠅励∴縺ｦ縺・ｋ縺ｮ縺ｧ隗｣謾ｾ
 //           }
 //           if (rtv) {
 //               rtv->Release(); // Get縺ｧ蜿ら・繧ｫ繧ｦ繝ｳ繝医′蠅励∴縺ｦ縺・ｋ縺ｮ縺ｧ隗｣謾ｾ
 //           }
 //           
 //           // 繝悶Ξ繝ｳ繝峨せ繝・・繝医・ SetDefaultRenderState (Normal譎ゅ・險ｭ螳・ 縺ｫ莉ｻ縺帙ｋ縺九∝ｿ・ｦ√↑繧峨％縺薙〒謇句虚菴懈・縺励※繧ｻ繝・ヨ縺吶ｋ
 //           // renderer->SetBlendState 縺ｯ private 縺ｪ縺ｮ縺ｧ蜻ｼ縺ｹ縺ｪ縺・
 //           
	//		// 陦悟・縺ｮ讒狗ｯ・
	//		Csm::CubismMatrix44 matrix;
	//		matrix.LoadIdentity();

 //           // 繝｢繝・Ν縺ｮ繧ｭ繝｣繝ｳ繝舌せ繧ｵ繧､繧ｺ繧貞叙蠕・(騾壼ｸｸ縺ｯ 2000x2000 遞句ｺｦ)
 //           float modelCanvasW = _hiyoriModel->GetModel()->GetCanvasWidth();
 //           float modelCanvasH = _hiyoriModel->GetModel()->GetCanvasHeight();
 //           
 //           if (modelCanvasH == 0) modelCanvasH = 2000.0f; // fallback

 //           // 逕ｻ髱｢蜈ｨ菴・-1 ~ 1 -> 蟷・.0)縺ｫ繝｢繝・Ν蜈ｨ菴・Width/Height)繧貞庶繧√ｋ
 //           // 繧｢繧ｹ繝壹け繝域ｯ皮ｶｭ謖・
 //           float aspect = vp.Width / vp.Height;
 //           
 //           // 繧｢繧ｹ繝壹け繝郁｣懈ｭ｣繧貞・繧後※縲∫判髱｢荳ｭ螟ｮ縺ｫ陦ｨ遉ｺ縺励※縺ｿ繧・
 //           matrix.LoadIdentity();
 //           
 //           // 螳牙・遲・ 蟆代＠蟆上＆繧√↓陦ｨ遉ｺ (1.5 / canvasH)
 //           float scale = 1.6f / modelCanvasH; 
 //           matrix.Scale(scale / aspect, scale); 
 //           // 邵ｦ菴咲ｽｮ隱ｿ謨ｴ (蟆代＠荳九↓荳九￡繧具ｼ・
 //            matrix.Translate(0.0f, -0.5f); 

	//		_hiyoriModel->Draw(device, context, matrix);
	//	}

	//	Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::EndFrame(device);
	//}
}
