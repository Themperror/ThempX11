#include "ThempSystem.h"

#include "ThempD3D.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "ThempGUI.h"

#include <imgui.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdarg>
#include <sys/timeb.h>

#pragma warning( disable : 4996) //disables warning unsafe function: freopen()
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
IMGUI_API LRESULT ImGui_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ImGui_PrepareFrame();

namespace Themp
{
	System* System::tSys = nullptr;
	FILE* System::logFile = nullptr;
	
	void System::Start()
	{
		m_D3D = new Themp::D3D();
		m_Game = new Themp::Game();
		m_Resources = new Themp::Resources();
		tSys->m_Quitting = !m_D3D->Init();
		if (tSys->m_Quitting) { MessageBox(m_Window, L"Failed to initialise all required D3D11 resources, Is your hardware supported?", L"ThempSystem - Critical Error", MB_OK); }
		
		m_GUI = new Themp::GUI(m_Window);

		m_Game->Start();


		Timer timer;
		timer.Init();
		double trackerTime = 0;
		int numSamples = 0;
		double frameTimeAdd = 0, tickTimeAdd=0;

		RECT windowRect,clientRect;
		POINT cursorPos;
		GetWindowRect(m_Window, &windowRect);
		GetClientRect(m_Window, &clientRect);


		m_D3D->ResizeWindow(clientRect.right, clientRect.bottom);

		//printf("BorderX: %i\n BorderY: %i\n Caption: %i\n", borderX, borderY, caption);
		SetCursorPos(windowRect.left + (windowRect.right - windowRect.left) / 2, windowRect.top + (windowRect.bottom - windowRect.top) / 2);
		ShowCursor(false);
		ImGuiIO& io = ImGui::GetIO();
		while (!tSys->m_Quitting)
		{
			double delta = timer.GetDeltaTimeReset();
			trackerTime += delta;
			MSG msg;
			io = ImGui::GetIO();
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (msg.message == WM_QUIT)
				{
					tSys->m_Quitting = true;
				}
				if (msg.message == WM_LBUTTONDOWN)
				{
					m_Game->m_Keys[1022] = m_Game->m_Keys[1022] == 0 ? 2 : 1;
				}
				if (msg.message == WM_LBUTTONUP)
				{
					m_Game->m_Keys[1022] = 0;
				}
				if (msg.message == WM_RBUTTONDOWN)
				{
					m_Game->m_Keys[1023] = m_Game->m_Keys[1023] == 0 ? 2 : 1;
				}
				if (msg.message == WM_RBUTTONUP)
				{
					m_Game->m_Keys[1023] = 0;
				}
				if (msg.message == WM_KEYDOWN)
				{
					m_Game->m_Keys[msg.wParam] = m_Game->m_Keys[msg.wParam] == 0 ? 2 : 1;
					if (msg.wParam == VK_ESCAPE)tSys->m_Quitting = true;
					//std::cout << msg.wParam << std::endl;
					if (msg.wParam == VK_TAB)
					{
						m_CursorShown = !m_CursorShown;
						ShowCursor(m_CursorShown);
					}
				}
				if (msg.message == WM_KEYUP)
				{
					m_Game->m_Keys[msg.wParam] = 0;
				}
			}
			//sadly we need all these calls
			GetWindowRect(m_Window, &windowRect);
			GetClientRect(m_Window, &clientRect);
			GetCursorPos(&m_Game->m_CursorPos);
			int windowDiffX = (windowRect.right - windowRect.left - clientRect.right) / 2;
			int windowDiffY = (windowRect.bottom - windowRect.top - clientRect.bottom) * 0.75;

			io.DeltaTime = delta;
			m_Game->m_CursorDeltaX = (windowRect.left + (windowRect.right - windowRect.left) / 2) - m_Game->m_CursorPos.x;
			m_Game->m_CursorDeltaY = (windowRect.top + (windowRect.bottom - windowRect.top) / 2) - m_Game->m_CursorPos.y;

			//windows Title bar messes up the actual mouse position for collision testing with the UI, so I adjust it to fit "good enough" since getting exact measurements from top and bottom is a pain
			io.MousePos = ImVec2(m_Game->m_CursorPos.x - windowRect.left - windowDiffX, m_Game->m_CursorPos.y - windowRect.top - windowDiffY);
			io.DisplaySize = ImVec2(clientRect.right, clientRect.bottom);

			ImGui_PrepareFrame();
			//finally we can start our important stuff, ImGui::NewFrame is needed for ImGui so it can get set up for any UI calls we make
			ImGui::NewFrame();

			m_Game->Update(delta);
			if (!m_CursorShown)
			{
				SetCursorPos(windowRect.left + (windowRect.right - windowRect.left) / 2, windowRect.top + (windowRect.bottom - windowRect.top) / 2);
			}

			//Doesn't actually render but prepares render data for us to use
			ImGui::Render();

			numSamples++;
			double tDelta = timer.GetDeltaTime();
			m_D3D->PrepareSystemBuffer(*m_Game);
			m_D3D->Draw(*m_Game);
			m_D3D->DrawImGUI();

			double frameDelta = (timer.GetDeltaTime() - tDelta);
			frameTimeAdd += frameDelta;
			tickTimeAdd += delta;
			if (trackerTime >= 1.0)
			{
				System::Print("Avg FPS: %3i  Avg Frametime: %.4f   Avg Tick Time: %.4f", (int)(trackerTime / (tickTimeAdd / (double)numSamples)), frameTimeAdd / (double)numSamples, tickTimeAdd - frameTimeAdd);
				trackerTime = 0;
				frameTimeAdd = 0;
				tickTimeAdd = 0;
				numSamples = 0;
			}
		}
		m_Game->Stop();
		delete m_Game;
		m_Game = nullptr;
		Themp::System::tSys->m_Game = nullptr;
		delete m_Resources;
		m_Resources = nullptr;
		delete m_GUI;
		m_GUI = nullptr;
		delete m_D3D;
		m_D3D = nullptr;
		ShowCursor(true);
		m_CursorShown = true;
		ImGui::DestroyContext();
	}
}

int newWindowSizeX = 0;
int newWindowSizeY = 0;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,	LPSTR lpCmdLine,int nCmdShow)
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	Themp::System::tSys = new Themp::System();
	Themp::System* tSys = Themp::System::tSys;
	Themp::System::logFile = fopen("log.txt", "w+");
	std::ifstream configFile("config.ini");
	std::string line;
	if (configFile.is_open())
	{
		while (std::getline(configFile, line))
		{
			size_t cIndex = line.find(" ", 0);
			if (cIndex != std::string::npos)
			{
				tSys->m_SVars[line.substr(0, cIndex)] = std::stof(line.substr(cIndex + 1, line.size() - (cIndex + 1)));
			}
		}
		configFile.close();
	}
	else
	{
		Themp::System::Print("Could not find config.ini, creating");
		std::ofstream nConfig("config.ini");
		if (nConfig.is_open())
		{
			nConfig << "Fullscreen 0\n";
			nConfig << "WindowPosX 0\n";
			nConfig << "WindowPosY 0\n";
			nConfig << "WindowSizeX 800\n";
			nConfig << "WindowSizeY 600\n";
			nConfig << "Anisotropic_Filtering 1\n";

			nConfig.close();
		}
		tSys->m_SVars[std::string("Fullscreen")] = 0;
		tSys->m_SVars[std::string("WindowPosX")] = 0;
		tSys->m_SVars[std::string("WindowPosY")] = 0;
		tSys->m_SVars[std::string("WindowSizeX")] = 800;
		tSys->m_SVars[std::string("WindowSizeY")] = 600;
		tSys->m_SVars[std::string("Anisotropic_Filtering")] = 1;
	}
	
	//for (std::map<std::string,float>::iterator i = tSys->m_SVars.begin(); i != tSys->m_SVars.end(); i++)
	//{
	//	std::cout << i->first << "  "<<i->second << std::endl;
	//}
	//Sleep(4000);
	ImGui::CreateContext();
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW; 
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if(tSys->m_SVars.find("Fullscreen")->second == 1)
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"ThempX11";

	RegisterClassEx(&wc);

	if (tSys->m_SVars.find("Fullscreen")->second == 1)
	{
		HWND desktop = GetDesktopWindow();
		RECT bSize;
		GetWindowRect(desktop, &bSize);

		tSys->m_SVars.find("WindowSizeX")->second = static_cast<float>(bSize.right);
		tSys->m_SVars.find("WindowSizeY")->second = static_cast<float>(bSize.bottom);
		tSys->m_Window = CreateWindowEx(NULL,
			L"ThempX11",
			L"ThempX11",
			WS_EX_TOPMOST,
			bSize.left,
			bSize.top,
			bSize.right,
			bSize.bottom,
			NULL, NULL, hInstance, NULL);
	}
	else
	{
		tSys->m_Window = CreateWindowEx(NULL,
			L"ThempX11",
			L"ThempX11",
			WS_OVERLAPPEDWINDOW,
			static_cast<int>(tSys->m_SVars.find("WindowPosX")->second ),
			static_cast<int>(tSys->m_SVars.find("WindowPosY")->second ),
			static_cast<int>(tSys->m_SVars.find("WindowSizeX")->second),
			static_cast<int>(tSys->m_SVars.find("WindowSizeY")->second),
			NULL, NULL, hInstance, NULL);
	}
	RECT winRect;

	newWindowSizeX = static_cast<int>(tSys->m_SVars.find("WindowSizeX")->second);
	newWindowSizeY = static_cast<int>(tSys->m_SVars.find("WindowSizeY")->second);

	winRect.left = 0;
	winRect.right = newWindowSizeX;
	winRect.top = 0;
	winRect.bottom = newWindowSizeY;

	//We have to recalculate and rescale the window because window size doesn't equal render (client) size, so windows will rescale the render targets
	AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, true);
	SetWindowPos(tSys->m_Window, 0, static_cast<int>(tSys->m_SVars.find("WindowPosX")->second), static_cast<int>(tSys->m_SVars.find("WindowPosY")->second), winRect.right, winRect.bottom, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	ShowWindow(tSys->m_Window, nCmdShow);

	//reset all values again
	newWindowSizeX = winRect.right;
	newWindowSizeY = winRect.bottom;

	ImGuiIO& imgIo = ImGui::GetIO();
	imgIo.DisplaySize.x = winRect.right;
	imgIo.DisplaySize.y = winRect.bottom;

	imgIo.KeyMap[ImGuiKey_Tab] = VK_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	imgIo.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	imgIo.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	imgIo.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	imgIo.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	imgIo.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	imgIo.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	imgIo.KeyMap[ImGuiKey_Home] = VK_HOME;
	imgIo.KeyMap[ImGuiKey_End] = VK_END;
	imgIo.KeyMap[ImGuiKey_Insert] = VK_INSERT;
	imgIo.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	imgIo.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	imgIo.KeyMap[ImGuiKey_Space] = VK_SPACE;
	imgIo.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	imgIo.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	imgIo.KeyMap[ImGuiKey_A] = 'A';
	imgIo.KeyMap[ImGuiKey_C] = 'C';
	imgIo.KeyMap[ImGuiKey_V] = 'V';
	imgIo.KeyMap[ImGuiKey_X] = 'X';
	imgIo.KeyMap[ImGuiKey_Y] = 'Y';
	imgIo.KeyMap[ImGuiKey_Z] = 'Z';

	imgIo.ImeWindowHandle = tSys->m_Window;
	Themp::System::Print("TestNoArgs");
	try
	{
		tSys->Start();
	}
	catch (std::exception& e)
	{
		Themp::System::Print(e.what());
		throw e;
	}
	fclose(Themp::System::logFile);
	delete tSys;
	return 0;
}

void Themp::System::Print(const char* message, ...)
{
	int strLength = strlen(message);
	int fmtMsgSize = strLength < 64 ? 64 : strLength * 2;
	char* buffer = new char[fmtMsgSize];
	memset(buffer, 0, fmtMsgSize);
	std::string timestamp;
	char msg[64];
	timeb t;
	ftime(&t);
	strftime(msg, sizeof(msg), "[%T", localtime(&t.time));
	timestamp.insert(0, msg);
	timestamp.append(":");
	memset(msg, 0, 4);
	short msVal = t.millitm;
	timestamp.append(itoa(t.millitm, msg, 10));
	timestamp.append(msVal < 100 ? "0] " : "] ");
	va_list args;
	va_start(args, message);
	vsnprintf(buffer, fmtMsgSize, message, args);
	//snprintf(buffer, fmtMsgSize, message, args);
	va_end(args);
	timestamp.append(buffer);
	timestamp.append("\n");
	printf("%s", timestamp.c_str());
	delete buffer;
	if (logFile)
	{
		fwrite(timestamp.c_str(), timestamp.size(), 1, logFile);
		fflush(logFile);
	}
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ImGui_WndProcHandler(hWnd, message, wParam, lParam);
	ImGuiIO& imgIo = ImGui::GetIO();
	//creating its own if, prevents stack allocating windowRect for every single message that comes through
	if (message == WM_SIZE)
	{
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			RECT windowRect;
			GetClientRect(Themp::System::tSys->m_Window, &windowRect);

			newWindowSizeX = windowRect.right;
			newWindowSizeY = windowRect.bottom;

			imgIo.DisplaySize.x = newWindowSizeX;
			imgIo.DisplaySize.y = newWindowSizeY;
			if(Themp::System::tSys->m_D3D)
				Themp::System::tSys->m_D3D->ResizeWindow(newWindowSizeX, newWindowSizeY);
		}
	}

	RECT windowRect;
	switch (message)
	{
		case WM_CLOSE:
		{
			Themp::System::tSys->m_Quitting = true;
		}break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0; 
		}break;
		case WM_SIZING:
		{
			//I see no reason why we should resize DURING the sizing move, rather wait until we're done resizing and then actually change everything..
		}break;
		case WM_EXITSIZEMOVE:
		{
			//we're done resizing the window, now resize all the rendering resources
			GetClientRect(Themp::System::tSys->m_Window, &windowRect);
			newWindowSizeX = windowRect.right;
			newWindowSizeY = windowRect.bottom;
			imgIo.DisplaySize.x = windowRect.right;
			imgIo.DisplaySize.y = windowRect.bottom;
			Themp::System::tSys->m_D3D->ResizeWindow(newWindowSizeX, newWindowSizeY);
		}break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static void ImGui_UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiMouseCursor imgui_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (imgui_cursor == ImGuiMouseCursor_None)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		::SetCursor(NULL);
	}
	else
	{
		// Hardware cursor type
		LPTSTR win32_cursor = IDC_ARROW;
		switch (imgui_cursor)
		{
		case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
		case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
		case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
		case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
		case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
		case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
		case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
		}
		::SetCursor(::LoadCursor(NULL, win32_cursor));
	}
}
IMGUI_API LRESULT ImGui_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == NULL)
		return 0;

	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	{
		int button = 0;
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
		if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
		if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
			::SetCapture(hwnd);
		io.MouseDown[button] = true;
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		int button = 0;
		if (msg == WM_LBUTTONUP) button = 0;
		if (msg == WM_RBUTTONUP) button = 1;
		if (msg == WM_MBUTTONUP) button = 2;
		io.MouseDown[button] = false;
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
			::ReleaseCapture();
		return 0;
	}
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return 0;
	case WM_MOUSEHWHEEL:
		io.MouseWheelH += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return 0;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		return 0;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return 0;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return 0;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return 0;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			ImGui_UpdateMouseCursor();
			return 1;
		}
		return 0;
	}
	return 0;
}
ImGuiMouseCursor oldCursor;
void ImGui_PrepareFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Read keyboard modifiers inputs
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;
	// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
	// io.MousePos : filled by WM_MOUSEMOVE events
	// io.MouseDown : filled by WM_*BUTTON* events
	// io.MouseWheel : filled by WM_MOUSEWHEEL events

	// Set OS mouse position if requested last frame by io.WantMoveMouse flag (used when io.NavMovesTrue is enabled by user and using directional navigation)
	if (io.WantMoveMouse)
	{
		POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
		ClientToScreen(Themp::System::tSys->m_Window, &pos);
		SetCursorPos(pos.x, pos.y);
	}

	// Update OS mouse cursor with the cursor requested by imgui
	ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (oldCursor != mouse_cursor)
	{
		oldCursor = mouse_cursor;
		ImGui_UpdateMouseCursor();
	}

	// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
	//ImGui::NewFrame();
}