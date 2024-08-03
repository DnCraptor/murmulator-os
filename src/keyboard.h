#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "ff.h"

typedef struct kbd_state {
    bool bCtrlPressed;
    bool bAltPressed;
    bool bDelPressed;
    bool bLeftShift;
    bool bRightShift;
    bool bRus;
    bool bCapsLock;
    bool bTabPressed;
    bool bPlusPressed;
    bool bMinusPressed;
    uint32_t input;
} kbd_state_t;

kbd_state_t* get_kbd_state(void);

bool handleScancode(const uint32_t ps2scancode);
char __getch(void);
int __getc(FIL*);
char getch_now(void);

typedef bool (*scancode_handler_t)(const uint32_t);
scancode_handler_t get_scancode_handler();
void set_scancode_handler(scancode_handler_t h);

typedef bool (*cp866_handler_t)(const char, uint32_t);
cp866_handler_t get_cp866_handler();
void set_cp866_handler(cp866_handler_t h);
