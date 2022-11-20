#include <windows.h>
#include "playmidi.h"
#define GLATTER_HEADER_ONLY
#include <math.h>
#include <glatter/glatter.h>
#include "shaders.hpp"
#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")
HINSTANCE hInstance;
HWND  hWnd;
GLuint shaderProgram;

HDC deviceContext;
HGLRC renderContext;

static float vertices[] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f
};
unsigned int VBO, VAO;
bool transparent = false;
GLint timestampLocation, transparentLocation, usestdLocation;
LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static PAINTSTRUCT ps;
	int pf;
	PIXELFORMATDESCRIPTOR pfd;
	switch (uMsg) {
	case WM_NCCREATE:
		SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, NULL);

		deviceContext = GetDC(hWnd);
		//renderContext = wglCreateContext(deviceContext);

		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cGreenBits = 8;
		pfd.cRedBits = 8;
		pfd.cStencilBits = 8;
		pfd.cBlueBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		pf = ChoosePixelFormat(deviceContext, &pfd);
		SetPixelFormat(deviceContext, pf, &pfd);
		DescribePixelFormat(deviceContext, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		renderContext = wglCreateContext(deviceContext);
		wglMakeCurrent(deviceContext, renderContext);
		shaderProgram = compileShaderProgram();


		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		
		timestampLocation = glGetUniformLocation(shaderProgram, "timestamp");
		transparentLocation = glGetUniformLocation(shaderProgram, "transparent");
		glUniform1i(transparentLocation, 0);
		usestdLocation = glGetUniformLocation(shaderProgram, "use_standard");
		glUniform1i(usestdLocation, 0);
		glBindVertexArray(VAO);

		SetWindowPos(hWnd, NULL, NULL, NULL, 800, 800, SWP_NOMOVE | SWP_NOZORDER);
		SetTimer(hWnd, 0, 10, NULL);
		return TRUE;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, FALSE); UpdateWindow(hWnd);
		break;
	case WM_PAINT:
		glUniform1f(timestampLocation, fmod(GetTickCount64() / 1000.0, 1.5) / 1.5);
		glClearColor(0.0, 0.0, 0.0, transparent ? 0.0f : 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		SwapBuffers(deviceContext);
		break;
	case WM_SIZE:
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if (width > height)
			glViewport((width - height) / 2, 0, height, height);
		else
			glViewport(0, (height - width) / 2, width, width);
		InvalidateRect(hWnd, NULL, FALSE); UpdateWindow(hWnd);
		break;
	}
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		case VK_F12:
		{
			transparent = TRUE;

			glUniform1i(transparentLocation, 1);
			SetWindowLongA(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_APPWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED);
			SetWindowLongA(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX);
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

			HRGN region = CreateRectRgn(0, 0, -1, -1);
			DWM_BLURBEHIND bb = { 0 };
			bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
			bb.hRgnBlur = region;
			bb.fEnable = TRUE;

			DwmEnableBlurBehindWindow(hWnd, &bb);
			DeleteObject(region);
			break;
		}
		case VK_TAB:
		{
			GLint is_std;
			glGetUniformiv(shaderProgram, usestdLocation, &is_std);
			glUniform1i(usestdLocation, is_std == 0);
		}
		}
		break;
	case WM_DESTROY:
		ReleaseDC(hWnd, deviceContext);
		wglDeleteContext(renderContext);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0); return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	
	DWORD tid;
	CreateThread(NULL, 0, PlayMidiProc, NULL, 0, &tid);

	MSG   msg;
	WNDCLASS wc;

	hInstance = GetModuleHandle(NULL);

	wc.style = CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(struct WindowData*);
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"HeartbeatWindow";

	RegisterClass(&wc);
	hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_OVERLAPPEDWINDOW, L"HeartbeatWindow", L"Heartbeat", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN|WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
	DestroyWindow(hWnd);
	return 0;
}
