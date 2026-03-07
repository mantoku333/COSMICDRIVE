#pragma once
#include <Windows.h>
#include <string>


namespace sf
{
	namespace window
	{
		class Window
		{
		public:
			Window();
			~Window();

			bool Create(int width, int height, int style = WS_OVERLAPPED | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
			bool Create(int style = WS_OVERLAPPED | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

		public:
			WNDCLASSEX wc;

			HWND hwnd;

			std::string windowName = "WindowName";
			std::string className = "ClassName";

		private:
			HMODULE hInst;
		};
	}
}