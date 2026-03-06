#pragma once
#include <d3d11.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"
#include <string>

#ifdef _DEBUG
#define USEGUI
#endif

namespace sf
{
	namespace gui
	{
		class GUI
		{
		public:
			//GUIÇÃèâä˙âª
			static void Init(HWND hwnd, ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dContext);
			static void UnInit();
			static void Begin();
			static void End();
			static std::string GetOffSet();
		private:
			static int offset;
		};
	}
}