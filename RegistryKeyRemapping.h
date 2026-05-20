#pragma once
#include <stdint.h>
typedef uint16_t WORD;

struct KeyBindingsEntry
{
	WORD sourceKey;
	WORD destinationKey;
};

int ReadKeyBindings(KeyBindingsEntry arr[], int maxArrCount);
bool WriteKeyBindings(KeyBindingsEntry arr[], int count);
bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo);
