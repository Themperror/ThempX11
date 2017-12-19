#include "ThempSystem.h"

#include "ThempD3D.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#pragma warning( disable : 4996) //disables warning unsafe function: freopen()
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
namespace Themp
{
	System* System::tSys;

	void System::Start()
	{
		m_D3D = new Themp::D3D();
		m_Game = new Themp::Game();
		m_Resources = new Themp::Resources();
		tSys->m_Quitting = !m_D3D->Init();
		if (tSys->m_Quitting) { MessageBox(m_Window, L"Failed to initialise all required D3D11 resources, Is your hardware supported?", L"ThempSystem - Critical Error", MB_OK); }
		m_Game->Start();

		Timer timer;
		timer.Init();
		double trackerTime = 0;
		int numSamples = 0;
		double frameTimeAdd = 0, tickTimeAdd=0;

		RECT windowRect;
		POINT cursorPos;
		GetWindowRect(m_Window, &windowRect);
		SetCursorPos(windowRect.left + (windowRect.right - windowRect.left) / 2, windowRect.top + (windowRect.bottom - windowRect.top) / 2);
		ShowCursor(false);
		if (!tSys->m_Quitting)
			while (!tSys->m_Quitting)
			{
				double delta = timer.GetDeltaTimeReset();
				trackerTime += delta;
				MSG msg;
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
						//1
						if (m_Game->m_Keys[49] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 0;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
							m_D3D->dirtySystemBuffer = true;
						}
						else if (m_Game->m_Keys[50] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 1;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
						else if (m_Game->m_Keys[51] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 2;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
						else if (m_Game->m_Keys[52] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 3;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
						else if (m_Game->m_Keys[53] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 4;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
						else if (m_Game->m_Keys[54] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 5;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
						else if (m_Game->m_Keys[55] == 2)
						{
							m_D3D->m_ConstantBufferData.visualType = 6;
							m_D3D->dirtySystemBuffer = true;
							float v = m_D3D->m_ConstantBufferData.visualType;
							printf("Visual Type: %f %s\n", m_D3D->m_ConstantBufferData.visualType, v == 0 ? "Default" : v == 1.0f ? "Diffuse" : v == 2.0f ? "NormalMaps" : v == 3.0f ? "Specular" : v == 4.0f ? "Misc" : v == 5.0f ? "UV's" : v == 6.0f ? "Normals" : "Unsupported");
						}
					}
					if (msg.message == WM_KEYUP)
					{
						m_Game->m_Keys[msg.wParam] = 0;
					}
				}
				GetWindowRect(m_Window, &windowRect);
				GetCursorPos(&m_Game->m_CursorPos);

				m_Game->m_CursorDeltaX = (windowRect.left + (windowRect.right - windowRect.left) / 2) - m_Game->m_CursorPos.x;
				m_Game->m_CursorDeltaY = (windowRect.top + (windowRect.bottom - windowRect.top) / 2) - m_Game->m_CursorPos.y;

				m_Game->Update(delta);
				if (!m_CursorShown)
				{
					SetCursorPos(windowRect.left + (windowRect.right - windowRect.left) / 2, windowRect.top + (windowRect.bottom - windowRect.top) / 2);
				}

				numSamples++;
				double tDelta = timer.GetDeltaTime();
				m_D3D->PrepareSystemBuffer(*m_Game);
				m_D3D->Draw(*m_Game);
				
				double frameDelta = (timer.GetDeltaTime() - tDelta);
				frameTimeAdd += frameDelta;
				tickTimeAdd += delta;
				if (trackerTime >= 1.0)
				{
					std::cout << "Avg FPS: " << (int)(trackerTime / (tickTimeAdd / (double)numSamples)) << "  Avg Frametime: " << frameTimeAdd / (double)numSamples << "   Avg Tick Time: " << tickTimeAdd-frameTimeAdd<<std::endl;
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
		delete m_D3D;
		m_D3D = nullptr;
		ShowCursor(true);
		m_CursorShown = true;
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
		std::cout << "Could not find config.ini, creating" << std::endl;
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


	newWindowSizeX = static_cast<int>(tSys->m_SVars.find("WindowSizeX")->second);
	newWindowSizeY = static_cast<int>(tSys->m_SVars.find("WindowSizeY")->second);
	ShowWindow(tSys->m_Window, nCmdShow);
	tSys->Start();
	delete tSys;
	return 0;
}
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//creating its own if, prevents stack allocating windowRect for every single message that comes through
	if (message == WM_SIZE)
	{
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			RECT windowRect;
			GetWindowRect(Themp::System::tSys->m_Window, &windowRect);

			newWindowSizeX = windowRect.right - windowRect.left;
			newWindowSizeY = windowRect.bottom - windowRect.top;
			Themp::System::tSys->m_D3D->ResizeWindow(newWindowSizeX, newWindowSizeY);
		}
	}

	RECT* windowRectPtr;
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
			windowRectPtr = (RECT*)lParam;
			newWindowSizeX = windowRectPtr->right - windowRectPtr->left;
			newWindowSizeY = windowRectPtr->bottom - windowRectPtr->top;
			//I see no reason why we should resize DURING the sizing move, rather wait until we're done resizing and then actually change everything..
		}break;
		case WM_EXITSIZEMOVE:
		{
			//we're done resizing the window, now resize all the rendering resources
			Themp::System::tSys->m_D3D->ResizeWindow(newWindowSizeX, newWindowSizeY);
		}break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}