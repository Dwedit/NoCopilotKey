#pragma once
#include <stdint.h>
typedef uint16_t WORD;

bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo);
WORD GetRemappedKey(WORD scancodeToRemap);
WORD UpdateVolatile();

static const int SCANCODE_F23 = 0x6E;
