#include "ConfigCanvas.h"
#include "DirectX11.h"
#include "SInput.h"
#include "TitleScene.h"
#include "LoadingScene.h"

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
}

void ConfigCanvas::HandleInput(const sf::command::ICommand& command)
{
	Vector2 mousePos = GetMousePosition();
	isBackHovered = IsButtonHovered(mousePos, backButton);

	// マウスクリック
	if (SInput::Instance().GetMouseDown(0)) {
		if (isBackHovered) {
			OnButtonPressed();
		}
	}

	// キーボード 
	// キーボード 
	if (SInput::Instance().GetKeyDown(Key::SPACE) || 
		SInput::Instance().GetKeyDown(Key::ESCAPE) || 
		SInput::Instance().GetKeyDown(Key::KEY_BACK)){
		OnButtonPressed();
	}
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
