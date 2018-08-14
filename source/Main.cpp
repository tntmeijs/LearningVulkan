#include <Windows.h>

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

// Not using WinMain, as that is not working nicely with CMake
// Can't be bothered fixing it in a test project like this... ;)
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = windowProc;
	windowClass.hInstance = hInstance;
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
		hInstance,
		NULL);

	MSG msg;
	bool done = false;

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

	return msg.wParam;
}