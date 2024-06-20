#pragma once

#include <stdbool.h>
#include <stdint.h>

bool handleScancode(const uint32_t ps2scancode);
uint32_t get_last_scancode();
char __getc(void);

typedef bool (*scancode_handler_t)(const uint32_t);
scancode_handler_t get_scancode_handler();
void set_scancode_handler(scancode_handler_t h);

typedef bool (*cp866_handler_t)(const char, uint32_t);
cp866_handler_t get_cp866_handler();
void set_cp866_handler(cp866_handler_t h);
