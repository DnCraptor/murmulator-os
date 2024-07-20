#include <pico/platform.h>
#include <hardware/watchdog.h>
#include "keyboard.h"
#include "ps2.h"
#include "ff.h"
#include "cmd.h"
#include "overclock.h"

static bool bCtrlPressed = false;
static bool bAltPressed = false;
static bool bDelPressed = false;
static bool bLeftShift = false;
static bool bRightShift = false;
static bool bRus = false;
static bool bCapsLock = false;
static bool bTabPressed = false;
static bool bPlusPressed = false;
static bool bMinusPressed = false;
static uint32_t input = 0;

uint32_t get_last_scancode() {
    return input;
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

static volatile int __c = 0;
bool __time_critical_func(handleScancode)(const uint32_t ps2scancode) {
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
    input = ps2scancode;
    if (ps2scancode == 0xE048 || ps2scancode == 0x48 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        input = 0;
        if (cp866_handler) cp866_handler(17 /*up*/, ps2scancode);
        __c = 17;
        return true;
    }
    if (ps2scancode == 0xE050 || ps2scancode == 0x50 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        input = 0;
        if (cp866_handler) cp866_handler(18 /*down*/, ps2scancode);
        __c = 18;
        return true;
    }
    if (ps2scancode == 0xE09C) {
        if (cp866_handler) cp866_handler('\n', ps2scancode);
        __c = '\n';
        return true;
    }
    switch ((uint8_t)input & 0xFF) {
        case 0x81: // ESC
            if (cp866_handler) cp866_handler(0x1B /*ESC*/, ps2scancode);
            __c = 0x1B;
            return true;
        case 0x1D:
            bCtrlPressed = true;
            if (bRightShift || bLeftShift) bRus = !bRus;
            break;
        case 0x9D:
            bCtrlPressed = false;
            break;
        case 0x38:
            bAltPressed = true;
            break;
        case 0xB8:
            bAltPressed = false;
            s = strlen(tricode);
            if (s) {
                c = tricode2c(tricode, s);
                __c = c;
                return true;
            }
            break;
        case 0x53:
            bDelPressed = true;
            break;
        case 0xD3:
            bDelPressed = false;
            break;
        case 0x2A:
            bLeftShift = true;
            if (bCtrlPressed) bRus = !bRus;
            break;
        case 0xAA:
            bLeftShift = false;
            break;
        case 0x36:
            bRightShift = true;
            if (bCtrlPressed) bRus = !bRus;
            break;
        case 0xB6:
            bRightShift = false;
            break;
        case 0x3A:
            bCapsLock = !bCapsLock;
            break;
        case 0x0E:
            if (cp866_handler) cp866_handler(8 /*BS*/, ps2scancode);
            __c = 8;
            return true;
        case 0x0F:
            bTabPressed = true;
            break;
        case 0x8F:
            bTabPressed = false;
            break;
        case 0x0C: // -
        case 0x4A: // numpad -
            bMinusPressed = true;
            break;
        case 0x8C: // -
        case 0xCA: // numpad 
            bMinusPressed = false;
            break;
        case 0x0D: // +=
        case 0x4E: // numpad +
            bPlusPressed = true;
            break;
        case 0x8D: // += 82?
        case 0xCE: // numpad +
            bPlusPressed = false;
            break;
        default:
            break;
    }
    if (bCtrlPressed && bAltPressed && bDelPressed) {
        watchdog_enable(100, true);
        return true;
    }
    if (bTabPressed && bCtrlPressed) {
        if (bPlusPressed) {
            set_overclocking(get_overclocking_khz() + 1000);
            overclocking();
        }
        if (bMinusPressed) {
            set_overclocking(get_overclocking_khz() - 1000);
            overclocking();
        }
    }
    if (c || input < 86) {
        if (!c) {
            c = (bRus ?
            (
                !bCapsLock ?
                ((bRightShift || bLeftShift) ? scan_code_2_cp866_rA : scan_code_2_cp866_ra) :
                ((bRightShift || bLeftShift) ? scan_code_2_cp866_raCL : scan_code_2_cp866_rACL)
            ) : (
                !bCapsLock ?
                ((bRightShift || bLeftShift) ? scan_code_2_cp866_A : scan_code_2_cp866_a) :
                ((bRightShift || bLeftShift) ? scan_code_2_cp866_aCL : scan_code_2_cp866_ACL)
            ))[input];
        }
        if (c) {
            if (bAltPressed && c >= '0' && c <= '9') {
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
        if (bCtrlPressed && ps2scancode == 0x2E) {
            __c = -1; // EOF
        } else {
            __c = c;
        }
    }
    return true;
}

#include "FreeRTOS.h"
#include "task.h"

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
