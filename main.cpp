#include "config.h"
#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdint.h>
typedef uint32_t u32;
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <aclapi.h>
#include "RegistryKeyRemapping.h"

int my_strnicmp(const wchar_t* str1, const char* str2, int limit);


#if HANDLE_INVALID
//Only used detecting invalid key sequences.
//The timeout for the three key sequence, only used for invalid  - If it takes longer than this, don't treat it as a copilot key press/release
const int InvalidSequenceKeyChordTimeout = 30;
#endif

//The timeout for the three key sequence timer, used for transitions from LWIN -> LSHIFT -> F23
const int KeyChordTimerTimeout = 100;

#if DEBUG
int DebugMain();
EXTERN_C_START
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return DebugMain();
}
EXTERN_C_END
#endif

int Main();
EXTERN_C_START
int APIENTRY EntryPoint()
{
	return Main();
}
EXTERN_C_END

#if DEBUG

#include <queue>
#include <string>
#include <atomic>
#include <mutex>
std::queue<std::string> debugStringQueue;
std::mutex debugMutex;
HANDLE debugEvent;
std::atomic_bool debugQuit;

DWORD MyGetTickCount()
{
	LARGE_INTEGER performanceCount;
	QueryPerformanceCounter(&performanceCount);
	return (DWORD)(performanceCount.QuadPart / 10000);
}

DWORD WINAPI DebugThreadMain(void* unused)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS);

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

#endif //DEBUG

#if USE_SAS
HMODULE sasModule;
typedef VOID (WINAPI *SendSAS_Func)(BOOL);
SendSAS_Func SendSAS;
#endif //USE_SAS

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

#if WATCH_ACTIVE_WINDOW
HWND activeWindow;
bool activeWindowIsAdmin = false;
bool weAreAdmin = false;

HWINEVENTHOOK systemForegroundHook;
HWINEVENTHOOK systemMinimizeEndHook;
HWINEVENTHOOK eventObjectFocusHook;
#endif

HWND mainWindow;
HHOOK globalKeyboardHook;

#if REINSTALL_HOOK
UINT_PTR reattachTimer;
#endif

UINT_PTR activeTimer = 0;

#if HANDLE_INVALID
DWORD outOfPressSequenceTimestamp;
DWORD outOfReleaseSequenceTimestamp;
DWORD leftShiftTimestamp;
DWORD leftShiftTimestamp2;
bool leftShiftDown, leftShiftDown2;
bool outOfPressSequence = false;
bool outOfPressSequenceSuppressLeftShift = false;
bool outOfPressSequenceSuppressLeftWindows = false;
bool outOfReleaseSequence = false;
int pendingInjectedLeftShiftRelease = 0;
#endif //HANDLE_INVALID

bool leftWindowsSuppressed;
bool leftShiftSuppressed;
//bool f23Suppressed;

#if ENABLE_CUSTOM_KEY
bool allowTargetKeyToReplaceF23 = false;
DWORD targetVKey;
bool targetVKeyNoRepeat;
#else
const bool allowTargetKeyToReplaceF23 = false;
const DWORD targetVKey = VK_RCONTROL;
const bool targetVKeyNoRepeat = true;
#endif
bool targetVKeyDown = false;

#if USE_SAS
bool rightCtrlDown;
bool leftCtrlDown, leftAltDown, rightAltDown;
#endif //USE_SAS

int APIENTRY Main();

LRESULT CALLBACK MyKeyboardProc(int code, WPARAM wParam, LPARAM lParam);

#if DEBUG
const char* GetVKeyName(int key);
void RegisterVKeyCodes();
void DebugPrintf(const char* format, ...);

//In an attempt to make the keyboard more responsive while the console is processing
//we'll try putting the Windows code on its own dedicated thread and
//use the main thread for the console only.
//This only applies to debug builds.
DWORD WINAPI MainThread(void* unused)
{
	int exitCode = Main();
	return exitCode;
}

int DebugMain()
{
	RegisterVKeyCodes();
	debugEvent = CreateEvent(NULL, false, false, NULL);
	HANDLE hThread = CreateThread(NULL, 0, MainThread, 0, NULL, NULL);
	int exitCode = DebugThreadMain(0);
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

#endif //DEBUG

#if TEST
void InjectCopilotKeyDown()
{
	//proper sequence:
	PostMessage(mainWindow, WM_USER, VK_LWIN, 8);
	PostMessage(mainWindow, WM_USER, VK_LSHIFT, 8);
	PostMessage(mainWindow, WM_USER, VK_F23, 8);

	//example invalid sequence:
	//PostMessage(mainWindow, WM_USER, VK_LSHIFT, 8);
	//PostMessage(mainWindow, WM_USER, VK_F23, 8);

	//another example invalid sequence:
	//PostMessage(mainWindow, WM_USER, VK_LSHIFT, 8);
	//PostMessage(mainWindow, WM_USER, VK_LWIN, 8);
	//PostMessage(mainWindow, WM_USER, VK_F23, 8);
}
void InjectCopilotKeyUp()
{
	//proper sequence:
	PostMessage(mainWindow, WM_USER, VK_F23, 9);
	PostMessage(mainWindow, WM_USER, VK_LSHIFT, 9);
	PostMessage(mainWindow, WM_USER, VK_LWIN, 9);
	
	//No known invalid sequences have been seen so far?
}
#endif //TEST

void InjectKeyDownAsync(DWORD vKey)
{
	#if DEBUG
	DebugPrintf("    %d InjectKeyDownAsync %s\n", MyGetTickCount(), GetVKeyName(vKey));
	#endif
	PostMessage(mainWindow, WM_USER, vKey, 0);
}

void InjectKeyDownAsync2(DWORD vKey, bool isExtendedKey)
{
	#if DEBUG
	DebugPrintf("    %d InjectKeyDownAsync2 %s\n", MyGetTickCount(), GetVKeyName(vKey));
	#endif
	PostMessage(mainWindow, WM_USER, vKey, 2 + (isExtendedKey ? 4 : 0));
}

void InjectKeyUpAsync(DWORD vKey)
{
	#if DEBUG
	DebugPrintf("    %d InjectKeyUpAsync %s\n", MyGetTickCount(), GetVKeyName(vKey));
	#endif
	PostMessage(mainWindow, WM_USER, vKey, 1);
}

void InjectKeyUpAsync2(DWORD vKey, bool isExtendedKey)
{
	#if DEBUG
	DebugPrintf("    %d InjectKeyUpAsync2 %s\n", MyGetTickCount(), GetVKeyName(vKey));
	#endif
	PostMessage(mainWindow, WM_USER, vKey, 1 + 2 + (isExtendedKey ? 4 : 0));
}

void SetKeyDown(INPUT* input, DWORD VKEY);
void SetKeyUp(INPUT* input, DWORD VKEY);
void SetKeyDown2(INPUT* input, DWORD VKEY, bool isExtendedKey);
void SetKeyUp2(INPUT* input, DWORD VKEY, bool isExtendedKey);

//Converts a VKEY to a scancode, for when you don't have the extended key flag available
UINT VKeyToScanCode(WORD vkey)
{
	UINT result = MapVirtualKeyExW(vkey, MAPVK_VK_TO_VSC_EX, NULL);
	//These keys: Page Up, Page Down, End, Home, Left, Up, Right, Down, Insert, Delete
	//are also found on the numpad, so MapVirtualKeyExW will not set the extended key flag for those by default.
	//We want to set the extended key flag for those (and force the non-numpad version)
	if ((vkey >= VK_PRIOR && vkey <= VK_DOWN) ||
		vkey == VK_INSERT || vkey == VK_DELETE ||
		vkey == VK_NUMLOCK || vkey == VK_RSHIFT)
	{
		result |= 0xE000;
	}
	return result;
}

//Converts a VKEY to a scancode, for when you do have the extended key flag available
UINT VKeyToScanCode2(WORD vkey, int isExtended)
{
	UINT result = VKeyToScanCode(vkey);
	//Force extended key for Numpad Enter
	if (vkey == VK_RETURN && isExtended)
	{
		result |= 0xE000;
	}
	//Change Sysreq key from Print Screen key back to Sysreq key
	if (vkey == VK_SNAPSHOT && isExtended)
	{
		result = 0xE037;
	}
	//Change Pause key back
	if (vkey == VK_PAUSE && !isExtended)
	{
		result = 0x45;
	}
	//When pressing the numpad keys with numlock off, extended key flag is normally off
	//But my function VKeyToScanCode above forces the extended key flag on (not pressing the numpad version)
	//Force the extended key flag off, and return them to numpad versions
	if (!isExtended)
	{
		if ((vkey >= VK_PRIOR && vkey <= VK_DOWN) ||
			vkey == VK_INSERT || vkey == VK_DELETE ||
			vkey == VK_NUMLOCK || vkey == VK_RSHIFT) //page up, page down, end, home, left, up, right, down, insert, delete
		{
			result &= 0xFF;
		}
	}
	return result;
}

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INPUT input;
	switch (msg)
	{
	case WM_USER:
		{
			bool isRelease = 0 != (lParam & 1);
			bool haveExtendedKey = 0 != (lParam & 2);
			bool isExtendedKey = 0 != (lParam & 4);
			bool isDebugKey = 0 != (lParam & 8);
			if (isRelease)
			{
				if (!haveExtendedKey)
				{
					SetKeyUp(&input, (DWORD)wParam);
				}
				else
				{
					SetKeyUp2(&input, (DWORD)wParam, isExtendedKey);
				}
			}
			else
			{
				if (!haveExtendedKey)
				{
					SetKeyDown(&input, (DWORD)wParam);
				}
				else
				{
					SetKeyDown2(&input, (DWORD)wParam, isExtendedKey);
				}
			}
			#if TEST
			if (isDebugKey)
			{
				//Set a special Extra Info on the key input so that we don't ignore that injected keypress
				input.ki.dwExtraInfo = 0x12345678;
			}
			#endif //TEST
			UINT inputsSent = SendInput(1, &input, sizeof(input));
			#if DEBUG
			if (inputsSent == 0)
			{
				DebugPrintf("SendInput failed\n");
			}
			#endif

			return 0;
		}
		#if USE_RAW_INPUT
	case WM_INPUT:
		{
			//Raw Input isn't really raw, it's been processed by low level keyboard hooks first
			HRAWINPUT handle = (HRAWINPUT)lParam;
			RAWINPUT data = {};
			data.header.dwSize = sizeof(RAWINPUTHEADER);
			UINT size = sizeof(data);
			GetRawInputData(handle, RID_INPUT, &data, &size, sizeof(RAWINPUTHEADER));
			#if DEBUG
			const char* pressString = "Other message";
			if (data.data.keyboard.Message == WM_KEYUP) pressString = "released";
			if (data.data.keyboard.Message == WM_KEYDOWN) pressString = "pressed";
			DebugPrintf("Raw Input: %s %s\n", GetVKeyName(data.data.keyboard.VKey), pressString);
			#endif //DEBUG
			if (data.data.keyboard.Message == WM_KEYUP)
			{
				//If F23 key is detected as being released here, then somehow it didn't get rejected by the hook
				//Force target key to be released in case that happens
				if (data.data.keyboard.VKey == VK_F23)
				{
					InjectKeyUpAsync(targetVKey);
				}
			}
		}
		return 0;
		#endif
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

#if REINSTALL_HOOK
void CALLBACK TimerHandlerToReattachHook(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
#endif

#if WATCH_ACTIVE_WINDOW
void CALLBACK MyWinEventHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
bool IsWindowAdmin(HWND hwnd);
#endif

int APIENTRY Main()
{
	#if ENABLE_CUSTOM_KEY
	targetVKey = VK_RCONTROL;
	targetVKeyNoRepeat = true;
	#endif

	#if HANDLE_INVALID
	outOfPressSequenceTimestamp = GetTickCount();
	leftShiftTimestamp = GetTickCount();
	leftShiftTimestamp2 = GetTickCount();
	#endif //HANDLE_INVALID

	commandLine = GetCommandLineW();
	argv = CommandLineToArgvW(commandLine, &argc);
	
	#if ENABLE_REGISTRY_REMAPPING
	bool wantToRegistryRemap = false;
	#endif

	for (int i = 1; i < argc; i++)
	{
		#if ENABLE_REGISTRY_REMAPPING
		if (0 == my_strnicmp(argv[i], "--registry-remap", 255))
		{
			wantToRegistryRemap = true;
			continue;
		}
		#endif
		#if ENABLE_CUSTOM_KEY
		if (0 == my_strnicmp(argv[i], "--key", 255) && i + 1 < argc)
		{
			i++;
			const wchar_t* const keyName = argv[i];
			int VKeyNameToKeyCode(const wchar_t* keyName);
			int keyCode = VKeyNameToKeyCode(keyName);
			if (keyCode != 0)
			{
				targetVKey = keyCode;
				if (targetVKey == VK_SHIFT) targetVKey = VK_RSHIFT;
				if (targetVKey == VK_MENU) targetVKey = VK_RMENU;
				if (targetVKey == VK_CONTROL) targetVKey = VK_RCONTROL;
				if (targetVKey >= VK_LSHIFT && targetVKey <= VK_RMENU)
				{
					targetVKeyNoRepeat = true;
				}
				else
				{
					targetVKeyNoRepeat = false;
				}
			}
		}
		#endif
	}
	#if ENABLE_REGISTRY_REMAPPING
	if (wantToRegistryRemap)
	{
		int targetScanCode = VKeyToScanCode(targetVKey);
		//scancode 0x6E is the F23 key
		RegisterRemappedKey(0x6E, targetScanCode);
		allowTargetKeyToReplaceF23 = true;
	}
	#endif
	if (!DO_NOTHING)
	{
		HANDLE mutex = OpenMutexA(SYNCHRONIZE, false, "Mutex for NoCopilotKey");
		if (mutex == NULL)
		{
			mutex = CreateMutexA(NULL, true, "Mutex for NoCopilotKey");
		}
		else
		{
			return -1;
		}
	}

	#if USE_SAS
	sasModule = LoadLibraryA("sas.dll");
	if (sasModule != NULL)
	{
		SendSAS = (SendSAS_Func)GetProcAddress(sasModule, "SendSAS");
	}
	#endif //USE_SAS

	HMODULE module = GetModuleHandleW(NULL);

	WNDCLASSW wndClass = {};
	wndClass.lpfnWndProc = MyWndProc;
	wndClass.hInstance = module;
	wndClass.lpszClassName = L"NoCopilotKey Message Window";
	ATOM windowClassAtom = RegisterClassW(&wndClass);
	mainWindow = CreateWindowExW(0, wndClass.lpszClassName, L"NoCopilotKey", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	#if USE_RAW_INPUT
	RAWINPUTDEVICE rawInputDevice = {};
	rawInputDevice.usUsagePage = 1;
	rawInputDevice.usUsage = 6;
	rawInputDevice.dwFlags = RIDEV_INPUTSINK;
	rawInputDevice.hwndTarget = mainWindow;

	if (!DO_NOTHING)
	{
		BOOL okay = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE));
	}
	#endif

	globalKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &MyKeyboardProc, module, 0);
	int lastError = GetLastError();

	#if REINSTALL_HOOK
	reattachTimer = SetTimer(mainWindow, 2, 1000, &TimerHandlerToReattachHook);
	#endif //REINSTALL_HOOK

	#if WATCH_ACTIVE_WINDOW
	//eventLastTickCount = GetTickCount() - 10;
	weAreAdmin = IsWindowAdmin(mainWindow);
	if (!weAreAdmin)
	{
		activeWindow = mainWindow; //start with a window that can't possibly be the foreground window
		systemForegroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, MyWinEventHookProc, 0, 0, WINEVENT_OUTOFCONTEXT);
		systemMinimizeEndHook = SetWinEventHook(EVENT_SYSTEM_MINIMIZEEND, EVENT_SYSTEM_MINIMIZEEND, NULL, MyWinEventHookProc, 0, 0, WINEVENT_OUTOFCONTEXT);
		eventObjectFocusHook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, MyWinEventHookProc, 0, 0, WINEVENT_OUTOFCONTEXT);
		MyWinEventHookProc(NULL, EVENT_SYSTEM_FOREGROUND, NULL, 0, 0, 0, 0);
	}
	#endif //WATCH_ACTIVE_WINDOW

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

#if REINSTALL_HOOK
void CALLBACK TimerHandlerToReattachHook(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	reattachTimer = SetTimer(mainWindow, reattachTimer, 1000, &TimerHandlerToReattachHook);
	UnhookWindowsHookEx(globalKeyboardHook);
	globalKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &MyKeyboardProc, GetModuleHandle(NULL), 0);
}
#endif //REINSTALL_HOOK

#if WATCH_ACTIVE_WINDOW
void IsAdminChanged()
{
	if (!activeWindowIsAdmin)
	{
		if (targetVKeyDown)
		{
			InjectKeyUpAsync(targetVKey);
			InjectKeyUpAsync(VK_LSHIFT);
			InjectKeyUpAsync(VK_LWIN);
			InjectKeyUpAsync(VK_F23);
		}
	}
	else
	{
		if (targetVKeyDown)
		{
			#if DEBUG
			DebugPrintf("Target key is stuck due to switching to admin window\n");
			#endif
		}
	}
}

bool IsWindowAdmin(HWND hwnd)
{
	bool isAdmin = false;
	DWORD processId = 0;
	DWORD threadId = GetWindowThreadProcessId(activeWindow, &processId);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processId);
	if (hProcess)
	{
		HANDLE tokenHandle = NULL;
		BOOL okay = OpenProcessToken(hProcess, TOKEN_QUERY, &tokenHandle);
		if (tokenHandle)
		{
			TOKEN_ELEVATION_TYPE elevationType = {};
			DWORD bytesRead = 0;
			okay = GetTokenInformation(tokenHandle, TokenElevationType, &elevationType, sizeof(elevationType), &bytesRead);
			CloseHandle(tokenHandle);
			if (elevationType == TokenElevationTypeFull)
			{
				isAdmin = true;
			}
		}
		CloseHandle(hProcess);
	}
	return isAdmin;
}

void DoPollForegroundWindow()
{
	HWND foregroundWindow = GetForegroundWindow();
	if (foregroundWindow != activeWindow)
	{
		#if DEBUG
		DebugPrintf("Active Window Changed: %p -> %p\n", activeWindow, foregroundWindow);
		#endif
		activeWindow = foregroundWindow;
		if (activeWindow != NULL)
		{
			bool activeWindowWasAdmin = activeWindowIsAdmin;
			activeWindowIsAdmin = IsWindowAdmin(activeWindow);
			if (activeWindowIsAdmin != activeWindowWasAdmin)
			{
				#if DEBUG
				if (activeWindowIsAdmin)
				{
					DebugPrintf("Admin Window\n");
				}
				else
				{
					DebugPrintf("No longer Admin Window\n");
				}
				#endif
				IsAdminChanged();
			}
			#if REINSTALL_HOOK
			UnhookWindowsHookEx(globalKeyboardHook);
			globalKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &MyKeyboardProc, GetModuleHandle(NULL), 0);
			#endif
		}
	}
}

void CALLBACK MyWinEventHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
	DoPollForegroundWindow();
}
#endif //WATCH_ACTIVE_WINDOW

#if HANDLE_INVALID
void SetLeftShiftDown(bool isDown)
{
	#if DEBUG
	if (isDown != leftShiftDown)
	{
		DebugPrintf("    Left shift changed: %d -> %d\n", leftShiftDown, isDown);
	}
	#endif //DEBUG
	leftShiftDown2 = leftShiftDown;
	leftShiftDown = isDown;
	leftShiftTimestamp2 = leftShiftTimestamp2;
	leftShiftTimestamp = GetTickCount();
}
#endif //HANDLE_INVALID

void SetPressState(STATE state)
{
	#if DEBUG
	if (pressState != state)
	{
		DebugPrintf("    Press State Transition: %d -> %d\n", pressState, state);
	}
	#endif //DEBUG
	pressState = state;
}

void SetReleaseState(STATE state)
{
	#if DEBUG
	if (releaseState != state)
	{
		DebugPrintf("    Release State Transition: %d -> %d\n", releaseState, state);
	}
	#endif //DEBUG
	releaseState = state;
}

void CancelTimer();

void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	#if DEBUG
	DebugPrintf("    %d TimerProc (took too long to see all three keys)\n", MyGetTickCount());
	#endif
	ReplaySuppressedKeys();
}

void EnsureTimer()
{
	if (activeTimer == 0)
	{
		activeTimer = SetTimer(mainWindow, 1, KeyChordTimerTimeout, TimerProc);
		#if DEBUG
		DebugPrintf("    Timer set\n");
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
		DebugPrintf("    Timer cancelled\n");
		#endif	
	}
}

void SetKeyDown2(INPUT* input, DWORD VKEY, bool isExtendedKey)
{
	input->type = INPUT_KEYBOARD;
	input->ki.wVk = (WORD)VKEY;
	input->ki.wScan = (WORD)VKeyToScanCode2((WORD)VKEY, isExtendedKey);
	input->ki.dwFlags = 0;
	if (input->ki.wScan >= 0xE000)
	{
		input->ki.wScan &= 0xFF;
		input->ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	}
	input->ki.time = 0;
	input->ki.dwExtraInfo = 0;
}

void SetKeyUp2(INPUT* input, DWORD VKEY, bool isExtendedKey)
{
	SetKeyDown2(input, VKEY, isExtendedKey);
	input->ki.dwFlags |= KEYEVENTF_KEYUP;
}

void SetKeyDown(INPUT* input, DWORD VKEY)
{
	input->type = INPUT_KEYBOARD;
	input->ki.wVk = (WORD)VKEY;
	input->ki.wScan = (WORD)VKeyToScanCode((WORD)VKEY);
	input->ki.dwFlags = 0;
	if (input->ki.wScan >= 0xE000)
	{
		input->ki.wScan &= 0xFF;
		input->ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	}
	input->ki.time = 0;
	input->ki.dwExtraInfo = 0;
}

void SetKeyUp(INPUT* input, DWORD VKEY)
{
	SetKeyDown(input, VKEY);
	input->ki.dwFlags |= KEYEVENTF_KEYUP;
}

bool HaveSuppressedKeys()
{
	return leftWindowsSuppressed || leftShiftSuppressed;
}

void ReplaySuppressedKeys()
{
	if (leftWindowsSuppressed)
	{
		leftWindowsSuppressed = false;
		InjectKeyDownAsync(VK_LWIN);
	}
	if (leftShiftSuppressed)
	{
		leftShiftSuppressed = false;
		InjectKeyDownAsync(VK_LSHIFT);
	}
	CancelTimer();
}

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
	bool isExtendedKey = 0 != (flags & LLKHF_EXTENDED);
	bool pressed = !released;
	bool isF23 = keyCode == VK_F23 || (allowTargetKeyToReplaceF23 && keyCode == targetVKey);

	#if TEST
	//Special Extra info indicates that we don't ignore a special injected keypress
	if (hookStruct->dwExtraInfo == 0x12345678)
	{
		injected = false;
	}
	#endif //TEST

	if (injected)
	{
		#if HANDLE_INVALID
		if (outOfPressSequence && keyCode == VK_LSHIFT)
		{
			if (pendingInjectedLeftShiftRelease == 2)
			{
				#if DEBUG
				DebugPrintf("    Special: An out-of-sequence F23 press requested that Left Shift be released.\n"
					"      But another press to Left Shift happened before the key release was handled.\n"
					"      Accepting the Left Shift press and rejecting the injected Left Shift release.\n");
				#endif
				pendingInjectedLeftShiftRelease = 0;
				return -1;
			}
			else if (pendingInjectedLeftShiftRelease == 1)
			{
				#if DEBUG
				DebugPrintf("    Injected Left shift release was accepted.\n");
				#endif
				pendingInjectedLeftShiftRelease = 0;
			}
		}
		#endif //HANDLE_INVALID
		return CallNextHookEx(NULL, code, wParam, lParam);
	}
	
	if (DO_NOTHING)
	{
		return CallNextHookEx(NULL, code, wParam, lParam);
	}

	#if HANDLE_INVALID
	if (outOfPressSequence)
	{
		DWORD outOfPressSequenceElapsedTime = GetTickCount() - outOfPressSequenceTimestamp;
		if (outOfPressSequenceElapsedTime > InvalidSequenceKeyChordTimeout)
		{
			outOfPressSequence = false;
		}
	}
	if (outOfReleaseSequence)
	{
		DWORD outOfReleaseSequenceElapsedTime = GetTickCount() - outOfReleaseSequenceTimestamp;
		if (outOfReleaseSequenceElapsedTime > InvalidSequenceKeyChordTimeout)
		{
			outOfReleaseSequence = false;
		}
	}
	#endif //HANDLE_INVALID

	if (pressed)
	{
		#if TEST
		if (keyCode == VK_OEM_3) // make ` key count as copilot key for test build
		{
			InjectCopilotKeyDown();
			return -1;
		}
		#endif //TEST
		
		#if HANDLE_INVALID
		//any key press ends an out-of-sequence key release sequence
		outOfReleaseSequence = false;
		#endif //HANDLE_INVALID

		if (keyCode == VK_LWIN)
		{
			ReplaySuppressedKeys();
			leftWindowsSuppressed = true;
			SetPressState(STATE::LeftWindows);
			EnsureTimer();
			#if HANDLE_INVALID
			if (outOfPressSequence && outOfPressSequenceSuppressLeftWindows)
			{
				#if DEBUG
				DebugPrintf("    Left windows rejected because it was within 30ms of out-of-sequence F23\n");
				#endif
				SetPressState(STATE::Idle);
				outOfPressSequenceSuppressLeftWindows = false;
				//don't replay left windows key
				leftWindowsSuppressed = false;
			}
			#endif //HANDLE_INVALID
			return -1;  //block LWIN key
		}

		if (pressState == STATE::LeftWindows)
		{
			if (keyCode == VK_LSHIFT)
			{
				leftShiftSuppressed = true;
				SetPressState(STATE::LeftShift);
				#if HANDLE_INVALID
				if (outOfPressSequence && outOfPressSequenceSuppressLeftShift)
				{
					#if DEBUG
					DebugPrintf("    Left shift rejected because it was within 30ms of out-of-sequence F23\n");
					#endif
					outOfPressSequenceSuppressLeftShift = false;
					//don't replay left windows key
					leftShiftSuppressed = false;
				}
				#endif
				return -1;  //block LSHIFT key
			}
			#if HANDLE_INVALID
			else if (keyCode == VK_F23)
			{
				//Left Windows -> F23 breaks the sequence
				#if DEBUG
				DebugPrintf("  Out-of-sequence key press: LWIN -> F23\n");
				#endif
				//keep Left Windows Key suppressed (don't replay it later)
				if (leftWindowsSuppressed)
				{
					leftWindowsSuppressed = false;
				}
				outOfPressSequenceTimestamp = GetTickCount();
				outOfPressSequence = true;
				pendingInjectedLeftShiftRelease = 0;
				outOfPressSequenceSuppressLeftShift = false;
				//Possibility of bad sequence: LShift LWin F23 or LWin F23 LShift
				//Was LShift last pressed within 30ms?  (LShift LWin F23)
				if (leftShiftDown && ((GetTickCount() - leftShiftTimestamp) <= InvalidSequenceKeyChordTimeout))
				{
					//Was LShift previously in a pressed state?
					if (leftShiftDown2)
					{
						//Left shift was held down when seeing a left shift press, we don't want Left Shift Released, leave it alone
						#if DEBUG
						DebugPrintf("    Left shift was held down, do not release the key\n");
						#endif
					}
					else
					{
						#if DEBUG
						DebugPrintf("    Left shift had become pressed within 30ms, release the key\n");
						#endif
						//send Left Shift Released to cancel the prior keypress
						InjectKeyUpAsync(VK_LSHIFT);
						pendingInjectedLeftShiftRelease = 1;
						//Reset leftShiftDown flag to false to avoid stuck shift key
						SetLeftShiftDown(false);
					}
				}
				else
				{
					//Possibility of LWin F23 LShift (in the future)
					//Suppress Left Shift if it is pressed with 30ms
					outOfPressSequenceSuppressLeftShift = true;
					#if DEBUG
					DebugPrintf("    Left shift wasn't pressed, next Left shift within 30ms will be suppressed\n");
					#endif
				}
				SetPressState(STATE::Idle);
				SetReleaseState(STATE::F23);
				CancelTimer();
				leftWindowsSuppressed = false;
				InjectKeyDownAsync(targetVKey);
				return -1;  //block F23 key
			}
			#endif //HANDLE_INVALID
			else
			{
				//For keys that aren't in the sequence
				SetPressState(STATE::Idle);
				if (HaveSuppressedKeys())
				{
					ReplaySuppressedKeys();
					//block key now then enqueue it for replay afterwards
					//so that the key happens after the press to LWIN or LSHIFT
					InjectKeyDownAsync2(keyCode, isExtendedKey);
					return -1;  //block F23 key
				}
			}
		}
		else if (pressState == STATE::LeftShift)
		{
			if (isF23)
			{
				SetPressState(STATE::Idle);
				SetReleaseState(STATE::F23);
				CancelTimer();
				leftShiftSuppressed = false;
				leftWindowsSuppressed = false;
				//Copilot Key is a repeating key, but a real right ctrl (or alt or shift) key doesn't repeat
				//Ignore repeated presses
				if (targetVKeyDown && targetVKeyNoRepeat)
				{
					return -1;
				}
				if (keyCode == VK_F23)
				{
					InjectKeyDownAsync(targetVKey);
					return -1;  //block F23 key
				}
				//If we pressed remapped F23->Target Key, allow the Target keypress to proceed
			}
			else
			{
				//For keys that aren't in the sequence
				SetPressState(STATE::Idle);
				if (HaveSuppressedKeys())
				{
					ReplaySuppressedKeys();
					//block key now then enqueue it for replay afterwards
					//so that the key happens after the press to LWIN or LSHIFT
					InjectKeyDownAsync2(keyCode, isExtendedKey);
					return -1;
				}
			}
		}
		#if HANDLE_INVALID
		if (keyCode == VK_F23)
		{
			//Out of sequence keypress to F23
			#if DEBUG
			DebugPrintf("  Out-of-sequence key press: F23\n");
			#endif
			//possible sequences handled here:
			//F23
			//F23 LShift
			//F23 LShift LWin
			//LShift F23
			//LShift F23 LWin
			//F23 LWin LShift
			//sequences handled elsewhere:
			//LWin LShift F23 (proper sequence)
			//LWin F23 LShift (handled by LWin -> F23 code)
			//LWin F23 (handled by LWin -> F23 code)
			//LShift LWin F23 (handled by LWin -> F23 code)
			
			//so far, only correct sequence and LShift F23 have been seen, LShift LWin F23 has allegedly been seen too

			outOfPressSequenceTimestamp = GetTickCount();
			outOfPressSequence = true;
			pendingInjectedLeftShiftRelease = 0;
			outOfPressSequenceSuppressLeftShift = false;
			//To handle the cases where LShift comes before F23:
			//left shift may have been pressed with 30ms, if it was, release left shift unless it was held down
			if (leftShiftDown && ((GetTickCount() - leftShiftTimestamp) <= InvalidSequenceKeyChordTimeout))
			{
				//Was LShift previously in a pressed state?
				if (leftShiftDown2)
				{
					//Left shift was held down when seeing a left shift press, we don't want Left Shift Released, leave it alone
					#if DEBUG
					DebugPrintf("    Left shift was held down, do not release the key\n");
					#endif
				}
				else
				{
					#if DEBUG
					DebugPrintf("    Left shift had become pressed within 30ms, release the key\n");
					#endif
					//send Left Shift Released to cancel the prior keypress
					InjectKeyUpAsync(VK_LSHIFT);
					pendingInjectedLeftShiftRelease = 1;
					//Reset leftShiftDown flag to false to avoid stuck shift key
					SetLeftShiftDown(false);
				}
			}
			else
			{
				//Possibility of LShift in the future as part of the sequence
				//Suppress Left Shift if it is pressed with 30ms
				outOfPressSequenceSuppressLeftShift = true;
				#if DEBUG
				DebugPrintf("    Left shift wasn't pressed, next Left shift within 30ms will be suppressed\n");
				#endif
			}
			SetPressState(STATE::Idle);
			SetReleaseState(STATE::F23);
			CancelTimer();
			outOfPressSequenceSuppressLeftWindows = true;
			leftWindowsSuppressed = false;
			InjectKeyDownAsync(targetVKey);
			return -1;  //block F23 key
		}

		if (keyCode == VK_LSHIFT)
		{
			if (outOfPressSequence && outOfPressSequenceSuppressLeftShift)
			{
				#if DEBUG
				DebugPrintf("    Left shift rejected because it was within 30ms of out-of-sequence F23\n");
				#endif
				outOfPressSequenceSuppressLeftShift = false;
				return -1;
			}
			if (outOfPressSequence && pendingInjectedLeftShiftRelease == 1)
			{
				#if DEBUG
				DebugPrintf("    Left shift pressed while injected release not yet handled\n");
				#endif
				pendingInjectedLeftShiftRelease = 2;
			}
			//Left Shift is tracked by the program in order to detect the invalid sequence LSHIFT F23
			SetLeftShiftDown(true);
			if (leftShiftDown2)
			{
				#if DEBUG
				DebugPrintf("    Left shift pressed while already held down\n");
				#endif
			}
		}
		//Allow other keys to force-break an invalid sequence
		if (!(keyCode == VK_F23 || keyCode == VK_LSHIFT || keyCode == VK_LWIN))
		{
			outOfPressSequence = false;
		}
		#endif  //HANDLE_INVALID
	}
	else if (released)
	{
		#if TEST
		if (keyCode == VK_OEM_3) // make ` key count as copilot key for a test mode build
		{
			InjectCopilotKeyUp();
			return -1;
		}
		#endif //TEST

		#if HANDLE_INVALID
		//Allow releasing any key to force-break an invalid sequence
		outOfPressSequence = false;
		#endif

		if (isF23 && releaseState == STATE::F23)
		{
			#if HANDLE_INVALID
			SetPressState(STATE::Idle);
			CancelTimer();
			#endif //HANDLE_INVALID
			SetReleaseState(STATE::LeftShift);

			if (keyCode == VK_F23)
			{
				InjectKeyUpAsync(targetVKey);
				return -1;  //block F23 key release
			}
			//allow remapped F23->Target keypress to proceed
		}
		if (keyCode == VK_LSHIFT && releaseState == STATE::LeftShift)
		{
			SetReleaseState(STATE::LeftWindows);
			return -1;  //block LSHIFT key release
		}
		if (keyCode == VK_LWIN && releaseState == STATE::LeftWindows)
		{
			SetReleaseState(STATE::Idle);
			return -1;  //block LWIN key release
		}

		#if HANDLE_INVALID
		if (keyCode == VK_LSHIFT)
		{
			SetLeftShiftDown(false);
		}

		if (keyCode == VK_F23)
		{
			//Out-of-sequence F23 key release
			#if DEBUG
			DebugPrintf("  Out-of-sequence F23 key release\n");
			#endif
			//This only happens when there is an F23 release without a corresponding handled F23 press
			//We don't have a good way to handle this, so just treat all keys as if they are being released.
			InjectKeyUpAsync(targetVKey);
			InjectKeyUpAsync(VK_LSHIFT);
			InjectKeyUpAsync(VK_LWIN);
			InjectKeyUpAsync(VK_F23);
			return -1;
		}
		#endif //HANDLE_INVALID

		if (pressState != STATE::Idle)
		{
			bool leftWindowsWasSuppressed = leftWindowsSuppressed;
			bool leftShiftWasSuppressed = leftShiftSuppressed;
			ReplaySuppressedKeys();
			SetPressState(STATE::Idle);
			//Game Bar is weird, you need to inject a key up event and suppress the real key up
			//otherwise Game Bar sees the injected Key Down after the real Key Up.
			if (leftWindowsWasSuppressed && keyCode == VK_LWIN)
			{
				InjectKeyUpAsync(VK_LWIN);
				return -1;
			}
			if (leftShiftWasSuppressed && keyCode == VK_LSHIFT)
			{
				InjectKeyUpAsync(VK_LSHIFT);
				return -1;
			}
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
	DWORD arrivalTime = MyGetTickCount();
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
		//DWORD extendedKey = ((hookStruct->flags & LLKHF_EXTENDED) ? 0xE000 : 0);
		//DWORD scanCode = hookStruct->scanCode + extendedKey;
		//DebugPrintf("debug: scancode %02X -> %02X\n", scanCode, VKeyToScanCode2(keyCode, extendedKey));
	}
	#endif //DEBUG
	LRESULT result = MyKeyboardProc2(code, wParam, lParam);
	#if DEBUG
	if (keyCode > 0 && result != 0)
	{
		DebugPrintf("  %d %s%s0x%02X %s was suppressed\n", arrivalTime, injectedMessage, pressedMessage, keyCode, GetVKeyName(keyCode));
	}
	#endif //DEBUG			
	if (result == 0)
	{
		if (pressed)
		{
			if (keyCode == targetVKey && injected)
			{
				targetVKeyDown = true;
			}
			#if USE_SAS
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
					DebugPrintf("Sending Alt + Ctrl + Del (SendSAS)\n");
					#endif
					if (SendSAS != NULL) SendSAS(true);
				}
			}
			#endif //USE_SAS
		}
		else
		{
			if (keyCode == targetVKey && injected)
			{
				targetVKeyDown = false;
			}
			#if USE_SAS
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
			#endif //USE_SAS
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
	RegisterVKeyCode("VK_BACK", 0x08); //Backspace
	RegisterVKeyCode("VK_TAB", 0x09);
	RegisterVKeyCode("VK_CLEAR", 0x0C);
	RegisterVKeyCode("VK_RETURN", 0x0D);
	RegisterVKeyCode("VK_SHIFT", 0x10);
	RegisterVKeyCode("VK_CONTROL", 0x11);
	RegisterVKeyCode("VK_MENU", 0x12);
	RegisterVKeyCode("VK_PAUSE", 0x13);
	RegisterVKeyCode("VK_CAPITAL", 0x14); //Caps lock
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
	RegisterVKeyCode("VK_PRIOR", 0x21); //Page Up
	RegisterVKeyCode("VK_NEXT", 0x22); //Page Down
	RegisterVKeyCode("VK_END", 0x23);
	RegisterVKeyCode("VK_HOME", 0x24);
	RegisterVKeyCode("VK_LEFT", 0x25);
	RegisterVKeyCode("VK_UP", 0x26);
	RegisterVKeyCode("VK_RIGHT", 0x27);
	RegisterVKeyCode("VK_DOWN", 0x28);
	RegisterVKeyCode("VK_SELECT", 0x29);
	RegisterVKeyCode("VK_PRINT", 0x2A);
	RegisterVKeyCode("VK_EXECUTE", 0x2B);
	RegisterVKeyCode("VK_SNAPSHOT", 0x2C); //Print Screen
	RegisterVKeyCode("VK_INSERT", 0x2D);
	RegisterVKeyCode("VK_DELETE", 0x2E);
	RegisterVKeyCode("VK_HELP", 0x2F);
	RegisterVKeyCode("VK_LWIN", 0x5B);
	RegisterVKeyCode("VK_RWIN", 0x5C);
	RegisterVKeyCode("VK_APPS", 0x5D); //Menu Key
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
	RegisterVKeyCode("VK_MULTIPLY", 0x6A); //Numpad *
	RegisterVKeyCode("VK_ADD", 0x6B); //Numpad +
	RegisterVKeyCode("VK_SEPARATOR", 0x6C);
	RegisterVKeyCode("VK_SUBTRACT", 0x6D); //Numpad -
	RegisterVKeyCode("VK_DECIMAL", 0x6E); //Numpad .
	RegisterVKeyCode("VK_DIVIDE", 0x6F); //Numpad /
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
	RegisterVKeyCode("VK_SCROLL", 0x91); //Scroll Lock
	RegisterVKeyCode("VK_OEM_FJ_JISHO", 0x92);
	RegisterVKeyCode("VK_OEM_FJ_MASSHOU", 0x93);
	RegisterVKeyCode("VK_OEM_FJ_TOUROKU", 0x94);
	RegisterVKeyCode("VK_OEM_FJ_LOYA", 0x95);
	RegisterVKeyCode("VK_OEM_FJ_ROYA", 0x96);
	RegisterVKeyCode("VK_LSHIFT", 0xA0);
	RegisterVKeyCode("VK_RSHIFT", 0xA1);
	RegisterVKeyCode("VK_LCONTROL", 0xA2);
	RegisterVKeyCode("VK_RCONTROL", 0xA3);
	RegisterVKeyCode("VK_LMENU", 0xA4); //Left Alt
	RegisterVKeyCode("VK_RMENU", 0xA5); //Right Alt
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
	RegisterVKeyCode("VK_OEM_1", 0xBA); //;
	RegisterVKeyCode("VK_OEM_PLUS", 0xBB); //=
	RegisterVKeyCode("VK_OEM_COMMA", 0xBC); //,
	RegisterVKeyCode("VK_OEM_MINUS", 0xBD); //-
	RegisterVKeyCode("VK_OEM_PERIOD", 0xBE); //.
	RegisterVKeyCode("VK_OEM_2", 0xBF); ///
	RegisterVKeyCode("VK_OEM_3", 0xC0); //`
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
	RegisterVKeyCode("VK_OEM_4", 0xDB); //[
	RegisterVKeyCode("VK_OEM_5", 0xDC); //\ 
	RegisterVKeyCode("VK_OEM_6", 0xDD); //]
	RegisterVKeyCode("VK_OEM_7", 0xDE); //'
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
	RegisterVKeyCode("0xFF", 0xFF);
}

const char* GetVKeyName(int key)
{
	return vkeyList[key & 255];
}

#endif //DEBUG
