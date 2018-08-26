#include <Windows.h>
#include "LearningVulkan/Renderer.hpp"

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		break;
	}

	default:
	{
		break;
	}
	}

	// Just pass the parameters, nothing important just yet
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = windowProc;
	windowClass.hInstance = GetModuleHandle(0);
	windowClass.lpszClassName = "LearningVulkan";

	RegisterClassEx(&windowClass);

	HWND windowHandle = CreateWindowEx(
		NULL, "LearningVulkan", "Application",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1280,
		720,
		NULL,
		NULL,
		GetModuleHandle(0),
		NULL);

	MSG msg;
	bool done = false;

	RECT rect;
	GetClientRect(windowHandle, &rect);

	Renderer vulkanRenderer;
	vulkanRenderer.initialize(rect.right, rect.bottom, windowHandle);

	while (!done)
	{
		PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		RedrawWindow(windowHandle, NULL, NULL, RDW_INTERNALPAINT);
	}

	return 0;
}