#include "ConfigCanvas.h"
#include "DirectX11.h"
#include "SInput.h"
#include "TitleScene.h"
#include "LoadingScene.h"
#include "ConfigScene.h"
#include "Config.h"
#include <DirectXMath.h>
#include <algorithm>

using namespace app::test;
using namespace sf;

// ---------------------------------------------------------
// ConfigItem Implementation
// ---------------------------------------------------------
bool ConfigCanvas::ConfigItem::IsHovered(sf::ui::TextImage* btn, const Vector2& mousePos)
{
	if (!btn) return false;
	Vector3 pos = btn->transform.GetPosition();
	Vector3 scale = btn->transform.GetScale();

	float width = scale.x * 80.0f;
	float height = scale.y * 40.0f;

	float left = pos.x - width * 0.5f;
	float right = pos.x + width * 0.5f;
	float top = pos.y + height * 0.5f;
	float bottom = pos.y - height * 0.5f;

	return (mousePos.x >= left && mousePos.x <= right &&
		mousePos.y >= bottom && mousePos.y <= top);
}

void ConfigCanvas::ConfigItem::Update(float currentY, const Vector2& mousePos, bool isClicked, bool isSelected)
{
	if (label) label->transform.SetPosition(Vector3(-400.0f, currentY, 0));
	if (valueLabel) valueLabel->transform.SetPosition(Vector3(-100.0f, currentY, 0));
	if (leftButton) leftButton->transform.SetPosition(Vector3(-250.0f, currentY, 0));
	if (rightButton) rightButton->transform.SetPosition(Vector3(100.0f, currentY, 0));

    // Highlight Logic:
    // Priority: Mouse Hover > Key Selection
    // If selected by key, label turns Cyan.

    if (label) {
        if (isSelected) label->material.SetColor({ 0.0f, 1.0f, 1.0f, 1.0f }); // Cyan for selected item
        else label->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); 
    }

	// Hover & Click Logic
	auto UpdateBtn = [&](sf::ui::TextImage* btn, const std::function<void()>& action) {
		if (!btn) return;
		if (IsHovered(btn, mousePos)) {
			btn->material.SetColor({ 1.0f, 1.0f, 0.0f, 1.0f }); // Hover Yellow
			if (isClicked && action) {
				action();
				if (onUpdateView) onUpdateView();
			}
		}
		else {
            // If selected by key, maybe highlight buttons too?
            if (isSelected) btn->material.SetColor({ 0.8f, 1.0f, 1.0f, 1.0f }); // Slight tint
			else btn->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // Normal White
		}
	};

	UpdateBtn(leftButton, onLeft);
	UpdateBtn(rightButton, onRight);
}

void ConfigCanvas::ConfigItem::SetVisible(bool visible)
{
    if (!visible) {
        auto Hide = [&](sf::ui::TextImage* t) { if(t) t->transform.SetPosition(Vector3(10000.0f, 0, 0)); };
        Hide(label);
        Hide(valueLabel);
        Hide(leftButton);
        Hide(rightButton);
    }
}

// ---------------------------------------------------------
// ConfigCanvas Implementation
// ---------------------------------------------------------

void ConfigCanvas::Begin()
{
	sf::ui::Canvas::Begin();

	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();

	// Setup List Items
	items.clear();
	scrollY = 0.0f;
	targetScrollY = 0.0f;

	RebuildLayout();

	updateCommand.Bind(std::bind(&ConfigCanvas::Update, this, std::placeholders::_1));
}

void ConfigCanvas::RebuildLayout()
{
	for (auto* ui : uis) {
		delete ui;
	}
	uis.clear();
	items.clear();

	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();

	// BACK ボタン (再作成)
	backButton = AddUI<sf::ui::TextImage>();
	backButton->transform.SetPosition(Vector3(800, -450, 0));
	backButton->transform.SetScale(Vector3(3, 1.2f, 0));
	backButton->Create(device, L"BACK", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		100.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128);

	// ----------------------
	// General
	// ----------------------
	AddHeader(L"-- GENERAL --");
	AddFloatItem(L"HiSpeed", &gGameConfig.hiSpeed, 0.5f, 0.0f, 10.0f);
	AddFloatItem(L"Offset", &gGameConfig.offsetSec, 0.01f, -5.0f, 5.0f, L"%+.2fs");
	AddBoolItem(L"FAST/SLOW", &gGameConfig.enableFastSlow);
	AddBoolItem(L"Log Output", &gGameConfig.enableLog);
	AddBoolItem(L"Command Log", &gGameConfig.enableCommandLog);
	AddBoolItem(L"Input Latency", &gGameConfig.enableInputLatencyLog);
	AddBoolItem(L"Sound Latency", &gGameConfig.enableSoundLatencyLog);

	// ----------------------
	// Control
	// ----------------------
	AddHeader(L"-- CONTROL --");
	AddBoolItem(L"Controller", &gGameConfig.isControllerMode);
	AddBoolItem(L"Debug Camera", &gGameConfig.enableDebugCamera);
	AddKeyItem(0, L"Lane 1");
	AddKeyItem(1, L"Lane 2");
	AddKeyItem(2, L"Lane 3");
	AddKeyItem(3, L"Lane 4");

	// ----------------------
	// Audio
	// ----------------------
	AddHeader(L"AUDIO");
	AddBoolItem(L"Tap Sound", &gGameConfig.enableTapSound);

	// Calculate total height
	float y = 0.0f;
	for (auto& item : items) {
		item->localY = y;
		y -= item->height;
	}
	totalContentHeight = -y;
}

void ConfigCanvas::Update(const sf::command::ICommand& command)
{
    // MVP: 入力検出と通知
    if (presenter) {
        auto& input = SInput::Instance();
        
        // Navigation (with Repeat check if needed, but simple press for now)
        if (input.GetKeyDown(Key::KEY_UP) || input.GetKeyDown(Key::KEY_W)) presenter->OnNavigateUp();
        if (input.GetKeyDown(Key::KEY_DOWN) || input.GetKeyDown(Key::KEY_S)) presenter->OnNavigateDown();
        if (input.GetKeyDown(Key::KEY_LEFT) || input.GetKeyDown(Key::KEY_A)) presenter->OnNavigateLeft();
        if (input.GetKeyDown(Key::KEY_RIGHT) || input.GetKeyDown(Key::KEY_D)) presenter->OnNavigateRight();
        
        if (input.GetKeyDown(Key::SPACE)) presenter->OnConfirm();
        if (input.GetKeyDown(Key::ESCAPE) || input.GetKeyDown(Key::KEY_BACK)) presenter->OnCancel();
        
        // Key Config Waiting Check
        if (presenter->GetState() == ConfigScene::State::WaitingForKey) {
             for(int i=0; i<256; i++) {
                 if (input.GetKeyDown((Key)i)) {
                     presenter->OnKeyInput(i);
                     RebuildLayout(); // Update key labels
                     break;
                 }
             }
        }
    }

	UpdateScroll();

	// Hover Effect for Back Button
    Vector2 mousePos = GetMousePosition();
	isBackHovered = IsButtonHovered(mousePos, backButton);

	if (isBackHovered) backButton->material.SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
	else backButton->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    
    // Back Button Click
    if (SInput::Instance().GetMouseDown(0) && isBackHovered) {
        OnButtonPressed();
    }

	// Update Items
	bool isClicked = SInput::Instance().GetMouseDown(0);
    
    // Check if we are in normal state for mouse interactions
    bool isNormalState = true;
    if (presenter && presenter->GetState() != ConfigScene::State::Normal) {
        isNormalState = false;
    }

	float startY = LIST_START_Y + scrollY;
    
    // Masking range
    float viewTop = 600.0f;
    float viewBottom = -600.0f;
    
    int selectedIdx = presenter ? presenter->GetSelectedIndex() : -1;

	for (int i = 0; i < (int)items.size(); ++i) {
        auto& item = items[i];
		float currentY = startY + item->localY;
        
        bool visible = (currentY < viewTop && currentY > viewBottom);
        item->SetVisible(visible);

        if (visible) {
            bool isSelected = (i == selectedIdx);
		    item->Update(currentY, mousePos, isClicked && isNormalState, isSelected);
        }
	}
}

void ConfigCanvas::UpdateScroll()
{
	float wheel = SInput::Instance().GetMouseWheel();
	if (wheel != 0.0f) {
		targetScrollY -= wheel * 100.0f;
	}

	float maxScroll = std::max(0.0f, totalContentHeight - 500.0f);
	if (targetScrollY < 0.0f) targetScrollY = 0.0f;
	if (targetScrollY > maxScroll) targetScrollY = maxScroll;

	scrollY += (targetScrollY - scrollY) * 0.2f;
}

void ConfigCanvas::UpdateSelection(int index)
{
    // Auto Scroll logic
    if (index >= 0 && index < (int)items.size()) {
        float itemY = items[index]->localY; // Negative value usually
        
        // Target: LIST_START_Y + scrollY + itemY = 0 (Top) or Center
        // Let's center it: PosY = 0
        // LIST_START_Y + scrollY + itemY = 0 => scrollY = -itemY - LIST_START_Y
        
        targetScrollY = -itemY - LIST_START_Y;
        
        // Clamp
        float maxScroll = std::max(0.0f, totalContentHeight - 500.0f);
    	if (targetScrollY < 0.0f) targetScrollY = 0.0f;
	    if (targetScrollY > maxScroll) targetScrollY = maxScroll;
    }
}

void ConfigCanvas::ExecuteItem(int index)
{
    if (index >= 0 && index < items.size()) {
        if (items[index]->onRight) items[index]->onRight();
    }
}

void ConfigCanvas::ExecuteLeft(int index)
{
    if (index >= 0 && index < items.size()) {
        if (items[index]->onLeft) items[index]->onLeft();
    }
}

void ConfigCanvas::ExecuteRight(int index)
{
    if (index >= 0 && index < items.size()) {
        if (items[index]->onRight) items[index]->onRight();
    }
}

void ConfigCanvas::OnButtonPressed()
{
    // Back button pressed -> Cancel
	if (presenter) {
        presenter->OnCancel();
    }
	SaveConfig();
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
	return (mousePos.x >= left && mousePos.x <= right && mousePos.y >= bottom && mousePos.y <= top);
}

// ---------------------------------------------------------
// Factories
// ---------------------------------------------------------
void ConfigCanvas::AddHeader(const wchar_t* text)
{
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();

	auto item = std::make_shared<ConfigItem>();
	item->height = 80.0f;

	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(3.0f, 0.8f, 0));
	item->label->Create(device, text, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		80.0f, D2D1::ColorF(D2D1::ColorF::Yellow), 512, 128);

	items.push_back(item);
}

void ConfigCanvas::AddBoolItem(const wchar_t* labelText, bool* targetBool)
{
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();
	auto item = std::make_shared<ConfigItem>();
    item->height = ITEM_SPACING;

	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(2.0f, 0.6f, 0));
	item->label->Create(device, labelText, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);

	item->rightButton = AddUI<sf::ui::TextImage>();
	item->rightButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));
	
	auto Refresh = [=]() {
		std::wstring s = *targetBool ? L"ON" : L"OFF";
		D2D1::ColorF c = *targetBool ? D2D1::ColorF(D2D1::ColorF::Green) : D2D1::ColorF(D2D1::ColorF::Red);
		item->rightButton->Create(device, s.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, c, 256, 128);
	};
	Refresh();

	item->onRight = [=]() {
		*targetBool = !(*targetBool);
		Refresh();
		SaveConfig();
	};
    
    item->leftButton = nullptr; 
	items.push_back(item);
}

void ConfigCanvas::AddFloatItem(const wchar_t* labelText, float* targetFloat, float step, float min, float max, const wchar_t* format)
{
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();
	auto item = std::make_shared<ConfigItem>();
    item->height = ITEM_SPACING;

	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(2.0f, 0.6f, 0));
	item->label->Create(device, labelText, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);

	item->valueLabel = AddUI<sf::ui::TextImage>();
	item->valueLabel->transform.SetScale(Vector3(1.5f, 0.8f, 0));

	item->leftButton = AddUI<sf::ui::TextImage>();
	item->leftButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
	item->leftButton->Create(device, L"<<", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

	item->rightButton = AddUI<sf::ui::TextImage>();
	item->rightButton->transform.SetScale(Vector3(0.8f, 0.8f, 0));
	item->rightButton->Create(device, L">>", L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 128);

	auto Refresh = [=]() {
		wchar_t buf[32];
		swprintf_s(buf, format, *targetFloat);
		item->valueLabel->Create(device, buf, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128);
	};
	Refresh();

	item->onLeft = [=]() {
		*targetFloat -= step;
		if (*targetFloat < min) *targetFloat = min;
		Refresh();
		SaveConfig();
	};
	item->onRight = [=]() {
		*targetFloat += step;
		if (*targetFloat > max) *targetFloat = max;
		Refresh();
		SaveConfig();
	};

	items.push_back(item);
}

void ConfigCanvas::AddKeyItem(int laneIndex, const wchar_t* labelText)
{
	auto* dx11 = sf::dx::DirectX11::Instance();
	auto device = dx11->GetMainDevice().GetDevice();
	auto item = std::make_shared<ConfigItem>();
    item->height = ITEM_SPACING;
	
	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(1.5f, 0.6f, 0));
	item->label->Create(device, labelText, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);

	item->rightButton = AddUI<sf::ui::TextImage>();
	item->rightButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));

	auto Refresh = [=]() {
		Key k = Key::KEY_UNKNOWN;
		if (laneIndex == 0) k = gKeyConfig.lane1;
		else if (laneIndex == 1) k = gKeyConfig.lane2;
		else if (laneIndex == 2) k = gKeyConfig.lane3;
		else if (laneIndex == 3) k = gKeyConfig.lane4;
		
		std::wstring s = KeyToString(k);
		item->rightButton->Create(device, s.c_str(), L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 256, 128);
	};
	Refresh();

	item->onRight = [=]() {
        // Use Presenter logic for state change
        if (this->presenter) {
            this->presenter->OnKeyInputStart(laneIndex);
        }
        item->rightButton->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	};
    
    item->onUpdateView = [=]() {
        // Red color if waiting
        if (this->presenter) {
            if (this->presenter->GetState() == ConfigScene::State::WaitingForKey && 
                this->presenter->GetWaitingLaneIndex() == laneIndex) {
                 item->rightButton->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
            }
        }
    };

	items.push_back(item);
}
