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

bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo)
{
	const DWORD maxBufferSize = 8192;
	const DWORD minBufferSize = sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD);
	const DWORD maxCount = (maxBufferSize - 3 * sizeof(DWORD) - sizeof(KeyBindingsHeader) - sizeof(DWORD)) / sizeof(KeyBindingsEntry);
	if (scancodeToRemap == 0)
	{
		return false;
	}
	DWORD buffer[maxBufferSize / sizeof(DWORD)] = {};
	DWORD* const _arr = (DWORD*)((BYTE*)buffer + sizeof(KeyBindingsHeader));
	KeyBindingsHeader* const header = (KeyBindingsHeader*)buffer;
	KeyBindingsEntry* const arr = (KeyBindingsEntry*)_arr;
	DWORD &count = header->numberOfEntries;
	//note: count includes 0 termiantor at the end

	DWORD bufferSize = maxBufferSize;
	DWORD type = 0;
	LSTATUS status = RegGetValueA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", "Scancode Map", RRF_RT_REG_BINARY, &type, &buffer[0], &bufferSize);
	if (status == ERROR_SUCCESS &&
		bufferSize >= minBufferSize &&
		count < maxCount &&
		header->version == 0 &&
		header->headerFlags == 0 &&
		_arr[count - 1] == 0)
	{
		//validated
	}
	else
	{
		memset(buffer, 0, maxBufferSize);
		count = 1;
	}

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
			if (count + 1 < maxCount)
			{
				_arr[i] = ((DWORD)scancodeToChangeTo | ((DWORD)scancodeToRemap << 16));
				_arr[count] = 0;
				count++;
				changed = true;
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
		HKEY key = NULL;
		bool result = false;
		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", 0, KEY_ALL_ACCESS, &key);
		if (status == ERROR_FILE_NOT_FOUND)
		{
			status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", 0, NULL, NULL, KEY_ALL_ACCESS, NULL, &key, NULL);
		}
		if (status != ERROR_SUCCESS)
		{
			return false;
		}
		if (count == 0)
		{
			//Delete all key bindings when count is 0
			status = RegDeleteValueA(key, "Scancode Map");
		}
		else
		{
			status = RegSetValueExA(key, "Scancode Map", 0, REG_BINARY, (const BYTE*)&buffer[0], (count + 4) * sizeof(DWORD));
		}
		RegCloseKey(key);
		if (status == ERROR_SUCCESS)
		{
			result = true;
		}
		return result;
	}
	else
	{
		return true;
	}
}
