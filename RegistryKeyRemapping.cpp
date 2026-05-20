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

int ReadKeyBindings(KeyBindingsEntry arr[], int maxArrCount)
{
	const DWORD maxBufferSize = 8192;
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader) - sizeof(DWORD)) / sizeof(KeyBindingsEntry);
	BYTE buffer[maxBufferSize];
	DWORD bufferSize = maxBufferSize;
	DWORD type = 0;
	LSTATUS status = RegGetValueA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", "Scancode Map", RRF_RT_REG_BINARY, &type, &buffer[0], &bufferSize);
	if (status == ERROR_FILE_NOT_FOUND)
	{
		return 0;
	}
	else if (status == ERROR_UNSUPPORTED_TYPE)
	{
		return 0;
	}
	else if (status != ERROR_SUCCESS)
	{
		return 0;
	}
	if (bufferSize < 20)
	{
		return 0;
	}
	KeyBindingsHeader* pHeader = (KeyBindingsHeader*)buffer;
	KeyBindingsEntry* pEntry = (KeyBindingsEntry*)(buffer + sizeof(KeyBindingsHeader));
	int count = pHeader->numberOfEntries;
	if (count > maxArrCount)
	{
		count = maxArrCount;
	}
	if (count < 0 || count > maxCount ||
		count * sizeof(KeyBindingsEntry) + sizeof(KeyBindingsHeader) + sizeof(DWORD) < bufferSize)
	{
		return 0;
	}
	DWORD* pTerminator = (DWORD*)(buffer + sizeof(KeyBindingsHeader) + count * sizeof(KeyBindingsEntry));
	if (*pTerminator != 0)
	{
		return 0;
	}
	for (int i = 0; i < count; i++)
	{
		arr[i] = *pEntry;
		pEntry++;
	}
	return count;
}

bool WriteKeyBindings(KeyBindingsEntry arr[], int count)
{
	const DWORD maxBufferSize = 8192;
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader) - sizeof(DWORD)) / sizeof(KeyBindingsEntry);
	if (count < 0 || count > maxCount)
	{
		return false;
	}
	bool result = false;
	HKEY key = NULL;
	LSTATUS status;
	if (count == 0)
	{
		//Delete all key bindings when count is 0
		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", 0, KEY_ALL_ACCESS, &key);
		if (status != ERROR_SUCCESS)
		{
			result = true;
			goto done;
		}
		DWORD type = 0;
		status = RegGetValueA(key, NULL, "Scancode Map", RRF_RT_ANY, &type, NULL, NULL);
		if (status == ERROR_SUCCESS)
		{
			status = RegDeleteValueA(key, "Scancode Map");
			if (status == ERROR_SUCCESS)
			{
				result = true;
				goto done;
			}
		}
		else if (status == ERROR_FILE_NOT_FOUND)
		{
			result = true;
			goto done;
		}
		goto done;
	}
	else
	{
		//Write a Key Bindings object to the registry
		BYTE buffer[maxBufferSize];
		BYTE* p = &buffer[0];
		KeyBindingsHeader* header = (KeyBindingsHeader*)p;
		header->version = 0;
		header->headerFlags = 0;
		header->numberOfEntries = count;
		p += sizeof(KeyBindingsHeader);
		KeyBindingsEntry* pEntry = (KeyBindingsEntry*)p;
		for (int i = 0; i < count; i++)
		{
			*pEntry = arr[i];
			pEntry++;
		}
		p = (BYTE*)pEntry;
		DWORD* p2 = (DWORD*)p;
		*p2 = 0;
		p += sizeof(DWORD);
		int contentSize = p - buffer;

		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", 0, KEY_ALL_ACCESS, &key);
		if (status == ERROR_ACCESS_DENIED)
		{
			goto done;
		}
		if (status == ERROR_FILE_NOT_FOUND)
		{
			status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout", 0, NULL, NULL, KEY_ALL_ACCESS, NULL, &key, NULL);
		}
		if (status != ERROR_SUCCESS)
		{
			goto done;
		}
		status = RegSetValueExA(key, "Scancode Map", 0, REG_BINARY, &buffer[0], contentSize);
		if (status == ERROR_SUCCESS)
		{
			result = true;
			goto done;
		}
		goto done;
	}
done:
	if (key != NULL)
	{
		RegCloseKey(key);
		key = NULL;
	}
	return result;
}

bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo)
{
	const DWORD maxBufferSize = 8192;
	const DWORD maxCount = (maxBufferSize - sizeof(KeyBindingsHeader) - sizeof(DWORD)) / sizeof(KeyBindingsEntry);
	if (scancodeToRemap == 0)
	{
		return false;
	}
	KeyBindingsEntry arr[maxCount];
	int count = ReadKeyBindings(arr, maxCount);
	//check for scancodeToRemap, if it exists replace it, otherwise add it to the list
	bool found = false;
	bool changed = false;
	for (int i = 0; i < count; i++)
	{
		if (arr[i].sourceKey == scancodeToRemap)
		{
			found = true;
			if (arr[i].destinationKey != scancodeToChangeTo)
			{
				if (scancodeToChangeTo == scancodeToRemap)
				{
					//swap with last element, remove last element
					KeyBindingsEntry t = arr[i];
					arr[i] = arr[count - 1];
					arr[count - 1] = t;
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
	}
	if (!found && count < maxCount && scancodeToChangeTo != scancodeToRemap)
	{
		arr[count].sourceKey = scancodeToRemap;
		arr[count].destinationKey = scancodeToChangeTo;
		count++;
		changed = true;
	}
	if (changed)
	{
		return WriteKeyBindings(arr, count);
	}
	else
	{
		return true;
	}
}
