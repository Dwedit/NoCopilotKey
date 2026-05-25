#pragma once
#include <stdint.h>
typedef uint16_t WORD;

bool RegisterRemappedKey(WORD scancodeToRemap, WORD scancodeToChangeTo);
