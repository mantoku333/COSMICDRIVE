#include "ConfigCanvas.h"
#include "DirectX11.h"
#include "SInput.h"
#include "TitleScene.h"
#include "LoadingScene.h"
#include "Config.h"
#include <DirectXMath.h>

using namespace app::test;
using namespace sf;

void ConfigCanvas::Begin()
{
	sf::ui::Canvas::Begin();



	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();

	if (auto actor = actorRef.Target()) {
		auto* changer = actor->GetComponent<SceneChangeComponent>();
		if (changer) {
			sceneChanger = changer;
		}
	}

	// ---------------------------------------------------------
	// BACK ボタン
	// ---------------------------------------------------------
	backButton = AddUI<sf::ui::TextImage>();
	// 右下に配置 (例)
	backButton->transform.SetPosition(Vector3(800, -450, 0));
	backButton->transform.SetScale(Vector3(3, 1.2f, 0));

	backButton->Create(
		device,
		L"BACK",
		L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		100.0f,
		D2D1::ColorF(D2D1::ColorF::White),
		512, 128
	);

	// ---------------------------------------------------------
	// Lane Config Buttons
	// ---------------------------------------------------------
	for (int i = 0; i < 4; i++) {
		laneLabels[i] = AddUI<sf::ui::TextImage>();
		laneLabels[i]->transform.SetPosition(Vector3(-400.0f + i * 250.0f, 200.0f, 0)); 
		laneLabels[i]->transform.SetScale(Vector3(1.5f, 0.6f, 0));
		std::wstring labelText = L"LANE " + std::to_wstring(i + 1);
		laneLabels[i]->Create(device, labelText.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 256, 128);


		laneButtons[i] = AddUI<sf::ui::TextImage>();
		laneButtons[i]->transform.SetPosition(Vector3(-400.0f + i * 250.0f, 100.0f, 0));
		laneButtons[i]->transform.SetScale(Vector3(1.5f, 0.8f, 0));
		// Initial Create (Text will be updated)
		laneButtons[i]->Create(device, L"-", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 256, 128);
	}
	UpdateButtonText();

    // ---------------------------------------------------------
    // HiSpeed Config
    // ---------------------------------------------------------
    // Label
    speedLabel = AddUI<sf::ui::TextImage>();
    speedLabel->transform.SetPosition(Vector3(-400.0f, -100.0f, 0));
    speedLabel->transform.SetScale(Vector3(2.0f, 0.6f, 0));
    speedLabel->Create(device, L"HiSpeed", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 256, 128);

    // Value
    speedValueLabel = AddUI<sf::ui::TextImage>();
    speedValueLabel->transform.SetPosition(Vector3(-100.0f, -100.0f, 0));
    speedValueLabel->transform.SetScale(Vector3(1.5f, 0.8f, 0));
    
    // Down Button
    speedDownButton = AddUI<sf::ui::TextImage>();
    speedDownButton->transform.SetPosition(Vector3(-250.0f, -100.0f, 0));
    speedDownButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
    speedDownButton->Create(device, L"<<", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

    // Up Button
    speedUpButton = AddUI<sf::ui::TextImage>();
    speedUpButton->transform.SetPosition(Vector3(100.0f, -100.0f, 0));
    speedUpButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
    speedUpButton->Create(device, L">>", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

    UpdateSpeedText();

    // ---------------------------------------------------------
    // Controller Mode
    // ---------------------------------------------------------
    controllerModeLabel = AddUI<sf::ui::TextImage>();
    controllerModeLabel->transform.SetPosition(Vector3(-400.0f, -250.0f, 0));
    controllerModeLabel->transform.SetScale(Vector3(2.0f, 0.6f, 0));
    controllerModeLabel->Create(device, L"Controller Mode", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128); // Increased width for label

    controllerModeButton = AddUI<sf::ui::TextImage>();
    controllerModeButton->transform.SetPosition(Vector3(0.0f, -250.0f, 0));
    controllerModeButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));
    
    UpdateControllerModeText();

    // ---------------------------------------------------------
    // Tap Sound
    // ---------------------------------------------------------
    tapSoundLabel = AddUI<sf::ui::TextImage>();
    tapSoundLabel->transform.SetPosition(Vector3(-400.0f, -320.0f, 0)); // Position below Controller Mode
    tapSoundLabel->transform.SetScale(Vector3(2.0f, 0.6f, 0));
    tapSoundLabel->Create(device, L"Tap Sound", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);

    tapSoundButton = AddUI<sf::ui::TextImage>();
    tapSoundButton->transform.SetPosition(Vector3(0.0f, -320.0f, 0));
    tapSoundButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));

    UpdateTapSoundText();

    // ---------------------------------------------------------
    // Offset Config
    // ---------------------------------------------------------
    // Label
    offsetLabel = AddUI<sf::ui::TextImage>();
    offsetLabel->transform.SetPosition(Vector3(-400.0f, -390.0f, 0));
    offsetLabel->transform.SetScale(Vector3(2.0f, 0.6f, 0));
    offsetLabel->Create(device, L"Offset", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 256, 128);

    // Value
    offsetValueLabel = AddUI<sf::ui::TextImage>();
    offsetValueLabel->transform.SetPosition(Vector3(-100.0f, -390.0f, 0));
    offsetValueLabel->transform.SetScale(Vector3(1.5f, 0.8f, 0));

    // Down Button
    offsetDownButton = AddUI<sf::ui::TextImage>();
    offsetDownButton->transform.SetPosition(Vector3(-250.0f, -390.0f, 0));
    offsetDownButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
    offsetDownButton->Create(device, L"<<", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

    // Up Button
    offsetUpButton = AddUI<sf::ui::TextImage>();
    offsetUpButton->transform.SetPosition(Vector3(100.0f, -390.0f, 0));
    offsetUpButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
    offsetUpButton->Create(device, L">>", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

    UpdateOffsetText();

    // ---------------------------------------------------------
    // FAST/SLOW Config
    // ---------------------------------------------------------
    fastSlowLabel = AddUI<sf::ui::TextImage>();
    fastSlowLabel->transform.SetPosition(Vector3(-400.0f, -460.0f, 0));
    fastSlowLabel->transform.SetScale(Vector3(2.0f, 0.6f, 0));
    fastSlowLabel->Create(device, L"FAST/SLOW", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);

    fastSlowButton = AddUI<sf::ui::TextImage>();
    fastSlowButton->transform.SetPosition(Vector3(0.0f, -460.0f, 0));
    fastSlowButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));

    UpdateFastSlowText();

	updateCommand.Bind(std::bind(&ConfigCanvas::Update, this, std::placeholders::_1));
}

void ConfigCanvas::Update(const sf::command::ICommand& command)
{
	HandleInput(command);

	// シンプルなホバーエフェクト
	if (isBackHovered) {
		backButton->material.SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
	}
	else {
		backButton->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	for (int i = 0; i < 4; i++) {
		DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		if (state == State::WaitingForKey && waitingLaneIndex == i) {
			color = { 1.0f, 0.0f, 0.0f, 1.0f }; // Waiting color (Red)
		}
		else if (isLaneHovered[i]) {
			color = { 1.0f, 1.0f, 0.0f, 1.0f }; // Hover
		}
		laneButtons[i]->material.SetColor(color);
	}
}

void ConfigCanvas::HandleInput(const sf::command::ICommand& command)
{
	if (state == State::WaitingForKey) {
		DetectKeyInput();
		return; 
	}

	Vector2 mousePos = GetMousePosition();
	isBackHovered = IsButtonHovered(mousePos, backButton);

	for (int i = 0; i < 4; i++) {
		isLaneHovered[i] = IsButtonHovered(mousePos, laneButtons[i]);
	}

	// マウスクリック
	if (SInput::Instance().GetMouseDown(0)) {
		if (isBackHovered) {
			OnButtonPressed();
		}
		for (int i = 0; i < 4; i++) {
			if (isLaneHovered[i]) {
				OnLaneButtonPressed(i);
			}
		}

        // HiSpeed Buttons
        if (IsButtonHovered(mousePos, speedUpButton)) {
            gGameConfig.hiSpeed += 0.5f;
            if (gGameConfig.hiSpeed > 10.0f) gGameConfig.hiSpeed = 10.0f;
            UpdateSpeedText();
            SaveConfig();
        }
        if (IsButtonHovered(mousePos, speedDownButton)) {
            gGameConfig.hiSpeed -= 0.5f;
            if (gGameConfig.hiSpeed < 0.0f) gGameConfig.hiSpeed = 0.0f;
            UpdateSpeedText();
            SaveConfig();
        }

        // Controller Mode Toggle
        // Controller Mode Toggle
        if (IsButtonHovered(mousePos, controllerModeButton)) {
            gGameConfig.isControllerMode = !gGameConfig.isControllerMode;
            UpdateControllerModeText();
            SaveConfig();
        }

        // Tap Sound Toggle
        if (IsButtonHovered(mousePos, tapSoundButton)) {
            gGameConfig.enableTapSound = !gGameConfig.enableTapSound;
            UpdateTapSoundText();
            SaveConfig();
        }

        // Offset Config
        if (IsButtonHovered(mousePos, offsetUpButton)) {
            gGameConfig.offsetSec += 0.01f;
            UpdateOffsetText();
            SaveConfig();
        }
        if (IsButtonHovered(mousePos, offsetDownButton)) {
            gGameConfig.offsetSec -= 0.01f;
            UpdateOffsetText();
            SaveConfig();
        }

        // FAST/SLOW Toggle
        if (IsButtonHovered(mousePos, fastSlowButton)) {
            gGameConfig.enableFastSlow = !gGameConfig.enableFastSlow;
            UpdateFastSlowText();
            SaveConfig();
        }
	}

	// キーボード 
	if (SInput::Instance().GetKeyDown(Key::SPACE) || 
		SInput::Instance().GetKeyDown(Key::ESCAPE) || 
		SInput::Instance().GetKeyDown(Key::KEY_BACK)){
		OnButtonPressed();
	}
}

void ConfigCanvas::DetectKeyInput()
{
	// Scan all keys roughly
	for (int i = 0; i < 256; i++) {
		if (SInput::Instance().GetKeyDown((Key)i)) {
			Key k = (Key)i;
			// Ignore some keys if needed, but for now allow most
			
			if (waitingLaneIndex == 0) gKeyConfig.lane1 = k;
			else if (waitingLaneIndex == 1) gKeyConfig.lane2 = k;
			else if (waitingLaneIndex == 2) gKeyConfig.lane3 = k;
			else if (waitingLaneIndex == 3) gKeyConfig.lane4 = k;

			// Save and finish
			SaveConfig();
			UpdateButtonText();
			state = State::Normal;
			waitingLaneIndex = -1;
			break;
		}
	}
}

void ConfigCanvas::OnLaneButtonPressed(int laneIndex)
{
	state = State::WaitingForKey;
	waitingLaneIndex = laneIndex;
	// Update visual to show waiting state? (Handled in Update color)
}

void ConfigCanvas::UpdateButtonText()
{
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();

	auto UpdateBtn = [&](int idx, Key k) {
		std::wstring s = KeyToString(k);
		laneButtons[idx]->Create(device, s.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
			80.0f, D2D1::ColorF(D2D1::ColorF::White), 256, 128);
	};

	UpdateBtn(0, gKeyConfig.lane1);
	UpdateBtn(1, gKeyConfig.lane2);
	UpdateBtn(2, gKeyConfig.lane3);
	UpdateBtn(3, gKeyConfig.lane4);
}

void ConfigCanvas::OnButtonPressed()
{
	if (!sceneChanger.isNull()) {
		LoadingScene::SetLoadingType(LoadingType::Common);
		sceneChanger->ChangeScene(TitleScene::StandbyScene());
	}
}

Vector2 ConfigCanvas::GetMousePosition()
{
	POINT mousePoint;
	GetCursorPos(&mousePoint);
	HWND hwnd = GetActiveWindow();
	ScreenToClient(hwnd, &mousePoint);

	float uiX = static_cast<float>(mousePoint.x) - screenWidth * 0.5f;
	float uiY = screenHeight * 0.5f - static_cast<float>(mousePoint.y);

	return Vector2(uiX, uiY);
}

bool ConfigCanvas::IsButtonHovered(const Vector2& mousePos, sf::ui::TextImage* button)
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

void ConfigCanvas::UpdateSpeedText() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    // 小数点は適当に整形
    std::wstring s = std::to_wstring(gGameConfig.hiSpeed);
    // 簡易的に先頭4文字くらい ("18.0"000 -> "18.0")
    if (s.length() > 4) s = s.substr(0, 4);

    speedValueLabel->Create(device, s.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, D2D1::ColorF(D2D1::ColorF::White), 256, 128);
}

void ConfigCanvas::UpdateControllerModeText() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    std::wstring text = gGameConfig.isControllerMode ? L"ON" : L"OFF";
    D2D1::ColorF color = gGameConfig.isControllerMode ? D2D1::ColorF(D2D1::ColorF::Green) : D2D1::ColorF(D2D1::ColorF::Red);

    controllerModeButton->Create(device, text.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, color, 256, 128);
}

void ConfigCanvas::UpdateTapSoundText() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    std::wstring text = gGameConfig.enableTapSound ? L"ON" : L"OFF";
    D2D1::ColorF color = gGameConfig.enableTapSound ? D2D1::ColorF(D2D1::ColorF::Green) : D2D1::ColorF(D2D1::ColorF::Red);

    tapSoundButton->Create(device, text.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, color, 256, 128);
}

void ConfigCanvas::UpdateOffsetText() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    // 符号付きで表示 (例: "+0.00s", "-0.05s")
    wchar_t buf[32];
    swprintf_s(buf, L"%+.2fs", gGameConfig.offsetSec);
    
    offsetValueLabel->Create(device, buf, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, D2D1::ColorF(D2D1::ColorF::White), 256, 128);
}

void ConfigCanvas::UpdateFastSlowText() {
    auto* dx11 = sf::dx::DirectX11::Instance();
    auto device = dx11->GetMainDevice().GetDevice();

    std::wstring text = gGameConfig.enableFastSlow ? L"ON" : L"OFF";
    D2D1::ColorF color = gGameConfig.enableFastSlow ? D2D1::ColorF(D2D1::ColorF::Green) : D2D1::ColorF(D2D1::ColorF::Red);

    fastSlowButton->Create(device, text.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
        80.0f, color, 256, 128);
}
