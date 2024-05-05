#pragma once

#include <stdbool.h>
#include <stdint.h>

bool handleScancode(const uint32_t ps2scancode);
uint32_t get_last_scancode();
