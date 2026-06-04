#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <Windows.h>

#include "RegistryKeyRemapping.h"

struct KeyBindingsHeader
{
	DWORD version;
	DWORD headerFlags;
	DWORD numberOfEntries;
};

struct KeyBindingsEntry
{
	WORD destinationKey;
	WORD sourceKey;
};

static bool ReadBufferFromRegistry(DWORD buffer[], HKEY hkey, const char *keyName, const char *valueName)
{
	const DWORD maxBufferSize = 8192;
	const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader);
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader)) / sizeof(KeyBindingsEntry);
	DWORD bufferSize = maxBufferSize;
	DWORD type = 0;
	DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	DWORD& count = header->numberOfEntries;

	LSTATUS status = RegGetValueA(hkey, keyName, valueName, RRF_RT_REG_BINARY, &type, &buffer[0], &bufferSize);
	if (status == ERROR_SUCCESS &&
		bufferSize >= minBufferSize &&
		count < maxCount &&
		header->version == 0 &&
		header->headerFlags == 0 &&
		count * sizeof(DWORD) + sizeof(KeyBindingsHeader) <= bufferSize &&
		_arr[count - 1] == 0)
	{
		return true;
	}
	else
	{
		memset(buffer, 0, maxBufferSize);
		count = 1;
		return false;
	}
}

static bool ReadBufferFromRegistry(DWORD buffer[])
{
	return ReadBufferFromRegistry(buffer, HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", "Scancode Map");
}

static bool DeleteRegistryKey(HKEY hkey, const char* keyName, const char *valueName)
{
	HKEY key = NULL;
	LSTATUS status = RegOpenKeyExA(hkey, keyName, 0, KEY_ALL_ACCESS, &key);
	if (status == ERROR_SUCCESS)
	{
		status = RegDeleteValueA(key, valueName);
	}
	return status == ERROR_SUCCESS;
}

static bool DeleteRegistryKey()
{
	return DeleteRegistryKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", "Scancode Map");
}

static bool WriteBufferToRegistry(DWORD buffer[], HKEY hkey, const char* keyName, const char* valueName, DWORD dwOptions)
{
	//const DWORD maxBufferSize = 8192;
	//const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD);
	//const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader)) / sizeof(KeyBindingsEntry);
	//DWORD bufferSize = maxBufferSize;
	//DWORD type = 0;
	//DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	//KeyBindingsEntry* const arr = (KeyBindingsEntry*)_arr;
	DWORD& count = header->numberOfEntries;

	HKEY key = NULL;
	bool result = false;
	LSTATUS status = RegOpenKeyExA(hkey, keyName, 0, KEY_ALL_ACCESS, &key);
	if (status == ERROR_FILE_NOT_FOUND)
	{
		status = RegCreateKeyExA(hkey, keyName, 0, NULL, dwOptions, KEY_ALL_ACCESS, NULL, &key, NULL);
	}
	if (status != ERROR_SUCCESS)
	{
		return false;
	}
	//if (count <= 1)
	//{
	//	//Delete all key bindings when there aren't any
	//	status = RegDeleteValueA(key, valueName);
	//}
	//else
	{
		status = RegSetValueExA(key, valueName, 0, REG_BINARY, (const BYTE*)&buffer[0], (count + 3) * sizeof(DWORD));
	}
	RegCloseKey(key);
	if (status == ERROR_SUCCESS)
	{
		result = true;
	}
	return result;
}

static bool WriteBufferToRegistry(DWORD buffer[])
{
	return WriteBufferToRegistry(buffer, HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", "Scancode Map", 0);
}

bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo)
{
	const DWORD maxBufferSize = 8192;
	//const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD);
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader)) / sizeof(KeyBindingsEntry);
	if (scancodeToRemap == 0)
	{
		return false;
	}
	DWORD buffer[maxBufferSize / sizeof(DWORD)];
	DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	KeyBindingsEntry* const arr = (KeyBindingsEntry*)_arr;
	DWORD &count = header->numberOfEntries;
	//note: count includes 0 termiantor at the end

	bool okay = ReadBufferFromRegistry(buffer);

	//check for scancodeToRemap, if it exists, replace it, otherwise add it to the list
	bool changed = false;
	for (DWORD i = 0;; i++)
	{
		if (arr[i].sourceKey == scancodeToRemap)
		{
			if (arr[i].destinationKey != scancodeToChangeTo)
			{
				if (scancodeToChangeTo == scancodeToRemap)
				{
					//move last element to this slot
					_arr[i] = _arr[count - 2];
					_arr[count - 2] = 0;
					count--;
				}
				else
				{
					//change last key
					arr[i].destinationKey = scancodeToChangeTo;
				}
				changed = true;
			}
			break;
		}
		else if (_arr[i] == 0)
		{
			//we got a zero or terminator
			//if we have room, assign
			if (i < maxCount - 1)
			{
				_arr[i] = ((DWORD)scancodeToChangeTo | ((DWORD)scancodeToRemap << 16));
				changed = true;
			}
			//if we're at the end, expand
			if (i == count - 1)
			{
				_arr[count] = 0;
				count++;
			}
			else
			{
				return false;
			}
			break;
		}
	}
	if (changed)
	{
		if (count <= 1)
		{
			DeleteRegistryKey();
		}
		else
		{
			return WriteBufferToRegistry(buffer);
		}
	}
	else
	{
		return true;
	}
}

WORD GetRemappedKey(WORD scancodeToRemap)
{
	const DWORD maxBufferSize = 8192;
	const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD);
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader)) / sizeof(KeyBindingsEntry);
	DWORD buffer[maxBufferSize / sizeof(DWORD)];
	DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	KeyBindingsEntry* const arr = (KeyBindingsEntry*)_arr;
	DWORD& count = header->numberOfEntries;
	//note: count includes 0 termiantor at the end

	bool okay = ReadBufferFromRegistry(buffer);
	if (!okay) return 0xFFFF;

	for (DWORD i = 0;; i++)
	{
		if (arr[i].sourceKey == scancodeToRemap)
		{
			return arr[i].destinationKey;
		}
		else if (i == count - 1)
		{
			return 0xFFFF;
		}
	}
}

//Converts an unsigned int into a numeric string, along with a null terminator.
//Returns NULL if buffer does not have room for the whole string.
//Otherwise returns the position after the end of the number
char* my_itoa(char* str, size_t bufferSize, int position, unsigned int value)
{
	char temp[12];
	char* p = &temp[10];
	p[1] = 0;
	*p = '0';
	if (value > 0)
	{
		while (value > 0)
		{
			unsigned int modulo = value % 10;
			*p = '0' + modulo;
			p--;
			value /= 10;
		}
		p++;
	}
	size_t size2 = temp + 11 - p;
	if (position < 0)
	{
		return NULL;
	}
	if (size2 + position + 1 < bufferSize)
	{
		memcpy(str + position, p, size2 + 1);
		return str + position + size2;
	}
	return NULL;
}

WORD UpdateVolatile()
{
	//We keep a copy of "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout\\Scancode Map"
	//in the registry at "HKCU\\Volatile Environment\\<session id>\\NoCopilotKey-Scancode Map"
	//so we can try to guess what the key remapping was at login time rather than what's there now.
	//Because it's a volatile key, it is automatically deleted from the registry on logout/restart.
	const DWORD maxBufferSize = 8192;
	const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD);
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader)) / sizeof(KeyBindingsEntry);
	DWORD buffer[maxBufferSize / sizeof(DWORD)];
	DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	KeyBindingsEntry* const arr = (KeyBindingsEntry*)_arr;
	DWORD& count = header->numberOfEntries;

	char regKeyName[MAX_PATH] = {};
	strcpy(regKeyName, "Volatile Environment\\");
	DWORD sessionId = 0;
	ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
	my_itoa(regKeyName, MAX_PATH, strlen(regKeyName), sessionId);
	
	bool hasBeenRead = ReadBufferFromRegistry(buffer, HKEY_CURRENT_USER, regKeyName, "NoCopilotKey-Scancode Map");
	if (!hasBeenRead)
	{
		hasBeenRead = ReadBufferFromRegistry(buffer);
		WriteBufferToRegistry(buffer, HKEY_CURRENT_USER, regKeyName, "NoCopilotKey-Scancode Map", REG_OPTION_VOLATILE);
	}
	for (DWORD i = 0;; i++)
	{
		if (arr[i].sourceKey == SCANCODE_F23)
		{
			return arr[i].destinationKey;
		}
		else if (i == count - 1)
		{
			return 0xFFFF;
		}
	}
}
