#include "GUI.h"

int sf::gui::GUI::offset = 0;

void sf::gui::GUI::Init(HWND hwnd, ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dContext)
{
	ImGui::CreateContext();

	//ì˙ñ{åÍâª
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 24.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

	//ImGUIÇÃèâä˙âª
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiInputTextFlags_EnterReturnsTrue;

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(d3dDevice, d3dContext);
	ImGui_ImplDX11_CreateDeviceObjects();
}

void sf::gui::GUI::UnInit()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void sf::gui::GUI::Begin()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	offset = 0;
}

void sf::gui::GUI::End()
{
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

std::string sf::gui::GUI::GetOffSet()
{
	offset++;
	return std::string("##" + std::to_string(offset));
}
