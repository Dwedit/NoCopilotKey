#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdint.h>
typedef uint32_t u32;
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#define DEBUG _DEBUG

#if DEBUG

#include <queue>
#include <string>
#include <atomic>
#include <mutex>
std::queue<std::string> debugStringQueue;
std::mutex debugMutex;
HANDLE debugEvent;
std::atomic_bool debugQuit;

DWORD WINAPI SecondaryThreadMain(void* unused)
{
	while (!debugQuit)
	{
		WaitForSingleObject(debugEvent, INFINITE);
		while (true)
		{
			std::string str;
			{
				std::lock_guard<std::mutex> lockGuard(debugMutex);
				if (!debugStringQueue.empty())
				{
					str = debugStringQueue.front();
					debugStringQueue.pop();
				}
			}
			if (str.length() > 0)
			{
				fwrite(str.c_str(), 1, str.length(), stdout);
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

#endif


extern "C"
{
	//HMODULE sasModule;
	//typedef VOID (WINAPI *SendSAS_Func)(BOOL);
	//SendSAS_Func SendSAS;
	
	void ReplaySuppressedKeys();
	
	enum STATE
	{
		Idle = 0,
		LeftWindows,
		LeftShift,
		F23,
	};
	STATE pressState;
	STATE releaseState;

	bool leftWindowsSuppressed;
	bool leftShiftSuppressed;
	//bool f23Suppressed;
	bool rightCtrlDown;

	//bool leftCtrlDown, rightCtrlDown, leftAltDown, rightAltDown;

	HWND mainWindow;

	int APIENTRY MyWinMain();

	LRESULT CALLBACK MyKeyboardProc(int code, WPARAM wParam, LPARAM lParam);
	void DebugPrintf(const char* format, ...);

#if DEBUG
	const char* GetScancodeName(int key);
	void RegisterScanCodes();

	int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		freopen("CON", "w", stdout);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS);
		RegisterScanCodes();
		debugEvent = CreateEvent(NULL, false, false, NULL);
		HANDLE hThread = CreateThread(NULL, 0, SecondaryThreadMain, 0, NULL, NULL);
		int exitCode = MyWinMain();
		ExitProcess(exitCode);
	}

	void DebugPrintf2(const char* msg)
	{
		{
			std::lock_guard<std::mutex> myLock(debugMutex);
			debugStringQueue.emplace(std::string(msg));
		}
		SetEvent(debugEvent);
	}

	void DebugPrintf(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		char buffer[256];
		vsprintf_s(buffer, format, args);
		DebugPrintf2(buffer);
		va_end(args);
	}

#endif
	//extern void TimerProc(MSG& msg);
	int APIENTRY MyWinMain()
	{
		//sasModule = LoadLibraryA("sas.dll");
		//if (sasModule != NULL)
		//{
		//	SendSAS = (SendSAS_Func)GetProcAddress(sasModule, "SendSAS");
		//}

		HANDLE mutex = OpenMutexA(SYNCHRONIZE, false, "Mutex for NoCopilotKey");
		if (mutex == NULL)
		{
			mutex = CreateMutexA(NULL, true, "Mutex for NoCopilotKey");
		}
		else
		{
			return -1;
		}
		HMODULE module = GetModuleHandleW(NULL);

		HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, &MyKeyboardProc, module, 0);
		int lastError = GetLastError();

		mainWindow = CreateWindowExW(0, L"STATIC", L"NoCopilotKey", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			//TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return msg.wParam;
	}

	UINT_PTR activeTimer = 0;
	void SetPressState(STATE state)
	{
#if DEBUG
		DebugPrintf("Press State Transition: %d -> %d\n", pressState, state);
#endif
		pressState = state;
	}

	void SetReleaseState(STATE state)
	{
#if DEBUG
		DebugPrintf("Release State Transition: %d -> %d\n", releaseState, state);
#endif
		releaseState = state;
	}

	void CancelTimer();

	void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
#if DEBUG
		DebugPrintf("In TimerProc because it took too long to see all three keys\n");
#endif
		CancelTimer();
		ReplaySuppressedKeys();
	}

	void EnsureTimer()
	{
		if (activeTimer == 0)
		{
			activeTimer = SetTimer(mainWindow, 1, 100, TimerProc);
		}
	}
	void CancelTimer()
	{
		if (activeTimer != 0)
		{
			KillTimer(mainWindow, activeTimer);
			activeTimer = 0;
#if DEBUG
			DebugPrintf("Timer cancelled\n");
#endif	
		}
	}

	void InjectRightControlPress()
	{
		INPUT inputArr[1];
		inputArr[0].type = INPUT_KEYBOARD;
		inputArr[0].ki.wVk = VK_RCONTROL;
		inputArr[0].ki.wScan = 0;
		inputArr[0].ki.dwFlags = 0;
		inputArr[0].ki.time = 0;
		inputArr[0].ki.dwExtraInfo = 0;

		SendInput(1, &inputArr[0], sizeof(INPUT));
	}
	void InjectRightControlRelease()
	{
		INPUT inputArr[1];
		inputArr[0].type = INPUT_KEYBOARD;
		inputArr[0].ki.wVk = VK_RCONTROL;
		inputArr[0].ki.wScan = 0;
		inputArr[0].ki.dwFlags = KEYEVENTF_KEYUP;
		inputArr[0].ki.time = 0;
		inputArr[0].ki.dwExtraInfo = 0;

		SendInput(1, &inputArr[0], sizeof(INPUT));
	}


	void ReplaySuppressedKeys()
	{
		INPUT inputArr[2];
		int count = 0;
		if (leftWindowsSuppressed)
		{
			leftWindowsSuppressed = false;
			inputArr[count].type = INPUT_KEYBOARD;
			inputArr[count].ki.wVk = VK_LWIN;
			inputArr[count].ki.wScan = 0;
			inputArr[count].ki.dwFlags = 0;
			inputArr[count].ki.time = 0;
			inputArr[count].ki.dwExtraInfo = 0;
			count++;
		}
		if (leftShiftSuppressed)
		{
			leftShiftSuppressed = false;
			inputArr[count].type = INPUT_KEYBOARD;
			inputArr[count].ki.wVk = VK_LSHIFT;
			inputArr[count].ki.wScan = 0;
			inputArr[count].ki.dwFlags = 0;
			inputArr[count].ki.time = 0;
			inputArr[count].ki.dwExtraInfo = 0;
			count++;
		}
		if (count > 0)
		{
			SendInput(count, &inputArr[0], sizeof(INPUT));
			SetPressState(STATE::Idle);
		}
	}
	void ReplaySuppressedKeys2()
	{
		if (leftWindowsSuppressed || leftShiftSuppressed)
		{
			ReplaySuppressedKeys();
		}
		CancelTimer();
	}

	bool lastWasRepeated = false;

	LRESULT CALLBACK MyKeyboardProc2(int code, WPARAM wParam, LPARAM lParam)
	{
		//cooperate with other programs which use low level keyboard hooks
		if (code < 0)
		{
			return code;
		}

		int keyCode;
		int flags;
		KBDLLHOOKSTRUCT* hookStruct = (KBDLLHOOKSTRUCT*)lParam;
		flags = hookStruct->flags;
		keyCode = hookStruct->vkCode;
		bool injected = 0 != (flags & LLKHF_INJECTED);
		bool released = 0 != (flags & (1 << 7));
		bool pressed = !released;

		if (injected)
		{
			return 0;
		}
		if (pressed)
		{
			if (keyCode == VK_LWIN)
			{
				ReplaySuppressedKeys2();
				leftWindowsSuppressed = true;
				SetPressState(STATE::LeftWindows);
				EnsureTimer();
				return 1;  //block key
			}

			if (pressState == STATE::LeftWindows)
			{
				if (keyCode == VK_LSHIFT)
				{
					leftShiftSuppressed = true;
					SetPressState(STATE::LeftShift);
					return 1;  //block key
				}
				else
				{
					ReplaySuppressedKeys2();
				}
			}
			else if (pressState == STATE::LeftShift)
			{
				if (keyCode == VK_F23)
				{
					SetPressState(STATE::Idle);
					SetReleaseState(STATE::F23);
					CancelTimer();
					leftShiftSuppressed = false;
					leftWindowsSuppressed = false;
					if (!rightCtrlDown)
					{
						InjectRightControlPress();
					}
					return 1;  //block key
				}
				else
				{
					ReplaySuppressedKeys2();
				}
			}
		}
		else if (released)
		{
			if (pressState != STATE::Idle)
			{
				ReplaySuppressedKeys2();
			}
			if (keyCode == VK_F23 && releaseState == STATE::F23)
			{
				SetReleaseState(STATE::LeftShift);
				InjectRightControlRelease();
				return 1;  //block key
			}
			if (keyCode == VK_LSHIFT && releaseState == STATE::LeftShift)
			{
				SetReleaseState(STATE::LeftWindows);
				return 1;  //block key
			}
			if (keyCode == VK_LWIN && releaseState == STATE::LeftWindows)
			{
				SetReleaseState(STATE::Idle);
				return 1;  //block key
			}
		}

		return 0;
	}

	LRESULT CALLBACK MyKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
	{
		int keyCode;
		int flags;
		KBDLLHOOKSTRUCT* hookStruct = (KBDLLHOOKSTRUCT*)lParam;
		flags = hookStruct->flags;
		keyCode = hookStruct->vkCode;
		bool injected = 0 != (flags & LLKHF_INJECTED);
		bool released = 0 != (flags & (1 << 7));
		bool pressed = !released;
		bool handled = false;

		LRESULT result = MyKeyboardProc2(code, wParam, lParam);
		if (result < 0)
		{
			return CallNextHookEx(NULL, code, wParam, lParam);
		}

#if DEBUG
		if (keyCode > 0)
		{
			const char* suppressedMessage = "";
			if (result > 0)
			{
				suppressedMessage = "SUPPRESSED ";
			}
			const char* injectedMessage = " Real KB ";
			if (injected)
			{
				injectedMessage = "INJECTED ";
			}
			const char* pressedMessage = "  PRESS ";
			if (released)
			{
				pressedMessage = "RELEASE ";
			}
			DebugPrintf("%d %s%s%s0x%02X %s\n", GetTickCount(), suppressedMessage, injectedMessage, pressedMessage, keyCode, GetScancodeName(keyCode));
		}
#endif
		if (result <= 0)
		{
			if (pressed)
			{
				//if (keyCode == VK_LMENU)
				//{
				//	leftAltDown = true;
				//}
				//if (keyCode == VK_RMENU)
				//{
				//	rightAltDown = true;
				//}
				//if (keyCode == VK_LCONTROL)
				//{
				//	leftCtrlDown = true;
				//}
				if (keyCode == VK_RCONTROL)
				{
					rightCtrlDown = true;
				}
				//if (keyCode == VK_DELETE)
				//{
				//	if ((leftAltDown || rightAltDown) && (leftCtrlDown || rightCtrlDown))
				//	{
				//		#if DEBUG
				//		DebugPrintf("SendSAS\n");
				//		#endif
				//		if (SendSAS != NULL) SendSAS(true);
				//	}
				//}
			}
			else
			{
				//if (keyCode == VK_LMENU)
				//{
				//	leftAltDown = false;
				//}
				//if (keyCode == VK_RMENU)
				//{
				//	rightAltDown = false;
				//}
				//if (keyCode == VK_LCONTROL)
				//{
				//	leftCtrlDown = false;
				//}
				if (keyCode == VK_RCONTROL)
				{
					rightCtrlDown = false;
				}
			}
		}
		if (result <= 0)
		{
			return CallNextHookEx(NULL, code, wParam, lParam);
		}
		return result;
	}

#if DEBUG
	const char *scancodeList[256];

	void RegisterScanCode(const char* keyName, int keyCode)
	{
		if (keyCode >= 0 && keyCode < 256)
		{
			scancodeList[keyCode] = keyName;
		}
	}

	void RegisterScanCodes()
	{
		for (int i = 0; i < 256; i++)
		{
			RegisterScanCode("", i);
		}
		RegisterScanCode("A", 'A');
		RegisterScanCode("B", 'B');
		RegisterScanCode("C", 'C');
		RegisterScanCode("D", 'D');
		RegisterScanCode("E", 'E');
		RegisterScanCode("F", 'F');
		RegisterScanCode("G", 'G');
		RegisterScanCode("H", 'H');
		RegisterScanCode("I", 'I');
		RegisterScanCode("J", 'J');
		RegisterScanCode("K", 'K');
		RegisterScanCode("L", 'L');
		RegisterScanCode("M", 'M');
		RegisterScanCode("N", 'N');
		RegisterScanCode("O", 'O');
		RegisterScanCode("P", 'P');
		RegisterScanCode("Q", 'Q');
		RegisterScanCode("R", 'R');
		RegisterScanCode("S", 'S');
		RegisterScanCode("T", 'T');
		RegisterScanCode("U", 'U');
		RegisterScanCode("V", 'V');
		RegisterScanCode("W", 'W');
		RegisterScanCode("X", 'X');
		RegisterScanCode("Y", 'Y');
		RegisterScanCode("Z", 'Z');

		RegisterScanCode("0", '0');
		RegisterScanCode("1", '1');
		RegisterScanCode("2", '2');
		RegisterScanCode("3", '3');
		RegisterScanCode("4", '4');
		RegisterScanCode("5", '5');
		RegisterScanCode("6", '6');
		RegisterScanCode("7", '7');
		RegisterScanCode("8", '8');
		RegisterScanCode("9", '9');

		RegisterScanCode("VK_LBUTTON", 0x01);
		RegisterScanCode("VK_RBUTTON", 0x02);
		RegisterScanCode("VK_CANCEL", 0x03);
		RegisterScanCode("VK_MBUTTON", 0x04);
		RegisterScanCode("VK_XBUTTON1", 0x05);
		RegisterScanCode("VK_XBUTTON2", 0x06);
		RegisterScanCode("VK_BACK", 0x08);
		RegisterScanCode("VK_TAB", 0x09);
		RegisterScanCode("VK_CLEAR", 0x0C);
		RegisterScanCode("VK_RETURN", 0x0D);
		RegisterScanCode("VK_SHIFT", 0x10);
		RegisterScanCode("VK_CONTROL", 0x11);
		RegisterScanCode("VK_MENU", 0x12);
		RegisterScanCode("VK_PAUSE", 0x13);
		RegisterScanCode("VK_CAPITAL", 0x14);
		RegisterScanCode("VK_KANA", 0x15);
		RegisterScanCode("VK_IME_ON", 0x16);
		RegisterScanCode("VK_JUNJA", 0x17);
		RegisterScanCode("VK_FINAL", 0x18);
		RegisterScanCode("VK_KANJI", 0x19);
		RegisterScanCode("VK_IME_OFF", 0x1A);
		RegisterScanCode("VK_ESCAPE", 0x1B);
		RegisterScanCode("VK_CONVERT", 0x1C);
		RegisterScanCode("VK_NONCONVERT", 0x1D);
		RegisterScanCode("VK_ACCEPT", 0x1E);
		RegisterScanCode("VK_MODECHANGE", 0x1F);
		RegisterScanCode("VK_SPACE", 0x20);
		RegisterScanCode("VK_PRIOR", 0x21);
		RegisterScanCode("VK_NEXT", 0x22);
		RegisterScanCode("VK_END", 0x23);
		RegisterScanCode("VK_HOME", 0x24);
		RegisterScanCode("VK_LEFT", 0x25);
		RegisterScanCode("VK_UP", 0x26);
		RegisterScanCode("VK_RIGHT", 0x27);
		RegisterScanCode("VK_DOWN", 0x28);
		RegisterScanCode("VK_SELECT", 0x29);
		RegisterScanCode("VK_PRINT", 0x2A);
		RegisterScanCode("VK_EXECUTE", 0x2B);
		RegisterScanCode("VK_SNAPSHOT", 0x2C);
		RegisterScanCode("VK_INSERT", 0x2D);
		RegisterScanCode("VK_DELETE", 0x2E);
		RegisterScanCode("VK_HELP", 0x2F);
		RegisterScanCode("VK_LWIN", 0x5B);
		RegisterScanCode("VK_RWIN", 0x5C);
		RegisterScanCode("VK_APPS", 0x5D);
		RegisterScanCode("VK_SLEEP", 0x5F);
		RegisterScanCode("VK_NUMPAD0", 0x60);
		RegisterScanCode("VK_NUMPAD1", 0x61);
		RegisterScanCode("VK_NUMPAD2", 0x62);
		RegisterScanCode("VK_NUMPAD3", 0x63);
		RegisterScanCode("VK_NUMPAD4", 0x64);
		RegisterScanCode("VK_NUMPAD5", 0x65);
		RegisterScanCode("VK_NUMPAD6", 0x66);
		RegisterScanCode("VK_NUMPAD7", 0x67);
		RegisterScanCode("VK_NUMPAD8", 0x68);
		RegisterScanCode("VK_NUMPAD9", 0x69);
		RegisterScanCode("VK_MULTIPLY", 0x6A);
		RegisterScanCode("VK_ADD", 0x6B);
		RegisterScanCode("VK_SEPARATOR", 0x6C);
		RegisterScanCode("VK_SUBTRACT", 0x6D);
		RegisterScanCode("VK_DECIMAL", 0x6E);
		RegisterScanCode("VK_DIVIDE", 0x6F);
		RegisterScanCode("VK_F1", 0x70);
		RegisterScanCode("VK_F2", 0x71);
		RegisterScanCode("VK_F3", 0x72);
		RegisterScanCode("VK_F4", 0x73);
		RegisterScanCode("VK_F5", 0x74);
		RegisterScanCode("VK_F6", 0x75);
		RegisterScanCode("VK_F7", 0x76);
		RegisterScanCode("VK_F8", 0x77);
		RegisterScanCode("VK_F9", 0x78);
		RegisterScanCode("VK_F10", 0x79);
		RegisterScanCode("VK_F11", 0x7A);
		RegisterScanCode("VK_F12", 0x7B);
		RegisterScanCode("VK_F13", 0x7C);
		RegisterScanCode("VK_F14", 0x7D);
		RegisterScanCode("VK_F15", 0x7E);
		RegisterScanCode("VK_F16", 0x7F);
		RegisterScanCode("VK_F17", 0x80);
		RegisterScanCode("VK_F18", 0x81);
		RegisterScanCode("VK_F19", 0x82);
		RegisterScanCode("VK_F20", 0x83);
		RegisterScanCode("VK_F21", 0x84);
		RegisterScanCode("VK_F22", 0x85);
		RegisterScanCode("VK_F23", 0x86);
		RegisterScanCode("VK_F24", 0x87);
		RegisterScanCode("VK_NAVIGATION_VIEW", 0x88);
		RegisterScanCode("VK_NAVIGATION_MENU", 0x89);
		RegisterScanCode("VK_NAVIGATION_UP", 0x8A);
		RegisterScanCode("VK_NAVIGATION_DOWN", 0x8B);
		RegisterScanCode("VK_NAVIGATION_LEFT", 0x8C);
		RegisterScanCode("VK_NAVIGATION_RIGHT", 0x8D);
		RegisterScanCode("VK_NAVIGATION_ACCEPT", 0x8E);
		RegisterScanCode("VK_NAVIGATION_CANCEL", 0x8F);
		RegisterScanCode("VK_NUMLOCK", 0x90);
		RegisterScanCode("VK_SCROLL", 0x91);
		RegisterScanCode("VK_OEM_FJ_JISHO", 0x92);
		RegisterScanCode("VK_OEM_FJ_MASSHOU", 0x93);
		RegisterScanCode("VK_OEM_FJ_TOUROKU", 0x94);
		RegisterScanCode("VK_OEM_FJ_LOYA", 0x95);
		RegisterScanCode("VK_OEM_FJ_ROYA", 0x96);
		RegisterScanCode("VK_LSHIFT", 0xA0);
		RegisterScanCode("VK_RSHIFT", 0xA1);
		RegisterScanCode("VK_LCONTROL", 0xA2);
		RegisterScanCode("VK_RCONTROL", 0xA3);
		RegisterScanCode("VK_LMENU", 0xA4);
		RegisterScanCode("VK_RMENU", 0xA5);
		RegisterScanCode("VK_BROWSER_BACK", 0xA6);
		RegisterScanCode("VK_BROWSER_FORWARD", 0xA7);
		RegisterScanCode("VK_BROWSER_REFRESH", 0xA8);
		RegisterScanCode("VK_BROWSER_STOP", 0xA9);
		RegisterScanCode("VK_BROWSER_SEARCH", 0xAA);
		RegisterScanCode("VK_BROWSER_FAVORITES", 0xAB);
		RegisterScanCode("VK_BROWSER_HOME", 0xAC);
		RegisterScanCode("VK_VOLUME_MUTE", 0xAD);
		RegisterScanCode("VK_VOLUME_DOWN", 0xAE);
		RegisterScanCode("VK_VOLUME_UP", 0xAF);
		RegisterScanCode("VK_MEDIA_NEXT_TRACK", 0xB0);
		RegisterScanCode("VK_MEDIA_PREV_TRACK", 0xB1);
		RegisterScanCode("VK_MEDIA_STOP", 0xB2);
		RegisterScanCode("VK_MEDIA_PLAY_PAUSE", 0xB3);
		RegisterScanCode("VK_LAUNCH_MAIL", 0xB4);
		RegisterScanCode("VK_LAUNCH_MEDIA_SELECT", 0xB5);
		RegisterScanCode("VK_LAUNCH_APP1", 0xB6);
		RegisterScanCode("VK_LAUNCH_APP2", 0xB7);
		RegisterScanCode("VK_OEM_1", 0xBA);
		RegisterScanCode("VK_OEM_PLUS", 0xBB);
		RegisterScanCode("VK_OEM_COMMA", 0xBC);
		RegisterScanCode("VK_OEM_MINUS", 0xBD);
		RegisterScanCode("VK_OEM_PERIOD", 0xBE);
		RegisterScanCode("VK_OEM_2", 0xBF);
		RegisterScanCode("VK_OEM_3", 0xC0);
		RegisterScanCode("VK_GAMEPAD_A", 0xC3);
		RegisterScanCode("VK_GAMEPAD_B", 0xC4);
		RegisterScanCode("VK_GAMEPAD_X", 0xC5);
		RegisterScanCode("VK_GAMEPAD_Y", 0xC6);
		RegisterScanCode("VK_GAMEPAD_RIGHT_SHOULDER", 0xC7);
		RegisterScanCode("VK_GAMEPAD_LEFT_SHOULDER", 0xC8);
		RegisterScanCode("VK_GAMEPAD_LEFT_TRIGGER", 0xC9);
		RegisterScanCode("VK_GAMEPAD_RIGHT_TRIGGER", 0xCA);
		RegisterScanCode("VK_GAMEPAD_DPAD_UP", 0xCB);
		RegisterScanCode("VK_GAMEPAD_DPAD_DOWN", 0xCC);
		RegisterScanCode("VK_GAMEPAD_DPAD_LEFT", 0xCD);
		RegisterScanCode("VK_GAMEPAD_DPAD_RIGHT", 0xCE);
		RegisterScanCode("VK_GAMEPAD_MENU", 0xCF);
		RegisterScanCode("VK_GAMEPAD_VIEW", 0xD0);
		RegisterScanCode("VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON", 0xD1);
		RegisterScanCode("VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON", 0xD2);
		RegisterScanCode("VK_GAMEPAD_LEFT_THUMBSTICK_UP", 0xD3);
		RegisterScanCode("VK_GAMEPAD_LEFT_THUMBSTICK_DOWN", 0xD4);
		RegisterScanCode("VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT", 0xD5);
		RegisterScanCode("VK_GAMEPAD_LEFT_THUMBSTICK_LEFT", 0xD6);
		RegisterScanCode("VK_GAMEPAD_RIGHT_THUMBSTICK_UP", 0xD7);
		RegisterScanCode("VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN", 0xD8);
		RegisterScanCode("VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT", 0xD9);
		RegisterScanCode("VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT", 0xDA);
		RegisterScanCode("VK_OEM_4", 0xDB);
		RegisterScanCode("VK_OEM_5", 0xDC);
		RegisterScanCode("VK_OEM_6", 0xDD);
		RegisterScanCode("VK_OEM_7", 0xDE);
		RegisterScanCode("VK_OEM_8", 0xDF);
		RegisterScanCode("VK_OEM_AX", 0xE1);
		RegisterScanCode("VK_OEM_102", 0xE2);
		RegisterScanCode("VK_ICO_HELP", 0xE3);
		RegisterScanCode("VK_ICO_00", 0xE4);
		RegisterScanCode("VK_PROCESSKEY", 0xE5);
		RegisterScanCode("VK_ICO_CLEAR", 0xE6);
		RegisterScanCode("VK_PACKET", 0xE7);
		RegisterScanCode("VK_OEM_RESET", 0xE9);
		RegisterScanCode("VK_OEM_JUMP", 0xEA);
		RegisterScanCode("VK_OEM_PA1", 0xEB);
		RegisterScanCode("VK_OEM_PA2", 0xEC);
		RegisterScanCode("VK_OEM_PA3", 0xED);
		RegisterScanCode("VK_OEM_WSCTRL", 0xEE);
		RegisterScanCode("VK_OEM_CUSEL", 0xEF);
		RegisterScanCode("VK_OEM_ATTN", 0xF0);
		RegisterScanCode("VK_OEM_FINISH", 0xF1);
		RegisterScanCode("VK_OEM_COPY", 0xF2);
		RegisterScanCode("VK_OEM_AUTO", 0xF3);
		RegisterScanCode("VK_OEM_ENLW", 0xF4);
		RegisterScanCode("VK_OEM_BACKTAB", 0xF5);
		RegisterScanCode("VK_ATTN", 0xF6);
		RegisterScanCode("VK_CRSEL", 0xF7);
		RegisterScanCode("VK_EXSEL", 0xF8);
		RegisterScanCode("VK_EREOF", 0xF9);
		RegisterScanCode("VK_PLAY", 0xFA);
		RegisterScanCode("VK_ZOOM", 0xFB);
		RegisterScanCode("VK_NONAME", 0xFC);
		RegisterScanCode("VK_PA1", 0xFD);
		RegisterScanCode("VK_OEM_CLEAR", 0xFE);
	}

	const char* GetScancodeName(int key)
	{
		return scancodeList[key & 255];
	}

#endif
}
