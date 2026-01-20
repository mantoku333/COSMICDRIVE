#include "ConfigCanvas.h"
#include "DirectX11.h"
#include "SInput.h"
#include "TitleScene.h"
#include "LoadingScene.h"
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

	float width = scale.x * 80.0f; // Approximate font width base
	float height = scale.y * 40.0f;

	float left = pos.x - width * 0.5f;
	float right = pos.x + width * 0.5f;
	float top = pos.y + height * 0.5f;
	float bottom = pos.y - height * 0.5f;

	return (mousePos.x >= left && mousePos.x <= right &&
		mousePos.y >= bottom && mousePos.y <= top);
}

void ConfigCanvas::ConfigItem::Update(float currentY, const Vector2& mousePos, bool isClicked)
{
	if (label) label->transform.SetPosition(Vector3(-400.0f, currentY, 0));
	if (valueLabel) valueLabel->transform.SetPosition(Vector3(-100.0f, currentY, 0));
	if (leftButton) leftButton->transform.SetPosition(Vector3(-250.0f, currentY, 0));
	if (rightButton) rightButton->transform.SetPosition(Vector3(100.0f, currentY, 0));

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
			btn->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // Normal White
		}
	};

	UpdateBtn(leftButton, onLeft);
	UpdateBtn(rightButton, onRight);
}

void ConfigCanvas::ConfigItem::SetVisible(bool visible)
{
	auto Set = [&](sf::ui::TextImage* t) { 
        if (t) {
            if (visible) {
                 // Creating/Updating sets proper scale, here we might just need to ensure it's not zero-scaled
                 // But wait, Update() continuously sets scale/pos?
                 // TextImage doesn't store "original scale".
                 // We can just move it far away if invisible.
            } else {
                 t->transform.SetPosition(Vector3(99999.0f, 99999.0f, 0.0f)); 
            }
        } 
    };
    // Alternative: Just don't update/draw it? 
    // But they are in the Canvas list, so they are drawn automatically.
    // If TextImage has no Active flag, modifying Transform is the way.
    
    // Better Implementation:
    // If visible, Update() sets the correct position.
    // If not visible, move to infinity.
    
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

	if (auto actor = actorRef.Target()) {
		if (auto* changer = actor->GetComponent<SceneChangeComponent>()) {
			sceneChanger = changer;
		}
	}

	// Setup List Items
	items.clear();
	scrollY = 0.0f;
	targetScrollY = 0.0f;

	RebuildLayout();

	updateCommand.Bind(std::bind(&ConfigCanvas::Update, this, std::placeholders::_1));
}

// End() removed

void ConfigCanvas::RebuildLayout()
{
	// Clear existing UI elements to prevent duplication/overlap
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

	// ----------------------
	// Control
	// ----------------------
	AddHeader(L"-- CONTROL --");
	AddBoolItem(L"Controller", &gGameConfig.isControllerMode);
	AddKeyItem(0, L"Lane 1");
	AddKeyItem(1, L"Lane 2");
	AddKeyItem(2, L"Lane 3");
	AddKeyItem(3, L"Lane 4");

	// ----------------------
	// Audio
	// ----------------------
	AddHeader(L"AUDIO");
	AddBoolItem(L"Tap Sound", &gGameConfig.enableTapSound);
	// Future: Volume sliders

	// Calculate total height
	float y = 0.0f;
	for (auto& item : items) {
		item->localY = y;
		y -= item->height; // Vertical stack downwards
	}
	totalContentHeight = -y;
}

void ConfigCanvas::Update(const sf::command::ICommand& command)
{
	HandleInput(command);
	UpdateScroll();

	// Hover Effect for Back Button
	if (isBackHovered) backButton->material.SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
	else backButton->material.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// Update Items
	bool isClicked = SInput::Instance().GetMouseDown(0);
	Vector2 mousePos = GetMousePosition();

	// Waiting State handles Key Input elsewhere, but we still draw
	if (state == State::WaitingForKey) {
		// Highlight waiting key item?
		// Handled by color update in items potentially or global override
	}

	float startY = LIST_START_Y + scrollY;
    
    // Masking range (simple visibility check)
    float viewTop = 600.0f;
    float viewBottom = -600.0f;

	for (auto& item : items) {
		float currentY = startY + item->localY;
        
        // Simple Culling
        bool visible = (currentY < viewTop && currentY > viewBottom);
        item->SetVisible(visible);

        if (visible) {
		    item->Update(currentY, mousePos, isClicked && state == State::Normal);
        }
	}
}

void ConfigCanvas::UpdateScroll()
{
	float wheel = SInput::Instance().GetMouseWheel();
	if (wheel != 0.0f) {
		targetScrollY -= wheel * 100.0f; // Scroll speed (Inverted)
	}

	// Clamp Scroll
	// content starts at 0 and goes down to -totalHeight
	// We want to scroll UP to see lower items.
	// So scrollY should go from 0 to totalHeight (roughly)
	// Actually, if items are at -y, we need StartY + (-y + scrollY)
	// Max scroll should be such that last item is visible.
	
	float maxScroll = std::max(0.0f, totalContentHeight - 500.0f); // 500 is approx view height adjustment
	if (targetScrollY < 0.0f) targetScrollY = 0.0f;
	if (targetScrollY > maxScroll) targetScrollY = maxScroll;

	// Smooth scroll
	scrollY += (targetScrollY - scrollY) * 0.2f;
}

void ConfigCanvas::HandleInput(const sf::command::ICommand& command)
{
	if (state == State::WaitingForKey) {
		DetectKeyInput();
		return;
	}

	Vector2 mousePos = GetMousePosition();
	isBackHovered = IsButtonHovered(mousePos, backButton);

	if (SInput::Instance().GetMouseDown(0)) {
		if (isBackHovered) {
			OnButtonPressed();
		}
	}

	if (SInput::Instance().GetKeyDown(Key::ESCAPE) || SInput::Instance().GetKeyDown(Key::KEY_BACK)) {
		OnButtonPressed();
	}
}

void ConfigCanvas::DetectKeyInput()
{
	for (int i = 0; i < 256; i++) {
		if (SInput::Instance().GetKeyDown((Key)i)) {
			Key k = (Key)i;
			if (waitingLaneIndex >= 0 && waitingLaneIndex < 4) {
				if (waitingLaneIndex == 0) gKeyConfig.lane1 = k;
				else if (waitingLaneIndex == 1) gKeyConfig.lane2 = k;
				else if (waitingLaneIndex == 2) gKeyConfig.lane3 = k;
				else if (waitingLaneIndex == 3) gKeyConfig.lane4 = k;
				
				SaveConfig();
				
				// Re-update the specific item text
				// Ideally we find the KeyItem and update it.
				// Since we rebuilt all on RebuildLayout, we can just trigger item updates.
				// But we need the values to refresh. The items hold pointers or we refresh value labels.
				
				RebuildLayout(); // Simple bruteforce refresh for now
			}
			state = State::Normal;
			waitingLaneIndex = -1;
			break;
		}
	}
}

void ConfigCanvas::OnButtonPressed()
{
	if (!sceneChanger.isNull()) {
		LoadingScene::SetLoadingType(LoadingType::Common);
		sceneChanger->ChangeScene(TitleScene::StandbyScene());
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
	item->height = 80.0f; // Smaller spacing for header

	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(3.0f, 0.8f, 0)); // Large
	item->label->Create(device, text, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf",
		80.0f, D2D1::ColorF(D2D1::ColorF::Yellow), 512, 128);
    // Header should be centered or left aligned? Currently implementation centers at -400.
    // Let's adjust header pos later if needed. For now it uses ConfigItem::Update logic which puts label at x=-400.
    // Maybe we want headers centered.
    
    // Override Update for Header to center it?
    // Or just make ConfigItem more flexible.
    // For now, let's keep it simple.

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

	item->rightButton = AddUI<sf::ui::TextImage>(); // Use right button as the main toggle button (or make it switch style)
	item->rightButton->transform.SetScale(Vector3(1.5f, 0.8f, 0));
	
	// Toggle logic
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
    
    // Bind click to right button for bool usually
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

	// Label in separate object or reuse label?
	// Existing had "LANE 1" label and Button with key name.
	
	item->label = AddUI<sf::ui::TextImage>();
	item->label->transform.SetScale(Vector3(1.5f, 0.6f, 0));
	item->label->Create(device, labelText, L"Assets/Fonts/\u30B4\u30C1\u30AB\u30AF\u30C3\u30C8.ttf", 
        80.0f, D2D1::ColorF(D2D1::ColorF::LightGray), 512, 128);
    // Override label position slightly? Standard position is fine.

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

    // Special behavior for Key Config: clicking sets waiting state
	item->onRight = [=]() {
		this->state = State::WaitingForKey;
		this->waitingLaneIndex = laneIndex;
		// Visual feedback handled in update loop by checking state/index?
		// Or we can change button color immediately here.
		item->rightButton->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	};
    
    // Add an UpdateView hook to handle the red color when waiting?
    item->onUpdateView = [=]() {
        if (state == State::WaitingForKey && waitingLaneIndex == laneIndex) {
             item->rightButton->material.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        }
    };

	items.push_back(item);
}
