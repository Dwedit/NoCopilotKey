#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdint.h>
typedef uint32_t u32;
#include <windows.h>
#include <shellapi.h>
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

DWORD WINAPI DebugThreadMain(void* unused)
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


EXTERN_C_START

HMODULE sasModule;
typedef VOID (WINAPI *SendSAS_Func)(BOOL);
SendSAS_Func SendSAS;
	
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
LPWSTR commandLine;
int argc;
PWSTR* argv;

bool leftWindowsSuppressed;
bool leftShiftSuppressed;
//bool f23Suppressed;
bool rightCtrlDown;

bool leftCtrlDown, leftAltDown, rightAltDown;

HWND mainWindow;

int APIENTRY MyWinMain();

LRESULT CALLBACK MyKeyboardProc(int code, WPARAM wParam, LPARAM lParam);

#if DEBUG
const char* GetVKeyName(int key);
void RegisterVKeyCodes();
void DebugPrintf(const char* format, ...);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS);
	RegisterVKeyCodes();
	debugEvent = CreateEvent(NULL, false, false, NULL);
	HANDLE hThread = CreateThread(NULL, 0, DebugThreadMain, 0, NULL, NULL);
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
	commandLine = GetCommandLineW();
	#if DEBUG
	wprintf(L"Command Line: %s\n", commandLine);
	#endif

	argv = CommandLineToArgvW(commandLine, &argc);
	#if DEBUG
	wprintf(L"argv[0]: %s\n", argv[0]);
	#endif

	HANDLE mutex = OpenMutexA(SYNCHRONIZE, false, "Mutex for NoCopilotKey");
	if (mutex == NULL)
	{
		mutex = CreateMutexA(NULL, true, "Mutex for NoCopilotKey");
	}
	else
	{
		return -1;
	}

	sasModule = LoadLibraryA("sas.dll");
	if (sasModule != NULL)
	{
		SendSAS = (SendSAS_Func)GetProcAddress(sasModule, "SendSAS");
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

void SetPressState(STATE state)
{
	#if DEBUG
	if (pressState != state)
	{
		DebugPrintf("Press State Transition: %d -> %d\n", pressState, state);
	}
	#endif
	pressState = state;
}

void SetReleaseState(STATE state)
{
	#if DEBUG
	if (releaseState != state)
	{
		DebugPrintf("Release State Transition: %d -> %d\n", releaseState, state);
	}
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

UINT_PTR activeTimer = 0;
void EnsureTimer()
{
	if (activeTimer == 0)
	{
		activeTimer = SetTimer(mainWindow, 1, 20, TimerProc);
		#if DEBUG
		DebugPrintf("Timer set\n");
		#endif	
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

void SetKeyDown(INPUT* input, DWORD VKEY)
{
	input->type = INPUT_KEYBOARD;
	input->ki.wVk = VKEY;
	input->ki.wScan = 0;
	input->ki.dwFlags = 0;
	input->ki.time = 0;
	input->ki.dwExtraInfo = 0;
}

void SetKeyUp(INPUT* input, DWORD VKEY)
{
	SetKeyDown(input, VKEY);
	input->ki.dwFlags = KEYEVENTF_KEYUP;
}

void InjectKeyUp(DWORD VKEY)
{
	INPUT input;
	SetKeyUp(&input, VKEY);
	SendInput(1, &input, sizeof(INPUT));
}

void InjectKeyDown(DWORD VKEY)
{
	INPUT input;
	SetKeyDown(&input, VKEY);
	SendInput(1, &input, sizeof(INPUT));
}

void ReplaySuppressedKeys()
{
	if (leftWindowsSuppressed)
	{
		leftWindowsSuppressed = false;
		InjectKeyDown(VK_LWIN);
	}
	if (leftShiftSuppressed)
	{
		leftShiftSuppressed = false;
		InjectKeyDown(VK_LSHIFT);
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
		return CallNextHookEx(NULL, code, wParam, lParam);
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
		return CallNextHookEx(NULL, code, wParam, lParam);
	}
	if (pressed)
	{
		if (keyCode == VK_LWIN)
		{
			ReplaySuppressedKeys2();
			leftWindowsSuppressed = true;
			SetPressState(STATE::LeftWindows);
			EnsureTimer();
			return -1;  //block key
		}

		if (pressState == STATE::LeftWindows)
		{
			if (keyCode == VK_LSHIFT)
			{
				leftShiftSuppressed = true;
				SetPressState(STATE::LeftShift);
				return -1;  //block key
			}
			else
			{
				ReplaySuppressedKeys2();
				SetPressState(STATE::Idle);
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
					InjectKeyDown(VK_RCONTROL);
				}
				return -1;  //block key
			}
			else
			{
				ReplaySuppressedKeys2();
				SetPressState(STATE::Idle);
			}
		}
	}
	else if (released)
	{
		if (pressState != STATE::Idle)
		{
			bool leftWindowsWasSuppressed = leftWindowsSuppressed;
			bool leftShiftWasSuppressed = leftShiftSuppressed;
			ReplaySuppressedKeys2();
			SetPressState(STATE::Idle);
			//Game Bar is weird, you need to inject a key up event and suppress the real key up
			//otherwise Game Bar sees the injected Key Down after the real Key Up.
			if (leftWindowsWasSuppressed && keyCode == VK_LWIN)
			{
				InjectKeyUp(VK_LWIN);
				return -1;
			}
			if (leftShiftWasSuppressed && keyCode == VK_LSHIFT)
			{
				InjectKeyUp(VK_LSHIFT);
				return -1;
			}
		}
		if (keyCode == VK_F23 && releaseState == STATE::F23)
		{
			SetReleaseState(STATE::LeftShift);
			InjectKeyUp(VK_RCONTROL);
			return -1;  //block key
		}
		if (keyCode == VK_LSHIFT && releaseState == STATE::LeftShift)
		{
			SetReleaseState(STATE::LeftWindows);
			return -1;  //block key
		}
		if (keyCode == VK_LWIN && releaseState == STATE::LeftWindows)
		{
			SetReleaseState(STATE::Idle);
			return -1;  //block key
		}
	}

	return CallNextHookEx(NULL, code, wParam, lParam);
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

	#if DEBUG
	DWORD arrivalTime = GetTickCount();
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
	if (keyCode > 0)
	{
		DebugPrintf("%d %s%s0x%02X %s\n", arrivalTime, injectedMessage, pressedMessage, keyCode, GetVKeyName(keyCode));
	}
	#endif
	LRESULT result = MyKeyboardProc2(code, wParam, lParam);
	#if DEBUG
	if (keyCode > 0 && result != 0)
	{
		DebugPrintf("%d %s%s0x%02X %s was suppressed\n", arrivalTime, injectedMessage, pressedMessage, keyCode, GetVKeyName(keyCode));
	}
	#endif			
	if (result == 0)
	{
		if (pressed)
		{
			if (keyCode == VK_LMENU)
			{
				leftAltDown = true;
			}
			if (keyCode == VK_RMENU)
			{
				rightAltDown = true;
			}
			if (keyCode == VK_LCONTROL)
			{
				leftCtrlDown = true;
			}
			if (keyCode == VK_RCONTROL)
			{
				rightCtrlDown = true;
			}
			if (keyCode == VK_DELETE)
			{
				if ((leftAltDown || rightAltDown) && (leftCtrlDown || rightCtrlDown))
				{
					#if DEBUG
					DebugPrintf("SendSAS\n");
					#endif
					if (SendSAS != NULL) SendSAS(true);
				}
			}
		}
		else
		{
			if (keyCode == VK_LMENU)
			{
				leftAltDown = false;
			}
			if (keyCode == VK_RMENU)
			{
				rightAltDown = false;
			}
			if (keyCode == VK_LCONTROL)
			{
				leftCtrlDown = false;
			}
			if (keyCode == VK_RCONTROL)
			{
				rightCtrlDown = false;
			}
		}
	}
	return result;
}

#if DEBUG
const char *vkeyList[256];

void RegisterVKeyCode(const char* keyName, int keyCode)
{
	if (keyCode >= 0 && keyCode < 256)
	{
		vkeyList[keyCode] = keyName;
	}
}

void RegisterVKeyCodes()
{
	for (int i = 0; i < 256; i++)
	{
		RegisterVKeyCode("", i);
	}
	RegisterVKeyCode("A", 'A');
	RegisterVKeyCode("B", 'B');
	RegisterVKeyCode("C", 'C');
	RegisterVKeyCode("D", 'D');
	RegisterVKeyCode("E", 'E');
	RegisterVKeyCode("F", 'F');
	RegisterVKeyCode("G", 'G');
	RegisterVKeyCode("H", 'H');
	RegisterVKeyCode("I", 'I');
	RegisterVKeyCode("J", 'J');
	RegisterVKeyCode("K", 'K');
	RegisterVKeyCode("L", 'L');
	RegisterVKeyCode("M", 'M');
	RegisterVKeyCode("N", 'N');
	RegisterVKeyCode("O", 'O');
	RegisterVKeyCode("P", 'P');
	RegisterVKeyCode("Q", 'Q');
	RegisterVKeyCode("R", 'R');
	RegisterVKeyCode("S", 'S');
	RegisterVKeyCode("T", 'T');
	RegisterVKeyCode("U", 'U');
	RegisterVKeyCode("V", 'V');
	RegisterVKeyCode("W", 'W');
	RegisterVKeyCode("X", 'X');
	RegisterVKeyCode("Y", 'Y');
	RegisterVKeyCode("Z", 'Z');

	RegisterVKeyCode("0", '0');
	RegisterVKeyCode("1", '1');
	RegisterVKeyCode("2", '2');
	RegisterVKeyCode("3", '3');
	RegisterVKeyCode("4", '4');
	RegisterVKeyCode("5", '5');
	RegisterVKeyCode("6", '6');
	RegisterVKeyCode("7", '7');
	RegisterVKeyCode("8", '8');
	RegisterVKeyCode("9", '9');

	RegisterVKeyCode("VK_LBUTTON", 0x01);
	RegisterVKeyCode("VK_RBUTTON", 0x02);
	RegisterVKeyCode("VK_CANCEL", 0x03);
	RegisterVKeyCode("VK_MBUTTON", 0x04);
	RegisterVKeyCode("VK_XBUTTON1", 0x05);
	RegisterVKeyCode("VK_XBUTTON2", 0x06);
	RegisterVKeyCode("VK_BACK", 0x08);
	RegisterVKeyCode("VK_TAB", 0x09);
	RegisterVKeyCode("VK_CLEAR", 0x0C);
	RegisterVKeyCode("VK_RETURN", 0x0D);
	RegisterVKeyCode("VK_SHIFT", 0x10);
	RegisterVKeyCode("VK_CONTROL", 0x11);
	RegisterVKeyCode("VK_MENU", 0x12);
	RegisterVKeyCode("VK_PAUSE", 0x13);
	RegisterVKeyCode("VK_CAPITAL", 0x14);
	RegisterVKeyCode("VK_KANA", 0x15);
	RegisterVKeyCode("VK_IME_ON", 0x16);
	RegisterVKeyCode("VK_JUNJA", 0x17);
	RegisterVKeyCode("VK_FINAL", 0x18);
	RegisterVKeyCode("VK_KANJI", 0x19);
	RegisterVKeyCode("VK_IME_OFF", 0x1A);
	RegisterVKeyCode("VK_ESCAPE", 0x1B);
	RegisterVKeyCode("VK_CONVERT", 0x1C);
	RegisterVKeyCode("VK_NONCONVERT", 0x1D);
	RegisterVKeyCode("VK_ACCEPT", 0x1E);
	RegisterVKeyCode("VK_MODECHANGE", 0x1F);
	RegisterVKeyCode("VK_SPACE", 0x20);
	RegisterVKeyCode("VK_PRIOR", 0x21);
	RegisterVKeyCode("VK_NEXT", 0x22);
	RegisterVKeyCode("VK_END", 0x23);
	RegisterVKeyCode("VK_HOME", 0x24);
	RegisterVKeyCode("VK_LEFT", 0x25);
	RegisterVKeyCode("VK_UP", 0x26);
	RegisterVKeyCode("VK_RIGHT", 0x27);
	RegisterVKeyCode("VK_DOWN", 0x28);
	RegisterVKeyCode("VK_SELECT", 0x29);
	RegisterVKeyCode("VK_PRINT", 0x2A);
	RegisterVKeyCode("VK_EXECUTE", 0x2B);
	RegisterVKeyCode("VK_SNAPSHOT", 0x2C);
	RegisterVKeyCode("VK_INSERT", 0x2D);
	RegisterVKeyCode("VK_DELETE", 0x2E);
	RegisterVKeyCode("VK_HELP", 0x2F);
	RegisterVKeyCode("VK_LWIN", 0x5B);
	RegisterVKeyCode("VK_RWIN", 0x5C);
	RegisterVKeyCode("VK_APPS", 0x5D);
	RegisterVKeyCode("VK_SLEEP", 0x5F);
	RegisterVKeyCode("VK_NUMPAD0", 0x60);
	RegisterVKeyCode("VK_NUMPAD1", 0x61);
	RegisterVKeyCode("VK_NUMPAD2", 0x62);
	RegisterVKeyCode("VK_NUMPAD3", 0x63);
	RegisterVKeyCode("VK_NUMPAD4", 0x64);
	RegisterVKeyCode("VK_NUMPAD5", 0x65);
	RegisterVKeyCode("VK_NUMPAD6", 0x66);
	RegisterVKeyCode("VK_NUMPAD7", 0x67);
	RegisterVKeyCode("VK_NUMPAD8", 0x68);
	RegisterVKeyCode("VK_NUMPAD9", 0x69);
	RegisterVKeyCode("VK_MULTIPLY", 0x6A);
	RegisterVKeyCode("VK_ADD", 0x6B);
	RegisterVKeyCode("VK_SEPARATOR", 0x6C);
	RegisterVKeyCode("VK_SUBTRACT", 0x6D);
	RegisterVKeyCode("VK_DECIMAL", 0x6E);
	RegisterVKeyCode("VK_DIVIDE", 0x6F);
	RegisterVKeyCode("VK_F1", 0x70);
	RegisterVKeyCode("VK_F2", 0x71);
	RegisterVKeyCode("VK_F3", 0x72);
	RegisterVKeyCode("VK_F4", 0x73);
	RegisterVKeyCode("VK_F5", 0x74);
	RegisterVKeyCode("VK_F6", 0x75);
	RegisterVKeyCode("VK_F7", 0x76);
	RegisterVKeyCode("VK_F8", 0x77);
	RegisterVKeyCode("VK_F9", 0x78);
	RegisterVKeyCode("VK_F10", 0x79);
	RegisterVKeyCode("VK_F11", 0x7A);
	RegisterVKeyCode("VK_F12", 0x7B);
	RegisterVKeyCode("VK_F13", 0x7C);
	RegisterVKeyCode("VK_F14", 0x7D);
	RegisterVKeyCode("VK_F15", 0x7E);
	RegisterVKeyCode("VK_F16", 0x7F);
	RegisterVKeyCode("VK_F17", 0x80);
	RegisterVKeyCode("VK_F18", 0x81);
	RegisterVKeyCode("VK_F19", 0x82);
	RegisterVKeyCode("VK_F20", 0x83);
	RegisterVKeyCode("VK_F21", 0x84);
	RegisterVKeyCode("VK_F22", 0x85);
	RegisterVKeyCode("VK_F23", 0x86);
	RegisterVKeyCode("VK_F24", 0x87);
	RegisterVKeyCode("VK_NAVIGATION_VIEW", 0x88);
	RegisterVKeyCode("VK_NAVIGATION_MENU", 0x89);
	RegisterVKeyCode("VK_NAVIGATION_UP", 0x8A);
	RegisterVKeyCode("VK_NAVIGATION_DOWN", 0x8B);
	RegisterVKeyCode("VK_NAVIGATION_LEFT", 0x8C);
	RegisterVKeyCode("VK_NAVIGATION_RIGHT", 0x8D);
	RegisterVKeyCode("VK_NAVIGATION_ACCEPT", 0x8E);
	RegisterVKeyCode("VK_NAVIGATION_CANCEL", 0x8F);
	RegisterVKeyCode("VK_NUMLOCK", 0x90);
	RegisterVKeyCode("VK_SCROLL", 0x91);
	RegisterVKeyCode("VK_OEM_FJ_JISHO", 0x92);
	RegisterVKeyCode("VK_OEM_FJ_MASSHOU", 0x93);
	RegisterVKeyCode("VK_OEM_FJ_TOUROKU", 0x94);
	RegisterVKeyCode("VK_OEM_FJ_LOYA", 0x95);
	RegisterVKeyCode("VK_OEM_FJ_ROYA", 0x96);
	RegisterVKeyCode("VK_LSHIFT", 0xA0);
	RegisterVKeyCode("VK_RSHIFT", 0xA1);
	RegisterVKeyCode("VK_LCONTROL", 0xA2);
	RegisterVKeyCode("VK_RCONTROL", 0xA3);
	RegisterVKeyCode("VK_LMENU", 0xA4);
	RegisterVKeyCode("VK_RMENU", 0xA5);
	RegisterVKeyCode("VK_BROWSER_BACK", 0xA6);
	RegisterVKeyCode("VK_BROWSER_FORWARD", 0xA7);
	RegisterVKeyCode("VK_BROWSER_REFRESH", 0xA8);
	RegisterVKeyCode("VK_BROWSER_STOP", 0xA9);
	RegisterVKeyCode("VK_BROWSER_SEARCH", 0xAA);
	RegisterVKeyCode("VK_BROWSER_FAVORITES", 0xAB);
	RegisterVKeyCode("VK_BROWSER_HOME", 0xAC);
	RegisterVKeyCode("VK_VOLUME_MUTE", 0xAD);
	RegisterVKeyCode("VK_VOLUME_DOWN", 0xAE);
	RegisterVKeyCode("VK_VOLUME_UP", 0xAF);
	RegisterVKeyCode("VK_MEDIA_NEXT_TRACK", 0xB0);
	RegisterVKeyCode("VK_MEDIA_PREV_TRACK", 0xB1);
	RegisterVKeyCode("VK_MEDIA_STOP", 0xB2);
	RegisterVKeyCode("VK_MEDIA_PLAY_PAUSE", 0xB3);
	RegisterVKeyCode("VK_LAUNCH_MAIL", 0xB4);
	RegisterVKeyCode("VK_LAUNCH_MEDIA_SELECT", 0xB5);
	RegisterVKeyCode("VK_LAUNCH_APP1", 0xB6);
	RegisterVKeyCode("VK_LAUNCH_APP2", 0xB7);
	RegisterVKeyCode("VK_OEM_1", 0xBA);
	RegisterVKeyCode("VK_OEM_PLUS", 0xBB);
	RegisterVKeyCode("VK_OEM_COMMA", 0xBC);
	RegisterVKeyCode("VK_OEM_MINUS", 0xBD);
	RegisterVKeyCode("VK_OEM_PERIOD", 0xBE);
	RegisterVKeyCode("VK_OEM_2", 0xBF);
	RegisterVKeyCode("VK_OEM_3", 0xC0);
	RegisterVKeyCode("VK_GAMEPAD_A", 0xC3);
	RegisterVKeyCode("VK_GAMEPAD_B", 0xC4);
	RegisterVKeyCode("VK_GAMEPAD_X", 0xC5);
	RegisterVKeyCode("VK_GAMEPAD_Y", 0xC6);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_SHOULDER", 0xC7);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_SHOULDER", 0xC8);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_TRIGGER", 0xC9);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_TRIGGER", 0xCA);
	RegisterVKeyCode("VK_GAMEPAD_DPAD_UP", 0xCB);
	RegisterVKeyCode("VK_GAMEPAD_DPAD_DOWN", 0xCC);
	RegisterVKeyCode("VK_GAMEPAD_DPAD_LEFT", 0xCD);
	RegisterVKeyCode("VK_GAMEPAD_DPAD_RIGHT", 0xCE);
	RegisterVKeyCode("VK_GAMEPAD_MENU", 0xCF);
	RegisterVKeyCode("VK_GAMEPAD_VIEW", 0xD0);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON", 0xD1);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON", 0xD2);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_THUMBSTICK_UP", 0xD3);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_THUMBSTICK_DOWN", 0xD4);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT", 0xD5);
	RegisterVKeyCode("VK_GAMEPAD_LEFT_THUMBSTICK_LEFT", 0xD6);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_THUMBSTICK_UP", 0xD7);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN", 0xD8);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT", 0xD9);
	RegisterVKeyCode("VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT", 0xDA);
	RegisterVKeyCode("VK_OEM_4", 0xDB);
	RegisterVKeyCode("VK_OEM_5", 0xDC);
	RegisterVKeyCode("VK_OEM_6", 0xDD);
	RegisterVKeyCode("VK_OEM_7", 0xDE);
	RegisterVKeyCode("VK_OEM_8", 0xDF);
	RegisterVKeyCode("VK_OEM_AX", 0xE1);
	RegisterVKeyCode("VK_OEM_102", 0xE2);
	RegisterVKeyCode("VK_ICO_HELP", 0xE3);
	RegisterVKeyCode("VK_ICO_00", 0xE4);
	RegisterVKeyCode("VK_PROCESSKEY", 0xE5);
	RegisterVKeyCode("VK_ICO_CLEAR", 0xE6);
	RegisterVKeyCode("VK_PACKET", 0xE7);
	RegisterVKeyCode("VK_OEM_RESET", 0xE9);
	RegisterVKeyCode("VK_OEM_JUMP", 0xEA);
	RegisterVKeyCode("VK_OEM_PA1", 0xEB);
	RegisterVKeyCode("VK_OEM_PA2", 0xEC);
	RegisterVKeyCode("VK_OEM_PA3", 0xED);
	RegisterVKeyCode("VK_OEM_WSCTRL", 0xEE);
	RegisterVKeyCode("VK_OEM_CUSEL", 0xEF);
	RegisterVKeyCode("VK_OEM_ATTN", 0xF0);
	RegisterVKeyCode("VK_OEM_FINISH", 0xF1);
	RegisterVKeyCode("VK_OEM_COPY", 0xF2);
	RegisterVKeyCode("VK_OEM_AUTO", 0xF3);
	RegisterVKeyCode("VK_OEM_ENLW", 0xF4);
	RegisterVKeyCode("VK_OEM_BACKTAB", 0xF5);
	RegisterVKeyCode("VK_ATTN", 0xF6);
	RegisterVKeyCode("VK_CRSEL", 0xF7);
	RegisterVKeyCode("VK_EXSEL", 0xF8);
	RegisterVKeyCode("VK_EREOF", 0xF9);
	RegisterVKeyCode("VK_PLAY", 0xFA);
	RegisterVKeyCode("VK_ZOOM", 0xFB);
	RegisterVKeyCode("VK_NONAME", 0xFC);
	RegisterVKeyCode("VK_PA1", 0xFD);
	RegisterVKeyCode("VK_OEM_CLEAR", 0xFE);
}

const char* GetVKeyName(int key)
{
	return vkeyList[key & 255];
}

#endif
EXTERN_C_END
