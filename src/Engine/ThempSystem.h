#pragma once

#include <Windows.h>
#include <map>
#include <vector>
#include <time.h>
#include <iostream>
#define ReportLiveObjects 0

#define CLEAN(x) if(x){x->Release();x=nullptr;}

namespace Themp
{
	struct Timer
	{
	public:
		Timer()
		{
			Init();
		}
		void Init()
		{
			QueryPerformanceFrequency(&freq);
			QueryPerformanceCounter(&oldT);
			QueryPerformanceCounter(&newT);
		}
		void StartTime()
		{
			QueryPerformanceCounter(&newT);
		}
		//microseconds: Retrieves the delta time since StartTime or Init was called.
		double GetDeltaTimeMicro()
		{
			LARGE_INTEGER temp;
			QueryPerformanceCounter(&temp);
			return  static_cast<double>((temp.QuadPart - oldT.QuadPart) / (freq.QuadPart / 1000000));
		}
		//milliseconds: Retrieves the delta time since StartTime or Init was called.
		double GetDeltaTimeMS()
		{
			LARGE_INTEGER temp;
			QueryPerformanceCounter(&temp);
			return static_cast<double>((temp.QuadPart - oldT.QuadPart) / (freq.QuadPart / 1000));
		}
		//microseconds: Resets the timer to the current time, Repeatedly calling this the same frame will yield low to 0 values
		double GetDeltaTimeMicroReset()
		{
			QueryPerformanceCounter(&newT);
			double delta = static_cast<double>((newT.QuadPart - oldT.QuadPart) / (freq.QuadPart / 1000000));
			oldT = newT;
			return delta;
		}
		//milliseconds: Resets the timer to the current time, Repeatedly calling this the same frame will yield low to 0 values
		double GetDeltaTimeMSReset()
		{
			QueryPerformanceCounter(&newT);
			double delta = static_cast<double>((newT.QuadPart - oldT.QuadPart) / (freq.QuadPart / 1000));
			oldT = newT;
			return delta;
		}
		//Seconds (Microsecond precision): Resets the timer to the current time, Repeatedly calling this the same frame will yield low to 0 values
		double GetDeltaTimeReset()
		{
			return  static_cast<double>(GetDeltaTimeMicroReset() / 1000000.0);
		}
		//Seconds (Microsecond precision): Retrieves the delta time since StartTime or Init was called.
		double GetDeltaTime()
		{
			return  static_cast<double>(GetDeltaTimeMicro() / 1000000.0);
		}
	private:
		LARGE_INTEGER oldT,newT;
		LARGE_INTEGER freq;
	};
	float lerp(float x, float y, float t);
	class Game;
	class D3D;
	class Resources;
	class GUI;
	class System
	{
	public:
		static FILE* logFile;
		static void Print(const char* message, ...);

		static Themp::System* tSys;
		System() {}; 
		void Start();
		void Interrupt() {}; // Alt tab, lost focus etc...

		HWND m_Window = nullptr;
		bool m_Quitting = false;
		bool m_CursorShown = true;
		std::map<std::string, float> m_SVars;
		Themp::Game* m_Game;
		Themp::D3D* m_D3D;
		Themp::Resources* m_Resources;
		Themp::GUI* m_GUI;
	};
};