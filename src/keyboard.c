#include <pico/platform.h>
#include <hardware/watchdog.h>
#include "keyboard.h"
#include "ps2.h"
#include "ff.h"
#include "cmd.h"
#include "overclock.h"

static kbd_state_t ks = { 0 };

kbd_state_t* get_kbd_state(void) {
    return &ks;
}

static char scan_code_2_cp866_a[] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 ,'\t', // 0D - TAB
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n',  0 , 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`',  0 ,'\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_A[] = {
     0 ,  0 , '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  0 ,'\t', // 0D - TAB
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}','\n',  0 , 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  0 , '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_aCL[] = {
     0 ,  0 , '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  0 ,'\t', // 0D - TAB
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n',  0 , 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '~',  0 , '|', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', '<', '>', '?',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_ACL[] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 ,'\t', // 0D - TAB
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}','\n',  0 , 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';','\'', '`',  0 ,'\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};

static char scan_code_2_cp866_ra[] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 ,'\t', // 0D - TAB
   0xA9,0xE6,0xE3,0xAA,0xA5,0xAD,0xA3,0xE8,0xE9,0xA7,0xE5,0xEA,'\n',  0 ,0xE4,0xEB,
   0xA2,0xA0,0xAF,0xE0,0xAE,0xAB,0xA4,0xA6,0xED,0xf1,  0 ,'\\',0xEF,0xE7,0xE1,0xAC,
   0xA8,0xE2,0xEC,0xA1,0xEE, ',',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_rA[] = {
     0 ,  0 , '!', '"',0xfc, ';', '%', ':', '?', '*', '(', ')', '_', '+',  0 ,'\t', // 0D - TAB
   0x89,0x96,0x93,0x8A,0x85,0x8D,0x83,0x98,0x99,0x87,0x95,0x9A,'\n',  0 ,0x94,0x9B,
   0x82,0x80,0x8F,0x90,0x8E,0x8B,0x84,0x86,0x9D,0xf0,  0 , '/',0x9F,0x97,0x91,0x8C,
   0x88,0x92,0x9C,0x81,0x9E, '.',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_raCL[] = {
     0 ,  0 , '!', '"',0xfc, ';', '%', ':', '?', '*', '(', ')', '_', '+',  0 ,'\t', // 0D - TAB
   0xA9,0xE6,0xE3,0xAA,0xA5,0xAD,0xA3,0xE8,0xE9,0xA7,0xE5,0xEA,'\n',  0 ,0xE4,0xEB,
   0xA2,0xA0,0xAF,0xE0,0xAE,0xAB,0xA4,0xA6,0xED,0xf1,  0 , '/',0xEF,0xE7,0xE1,0xAC,
   0xA8,0xE2,0xEC,0xA1,0xEE, ',',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};
static char scan_code_2_cp866_rACL[] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 ,'\t', // 0D - TAB
   0x89,0x96,0x93,0x8A,0x85,0x8D,0x83,0x98,0x99,0x87,0x95,0x9A,'\n',  0 ,0x94,0x9B,
   0x82,0x80,0x8F,0x90,0x8E,0x8B,0x84,0x86,0x9D,0xf0,  0 ,'\\',0x9F,0x97,0x91,0x8C,
   0x88,0x92,0x9C,0x81,0x9E, '.',  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  '/', 0 
};

static char tricode2c(char tricode[4], size_t s) {
    unsigned int r = 0;
    for (int pos = s - 1; pos >= 0; pos--) {
        int bs = s - 1 - pos;
        unsigned int dShift = bs == 2 ? 100 : (bs == 1 ? 10 : 1); // faster than exp
        r += (tricode[pos] - '0') * dShift;
    }
    tricode[0] = 0;
    return (char)r & 0xFF;
}

static volatile scancode_handler_t scancode_handler = 0;
scancode_handler_t get_scancode_handler() {
    return scancode_handler;
}
void set_scancode_handler(scancode_handler_t h) {
    scancode_handler = h;
}
static cp866_handler_t cp866_handler = 0;
cp866_handler_t get_cp866_handler() {
    return cp866_handler;
}
void set_cp866_handler(cp866_handler_t h) {
    cp866_handler = h;
}

#define CHAR_CODE_BS    8
#define CHAR_CODE_UP    17
#define CHAR_CODE_DOWN  18
#define CHAR_CODE_ENTER '\n'
#define CHAR_CODE_TAB   '\t'
#define CHAR_CODE_ESC   0x1B

static volatile int __c = 0;
bool __scratch_y("kbd_driver_text") handleScancode(const uint32_t ps2scancode) {
    if (scancode_handler) {
        if (scancode_handler(ps2scancode)) {
            return true;
        }
    }
    FIL f = { 0 };
    UINT br;
    static char tricode[4] = {0};
    size_t s;
    char c = 0;
    ks.input = ps2scancode;
    if (ps2scancode == 0xE048 || ps2scancode == 0x48 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        ks.input = 0;
        if (cp866_handler) cp866_handler(CHAR_CODE_UP, ps2scancode);
        __c = CHAR_CODE_UP;
        return true;
    }
    if (ps2scancode == 0xE050 || ps2scancode == 0x50 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        ks.input = 0;
        if (cp866_handler) cp866_handler(CHAR_CODE_DOWN, ps2scancode);
        __c = CHAR_CODE_DOWN;
        return true;
    }
    if (ps2scancode == 0xE09C) {
        if (cp866_handler) cp866_handler(CHAR_CODE_ENTER, ps2scancode);
        __c = CHAR_CODE_ENTER;
        return true;
    }
    switch ((uint8_t)ks.input & 0xFF) {
        case 0x81: // ESC
            if (cp866_handler) cp866_handler(CHAR_CODE_ESC, ps2scancode);
            __c = CHAR_CODE_ESC;
            return true;
        case 0x1D:
            ks.bCtrlPressed = true;
            if (ks.bRightShift || ks.bLeftShift) ks.bRus = !ks.bRus;
            break;
        case 0x9D:
            ks.bCtrlPressed = false;
            break;
        case 0x38:
            ks.bAltPressed = true;
            break;
        case 0xB8:
            ks.bAltPressed = false;
            s = strlen(tricode);
            if (s) {
                c = tricode2c(tricode, s);
                __c = c;
                return true;
            }
            break;
        case 0x53:
            ks.bDelPressed = true;
            break;
        case 0xD3:
            ks.bDelPressed = false;
            break;
        case 0x2A:
            ks.bLeftShift = true;
            if (ks.bCtrlPressed) ks.bRus = !ks.bRus;
            break;
        case 0xAA:
            ks.bLeftShift = false;
            break;
        case 0x36:
            ks.bRightShift = true;
            if (ks.bCtrlPressed) ks.bRus = !ks.bRus;
            break;
        case 0xB6:
            ks.bRightShift = false;
            break;
        case 0x3A:
            ks.bCapsLock = !ks.bCapsLock;
            break;
        case 0x0E:
            if (cp866_handler) cp866_handler(CHAR_CODE_BS, ps2scancode);
            __c = CHAR_CODE_BS;
            return true;
        case 0x0F:
            ks.bTabPressed = true;
            break;
        case 0x8F:
            ks.bTabPressed = false;
            break;
        case 0x0C: // -
        case 0x4A: // numpad -
            ks.bMinusPressed = true;
            break;
        case 0x8C: // -
        case 0xCA: // numpad 
            ks.bMinusPressed = false;
            break;
        case 0x0D: // +=
        case 0x4E: // numpad +
            ks.bPlusPressed = true;
            break;
        case 0x8D: // += 82?
        case 0xCE: // numpad +
            ks.bPlusPressed = false;
            break;
        default:
            break;
    }
    if (ks.bCtrlPressed && ks.bAltPressed && ks.bDelPressed) {
        watchdog_enable(1, true);
        return true;
    }
    if (ks.bTabPressed && ks.bCtrlPressed) {
        if (ks.bPlusPressed) {
            set_overclocking(get_overclocking_khz() + 1000);
            overclocking();
        }
        if (ks.bMinusPressed) {
            set_overclocking(get_overclocking_khz() - 1000);
            overclocking();
        }
    }
    if (c || ks.input < 86) {
        if (!c) {
            c = (ks.bRus ?
            (
                !ks.bCapsLock ?
                ((ks.bRightShift || ks.bLeftShift) ? scan_code_2_cp866_rA : scan_code_2_cp866_ra) :
                ((ks.bRightShift || ks.bLeftShift) ? scan_code_2_cp866_raCL : scan_code_2_cp866_rACL)
            ) : (
                !ks.bCapsLock ?
                ((ks.bRightShift || ks.bLeftShift) ? scan_code_2_cp866_A : scan_code_2_cp866_a) :
                ((ks.bRightShift || ks.bLeftShift) ? scan_code_2_cp866_aCL : scan_code_2_cp866_ACL)
            ))[ks.input];
        }
        if (c) {
            if (ks.bAltPressed && c >= '0' && c <= '9') {
                s = strlen(tricode);
                if (s == 3) {
                    c = tricode2c(tricode, s);
                } else {
                    tricode[s++] = c;
                    tricode[s] = 0;
                    return true;
                }
            }
        }
        if (cp866_handler) cp866_handler(c, ps2scancode);
        if (ks.bCtrlPressed && ps2scancode == 0x2E) {
            __c = -1; // EOF
        } else {
            __c = c;
        }
    }
    return true;
}

#include "FreeRTOS.h"
#include "task.h"

char getch_now(void) {
    char c = __c & 0xFF;
    __c = 0;
    return c;
}

char __getch(void) {
    while(!__c) vTaskDelay(50); // TODO: Queue ?
    char c = __c & 0xFF;
    __c = 0;
    return c;
}

int __getc(FIL* f) {
    if (f == NULL) {
        while(!__c) vTaskDelay(50); // TODO: Queue ?
        int c = __c;
        __c = 0;
        return c;
    }
//    if (f_eof(f)) return -1; // TODO: ensure
    char res[1] = {0};
    UINT br;
    if (FR_OK != f_read(f, res, 1, &br)) {
        return -1;
    }
    return res[0];
}
