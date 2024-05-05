#include <cstdlib>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "graphics.h"

extern "C" {
#include "ps2.h"
#include "ff.h"
//#include "usb.h"
#include "FreeRTOS.h"
#include "task.h"

#include "tests.h"
#include "sys_table.h"
#include "portable.h"

#define M_OS_APP_TABLE_BASE ((size_t*)0x10002000ul)
typedef int (*boota_ptr_t)( void *argv );

bool __not_in_flash_func(load_firmware)(const char pathname[256]);

void vAppTask(void *pv) {
    int res = ((boota_ptr_t)M_OS_APP_TABLE_BASE[0])(pv); // TODO:
    vTaskDelete( NULL );
    // TODO: ?? return res;
}

inline static void run_app(char * name) {
    xTaskCreate(vAppTask, name, configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}

// TODO: .h
void fgoutf(FIL *f, const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));

#include <stdarg.h>

static char* redirect2 = NULL;

void fgoutf(FIL *f, const char *__restrict str, ...) {
    char buf[512]; // TODO: some const?
    va_list ap;
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!redirect2) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
}

}

#include "nespad.h"

static FATFS fs;
semaphore vga_start_semaphore;
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)

typedef struct {
    // 32 byte header
    uint32_t magicStart0;
    uint32_t magicStart1;
    uint32_t flags;
    uint32_t targetAddr;
    uint32_t payloadSize;
    uint32_t blockNo;
    uint32_t numBlocks;
    uint32_t fileSize; // or familyID;
    uint8_t data[476];
    uint32_t magicEnd;
} UF2_Block_t;


static uint8_t* SCREEN = 0;

static bool bCtrlPressed = false;
static bool bAltPressed = false;
static bool bDelPressed = false;
static bool bLeftShift = false;
static bool bRightShift = false;
static bool bRus = false;
static bool bCapsLock = false;
static uint32_t input = 0;

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

static char cmd[512] = { 0 };
static char cmd_t[512] = { 0 }; // tokenised string
static int cmd_pos = 0;
static char tolower_token(char t) {
    if (t == ' ') {
        return 0;
    }
    if (t >= 'A' && t <= 'Z') {
        return t + ('a' - 'A');
    }
    if (t >= 0x80 && t <= 0x8F /* А-П */) {
        return t + (0xA0 - 0x80);
    }
    if (t >= 0x90 && t <= 0x9F /* Р-Я */) {
        return t + (0xE0 - 0x90);
    }
    if (t >= 0xF0 && t <= 0xF6) return t + 1; // Ё...
    return t;
}

static bool bAppend = false;
static int tokenize_cmd() {
    redirect2 = NULL;
    bAppend = false;
    if (cmd[0] == 0) {
        cmd_t[0] = 0;
        return 0;
    }
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = cmd;
    char* t2 = cmd_t;
    while(*t1) {
        char c = tolower_token(*t1++);
        if (c == '>') {
            *(t1-1) = 0;
            redirect2 = t1;
            if (*redirect2 == '>') {
                *redirect2 = 0;
                redirect2++;
                bAppend = true;
            }
            // ignore trailing spaces
            while(*redirect2 && *redirect2 == ' ') {
                redirect2++;
            }
            // trim trailing spaces
            char *end = redirect2;
            while(*end && *end != ' ') {
                end++;
            }
            *end = 0;
            // TODO: 2nd param after ">" ?
            // TODO: support quotes
            break;
        }
        if (inSpace) {
            //if (!c) {} // still in space
            if(c) { // token started
                inSpace = 0;
                inTokenN++; // new token
            }
        } else if(!c) { inSpace = true; } // not in space, after the token
        //else {} // still in token
        *t2++ = c;
    }
    *t2 = 0;
    return inTokenN;
}
static char* next_token(char* t) {
    char *t1 = t + strlen(t);
    while(!*t1++);
    return t1 - 1;
}
static void backspace() {
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    cmd[--cmd_pos] = 0;
    gbackspace();
}

static void type(FIL *f, char *fn) {
    FIL f2;
    if (f_open(&f2, fn, FA_READ) != FR_OK) {
        goutf("Unable to open file: '%s'\n", fn);
        return;
    }
    char buf[512];
    UINT rb;
    while(f_read(&f2, buf, 512, &rb) == FR_OK && rb > 0) {
        buf[rb] = 0;
        fgoutf(f, buf);
    }
    f_close(&f2);
}

static void del(char *d) {
    if (f_unlink(d) != FR_OK) {
        goutf("Unable to unlink: '%s'\n", d);
    }
}

static char curr_dir[512] = "MOS"; // TODO: configure start dir

static void cp(char *f1, char* f2) {
    FIL fil1, fil2;
    if(f_open(&fil1, f1, FA_READ) != FR_OK) {
        goutf("Unable to open file: '%s'\n", f1);
        return;
    }
    if (f_open(&fil2, f2, FA_CREATE_NEW | FA_WRITE) != FR_OK) {
        goutf("Unable to open file to write: '%s'\n", f2);
        f_close(&fil1);
        return;
    }
    char buf[512];
    UINT rb;
    while(f_read(&fil1, buf, 512, &rb) == FR_OK && rb > 0) {
        if (f_write(&fil2, buf, rb, &rb) != FR_OK) {
            goutf("Unable to write to file: '%s'\n", f2);
        }
    }
    f_close(&fil2);
    f_close(&fil1);
}

static void cd(char *d) {
    FILINFO fileinfo;
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        strcpy(curr_dir, d);
    } else if (f_stat(d, &fileinfo) != FR_OK || !(fileinfo.fattrib & AM_DIR)) {
        goutf("Unable to find directory: '%s'\n", d);
    } else {
        strcpy(curr_dir, d);
    }
}

static void mkdir(char *d) {
    if (f_mkdir(d) != FR_OK) {
        goutf("Unable to mkdir: '%s'\n", d);
    }
}

static int rmdir(char *d) {
    DIR dir;
    FILINFO fileInfo;
    if (FR_OK != f_opendir(&dir, d)) {
        goutf("Failed to open directory: '%s'\n", d);
        return 0;
    }
    size_t total_files = 0;
    char* t = (char*)pvPortMalloc(512);
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        size_t s = strlen(d);
        strncpy(t, d, 512);
        t[s] = '/';
        strncpy(t + s + 1, fileInfo.fname, 511 - s);
        if(fileInfo.fattrib & AM_DIR) {
            total_files += rmdir(t);
        }
        if (f_unlink(t) == FR_OK)
            total_files++;
        else
            goutf("Failed to remove file: '%s'\n", t);
    }
    vPortFree(t);
    f_closedir(&dir);
    if (f_unlink(d) == FR_OK) {
        total_files++;
    }
    goutf("    Total: %d files removed\n", total_files);
    return total_files;
}

static void dir(FIL *f, char *d) {
    DIR dir;
    FILINFO fileInfo;
    if (FR_OK != f_opendir(&dir, d)) {
        goutf("Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        fgoutf(f, "D ..\n");
    }
    int total_files = 0;
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        fgoutf(f, fileInfo.fattrib & AM_DIR ? "D " : "  ");
        fgoutf(f, "%s\n", fileInfo.fname);
        total_files++;
    }
    f_closedir(&dir);
    fgoutf(f, "    Total: %d files\n", total_files);
}

static void type_char(char c) {
    goutf("%c", c); // TODO: putc
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
}

static char* next_on(char* l, char *bi) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        l++;
        b++;
    }
    return *l == 0 ? b : bi;
}

static void tabPressed() {
    char * p = cmd;
    char * p2 = cmd;
    while (*p) {
        if (*p++ == ' ') {
            p2 = p;
        }
    }
    p = p2;
    char * p3 = p2;
    while (*p3) {
        if (*p3++ == '/') {
            p2 = p3;
        }
    }
    if (p != p2) {
        strncpy(cmd_t, p, p2 - p);
        cmd_t[p2 - p] = 0;
    }
    DIR dir;
    FILINFO fileInfo;
    //goutf("\nDIR: %s\n", p != p2 ? cmd_t : curr_dir);
    if (FR_OK != f_opendir(&dir, p != p2 ? cmd_t : curr_dir)) {
        return;
    }
    int total_files = 0;
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        p3 = next_on(p2, fileInfo.fname);
        if (p3 != fileInfo.fname) {
            strcpy(cmd_t, p3);
            total_files++;
        }
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, cmd_t, total_files);
    }
    if (total_files == 1) {
        p3 = cmd_t;
        while (*p3) {
            type_char(*p3++);
        }
    } else {
        // TODO: blimp
    }
    f_closedir(&dir);
}

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

static bool exists(char* t, const char * cmd) {
    FILINFO fileinfo;
    bool r = f_stat(cmd, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
    if (r) {
        strncpy(t, cmd, 511);
        return r;
    }
    size_t s = strlen(curr_dir);
    strncpy(t, curr_dir, 511);
    t[s] = '/';
    strncpy(t + s + 1, cmd, 511 - s);
    return f_stat(t, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
}

static char cmd_history_file[] = "\\MOS\\.cmd_history"; // TODO: config
static int cmd_history_idx = -2;

extern "C" {
bool __time_critical_func(handleScancode)(const uint32_t ps2scancode) {
    FIL f = { 0 };
    UINT br;
    static char tricode[4] = {0};
    char tmp[32];
    size_t s;
    char c = 0;
    snprintf(tmp, 32, "%ph", ps2scancode);
    draw_text(tmp, 0, 29, 13, 1);
    input = ps2scancode;
    if (ps2scancode == 0xE048 || ps2scancode == 0x48 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        input = 0;
        while(cmd_pos) {
            cmd[--cmd_pos] = 0;
            gbackspace();
        }
        cmd_history_idx--;
        f_open(&f, cmd_history_file, FA_READ);
        size_t j = 0;
        int idx = 0;
        while(f_read(&f, cmd_t, 512, &br) == FR_OK && br) {
            for(size_t i = 0; i < br; ++i) {
                char t = cmd_t[i];
                if(t == '\n') { // next line
                    cmd[j] = 0;
                    j = 0;
                    if(cmd_history_idx == idx)
                        break;
                    idx++;
                } else {
                    cmd[j++] = t;
                    cmd_pos = j;
                }
            }
        }
        f_close(&f);
        if (cmd_history_idx < 0) cmd_history_idx = idx;
        goutf(cmd);
    }
    if (ps2scancode == 0xE050 || ps2scancode == 0x50 && !(get_leds_stat() & PS2_LED_NUM_LOCK)) {
        input = 0;
        while(cmd_pos) {
            cmd[--cmd_pos] = 0;
            gbackspace();
        }
        if (cmd_history_idx == -2) cmd_history_idx = -1;
        cmd_history_idx++;
        f_open(&f, cmd_history_file, FA_READ);
        size_t j = 0;
        int idx = 0;
        while(f_read(&f, cmd_t, 512, &br) == FR_OK && br) {
            for(size_t i = 0; i < br; ++i) {
                char t = cmd_t[i];
                if(t == '\n') { // next line
                    cmd[j] = 0;
                    j = 0;
                    if(cmd_history_idx == idx)
                        break;
                    idx++;
                } else {
                    cmd[j++] = t;
                    cmd_pos = j;
                }
            }
        }
        f_close(&f);
        goutf(cmd);
    }
    switch ((uint8_t)input & 0xFF) {
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
            if (s) c = tricode2c(tricode, s);
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
            backspace();
            break;
        default:
            break;
    }
    if (bCtrlPressed && bAltPressed && bDelPressed) {
        watchdog_enable(100, true);
        return true;
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
            if (c == '\t') {
                tabPressed();
                c = 0;
            }
            else goutf("%c", c); // TODO: putc
        }
        if (c == '\n') {
            if (cmd_pos > 0) { // history
                f_open(&f, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
                f_write(&f, cmd, cmd_pos, &br);
                f_write(&f, "\n", 1, &br);
                f_close(&f);
            }
            int tokens = tokenize_cmd();
            if (redirect2) {
                if (bAppend) {
                    FILINFO fileinfo;
                    if (f_stat(redirect2, &fileinfo) != FR_OK) {
                        goutf("Unable to find file: '%s'\n", redirect2);
                        goto t;
                    } else {
                        if (f_open(&f, redirect2, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND) != FR_OK) {
                            f_lseek(&f, fileinfo.fsize);
                            goutf("Unable to open file: '%s'\n", redirect2);
                        }
                    }
                } else {
t:
                    if (f_open(&f, redirect2, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
                        goutf("Unable to open file: '%s'\n", redirect2);
                    }
                }
            }
            if (strcmp("cls", cmd_t) == 0 ) {
                clrScr(1);
                graphics_set_con_pos(0, 0);
                graphics_set_con_color(7, 0);
            } else if (strcmp("dir", cmd_t) == 0 || strcmp("ls", cmd_t) == 0) {
                dir(&f, tokens == 1 ? curr_dir : (char*)cmd + (next_token(cmd_t) - cmd_t));
            } else if (strcmp("rm", cmd_t) == 0 || strcmp("del", cmd_t) == 0 || strcmp("era", cmd_t) == 0) {
                if (tokens == 1) {
                    goutf("Unable to remove nothing\n");
                } else {
                    del((char*)cmd + (next_token(cmd_t) - cmd_t));
                }
            } else if (strcmp("cd", cmd_t) == 0) {
                if (tokens == 1) {
                    goutf("Unable to change directoy to nothing\n");
                } else {
                    cd((char*)cmd + (next_token(cmd_t) - cmd_t));
                }
            } else if (strcmp("cp", cmd_t) == 0 || strcmp("copy", cmd_t) == 0) {
                if (tokens < 3) {
                    goutf("Usage: %s <file1> <file2>\n", cmd_t);
                } else {
                    char* pt1 = next_token(cmd_t);
                    char* pt2 = next_token(pt1);
                    cp(pt1, (char*)cmd + (pt2 - cmd_t));
                }
            } else if (strcmp("mkdir", cmd_t) == 0) {
                if (tokens == 1) {
                    goutf("Unable to make directoy with no name\n");
                } else {
                    mkdir((char*)cmd + (next_token(cmd_t) - cmd_t));
                }
            } else if (strcmp("rmdir", cmd_t) == 0) {
                if (tokens == 1) {
                    goutf("Unable to remove directoy with no name\n");
                } else {
                    rmdir((char*)cmd + (next_token(cmd_t) - cmd_t));
                }
            } else if (strcmp("cat", cmd_t) == 0 || strcmp("type", cmd_t) == 0) {
                type(&f, tokens == 1 ? curr_dir : (char*)cmd + (next_token(cmd_t) - cmd_t));
            } else {
                char t[512] = {0};
                if (exists(t, cmd_t)) {
                    if (load_firmware(t)) {
                        run_app(t);
                    } else {
                        goutf("Unable to load application: '%s'\n", cmd_t);
                    }
                } else {
                    goutf("Illegal command: '%s'\n", cmd);
                }
            }
            if (redirect2) {
                f_close(&f);
            }
            goutf("%s>", curr_dir);
            cmd[0] = 0;
            cmd_pos = 0;
        } else if (c) {
            if (cmd_pos >= 512) {
                // TODO: blimp
            }
            cmd[cmd_pos++] = c;
            cmd[cmd_pos] = 0;
        }
    }
    return true;
}
}

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    const auto buffer = (uint8_t *)SCREEN;
    graphics_set_buffer(buffer, TEXTMODE_COLS, TEXTMODE_ROWS);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(0, 0);
    graphics_set_mode(TEXTMODE_DEFAULT);
    clrScr(1);

    sem_acquire_blocking(&vga_start_semaphore);
    // 60 FPS loop
#define frame_tick (16666)
    uint64_t tick = time_us_64();
#ifdef TFT
    uint64_t last_renderer_tick = tick;
#endif
    uint64_t last_input_tick = tick;
    while (true) {
#ifdef TFT
        if (tick >= last_renderer_tick + frame_tick) {
            refresh_lcd();
            last_renderer_tick = tick;
        }
#endif
        // Every 5th frame
        if (tick >= last_input_tick + frame_tick * 5) {
    ///        nespad_read();
            last_input_tick = tick;
        }
        tick = time_us_64();

        tight_loop_contents();
    }

    __unreachable();
}

/*
void __always_inline run_application() {
    multicore_reset_core1();
    asm volatile (
        "mov r0, %[start]\n"
        "ldr r1, =%[vtable]\n"
        "str r0, [r1]\n"
        "ldmia r0, {r0, r1}\n"
        "msr msp, r0\n"
        "bx r1\n"
        :: [start] "r" (0x10002000), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
    );
    __unreachable();
}
*/

bool __not_in_flash_func(load_firmware)(const char pathname[256]) {
    UINT bytes_read = 0;
    FIL file;
    if (FR_OK != f_open(&file, pathname, FA_READ)) {
        return false;
    }

    uint8_t* buffer = (uint8_t*)pvPortMalloc(FLASH_SECTOR_SIZE);
    UF2_Block_t* uf2 = (UF2_Block_t*)pvPortMalloc(sizeof(UF2_Block_t));

    uint32_t flash_target_offset = 0x2000;
    uint32_t data_sector_index = 0;

    multicore_lockout_start_blocking();
    const uint32_t ints = save_and_disable_interrupts();
    int i = 0;
    do {
        f_read(&file, uf2, sizeof(UF2_Block_t), &bytes_read); // err?
       // snprintf(tmp, 80, "#%d (%d) %ph", uf2->blockNo, uf2->payloadSize, uf2->targetAddr);
       // draw_text(tmp, 0, uf2->blockNo % 30, 7, 0);
        memcpy(buffer + data_sector_index, uf2->data, uf2->payloadSize);
        data_sector_index += uf2->payloadSize;
        if (flash_target_offset == 0)
            flash_target_offset = uf2->targetAddr - XIP_BASE;
            // TODO: order?

        if (data_sector_index == FLASH_SECTOR_SIZE || bytes_read == 0) {
            data_sector_index = 0;
            //подмена загрузчика boot2 прошивки на записанный ранее
        //    if (flash_target_offset == 0) {
        //        memcpy(buffer, (uint8_t *)XIP_BASE, 256);
        //    }
            flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE);
            flash_range_program(flash_target_offset, buffer, FLASH_SECTOR_SIZE);

        //    gpio_put(PICO_DEFAULT_LED_PIN, flash_target_offset & 1);
            //snprintf(tmp, 80, "#%d %ph -> %ph", uf2->blockNo, uf2->targetAddr, flash_target_offset);
            //draw_text(tmp, 0, i++, 7, 0);
        
            flash_target_offset = 0;
        }
    }
    while (bytes_read != 0);

    restore_interrupts(ints);
    multicore_lockout_end_blocking();

    gpio_put(PICO_DEFAULT_LED_PIN, false);
    f_close(&file);
    vPortFree(buffer);
    vPortFree(uf2);
    return true;
}

typedef struct __attribute__((__packed__)) {
    bool is_directory;
    bool is_executable;
    size_t size;
    char filename[79];
} file_item_t;

constexpr int max_files = 2500;
static file_item_t fileItems[max_files];

int compareFileItems(const void* a, const void* b) {
    const auto* itemA = (file_item_t *)a;
    const auto* itemB = (file_item_t *)b;
    // Directories come first
    if (itemA->is_directory && !itemB->is_directory)
        return -1;
    if (!itemA->is_directory && itemB->is_directory)
        return 1;
    // Sort files alphabetically
    return strcmp(itemA->filename, itemB->filename);
}

static inline bool isExecutable(const char pathname[256], const char* extensions) {
    const char* extension = strrchr(pathname, '.');
    if (extension == nullptr) {
        return false;
    }
    extension++; // Move past the '.' character

    const char* token = strtok((char *)extensions, "|"); // Tokenize the extensions string using '|'

    while (token != nullptr) {
        if (strcmp(extension, token) == 0) {
            return true;
        }
        token = strtok(NULL, "|");
    }

    return false;
}
#if 0
void __not_in_flash_func(filebrowser)(const char pathname[256], const char* executables) {
    bool debounce = true;
    char basepath[256];
    char tmp[TEXTMODE_COLS + 1];
    strcpy(basepath, pathname);
    constexpr int per_page = TEXTMODE_ROWS - 3;

    DIR dir;
    FILINFO fileInfo;

    if (FR_OK != f_mount(&fs, "SD", 1)) {
        draw_text("SD Card not inserted or SD Card error!", 0, 0, 12, 0);
        while (true);
    }

    while (true) {
        memset(fileItems, 0, sizeof(file_item_t) * max_files);
        int total_files = 0;

        snprintf(tmp, TEXTMODE_COLS, "SD:\\%s", basepath);
        draw_window(tmp, 0, 0, TEXTMODE_COLS, TEXTMODE_ROWS - 1);
        memset(tmp, ' ', TEXTMODE_COLS);

#ifndef TFT
        draw_text(tmp, 0, 29, 0, 0);
        auto off = 0;
        draw_text("START", off, 29, 7, 0);
        off += 5;
        draw_text(" Run at cursor ", off, 29, 0, 3);
        off += 16;
        draw_text("SELECT", off, 29, 7, 0);
        off += 6;
        draw_text(" Run previous  ", off, 29, 0, 3);
        off += 16;
        draw_text("ARROWS", off, 29, 7, 0);
        off += 6;
        draw_text(" Navigation    ", off, 29, 0, 3);
        off += 16;
        draw_text("A/F10", off, 29, 7, 0);
        off += 5;
        draw_text(" USB DRV ", off, 29, 0, 3);
#endif

        if (FR_OK != f_opendir(&dir, basepath)) {
            draw_text("Failed to open directory", 1, 1, 4, 0);
            while (true);
        }

        if (strlen(basepath) > 0) {
            strcpy(fileItems[total_files].filename, "..\0");
            fileItems[total_files].is_directory = true;
            fileItems[total_files].size = 0;
            total_files++;
        }

        while (f_readdir(&dir, &fileInfo) == FR_OK &&
               fileInfo.fname[0] != '\0' &&
               total_files < max_files
        ) {
            // Set the file item properties
            fileItems[total_files].is_directory = fileInfo.fattrib & AM_DIR;
            fileItems[total_files].size = fileInfo.fsize;
            fileItems[total_files].is_executable = isExecutable(fileInfo.fname, executables);
            strncpy(fileItems[total_files].filename, fileInfo.fname, 78);
            total_files++;
        }
        f_closedir(&dir);

        qsort(fileItems, total_files, sizeof(file_item_t), compareFileItems);

        if (total_files > max_files) {
            draw_text(" Too many files!! ", TEXTMODE_COLS - 17, 0, 12, 3);
        }

        int offset = 0;
        int current_item = 0;

        while (true) {
            sleep_ms(100);

            if (!debounce) {
                debounce = !(nespad_state & DPAD_START) && input != 0x1C;
            }

            // ESCAPE
            if (nespad_state & DPAD_B || input == 0x01) {
                return;
            }

            // F10
            if (nespad_state & DPAD_A || input == 0x44) {
                constexpr int window_x = (TEXTMODE_COLS - 40) / 2;
                constexpr int window_y = (TEXTMODE_ROWS - 4) / 2;
                draw_window("SD Cardreader mode ", window_x, window_y, 40, 4);
                draw_text("Mounting SD Card. Use safe eject ", window_x + 1, window_y + 1, 13, 1);
                draw_text("to conitinue...", window_x + 1, window_y + 2, 13, 1);

                sleep_ms(500);

                init_pico_usb_drive();

                while (!tud_msc_ejected()) {
                    pico_usb_drive_heartbeat();
                }

                int post_cicles = 1000;
                while (--post_cicles) {
                    sleep_ms(1);
                    pico_usb_drive_heartbeat();
                }
            }

            if (nespad_state & DPAD_DOWN || input == 0x50) {
                if (offset + (current_item + 1) < total_files) {
                    if (current_item + 1 < per_page) {
                        current_item++;
                    }
                    else {
                        offset++;
                    }
                }
            }

            if (nespad_state & DPAD_UP || input == 0x48) {
                if (current_item > 0) {
                    current_item--;
                }
                else if (offset > 0) {
                    offset--;
                }
            }

            if (nespad_state & DPAD_RIGHT || input == 0x4D || input == 0x7A) {
                offset += per_page;
                if (offset + (current_item + 1) > total_files) {
                    offset = total_files - (current_item + 1);
                }
            }

            if (nespad_state & DPAD_LEFT || input == 0x4B) {
                if (offset > per_page) {
                    offset -= per_page;
                }
                else {
                    offset = 0;
                    current_item = 0;
                }
            }

            if (debounce && ((nespad_state & DPAD_START) != 0 || input == 0x1C)) {
                auto file_at_cursor = fileItems[offset + current_item];

                if (file_at_cursor.is_directory) {
                    if (strcmp(file_at_cursor.filename, "..") == 0) {
                        const char* lastBackslash = strrchr(basepath, '\\');
                        if (lastBackslash != nullptr) {
                            const size_t length = lastBackslash - basepath;
                            basepath[length] = '\0';
                        }
                    }
                    else {
                        sprintf(basepath, "%s\\%s", basepath, file_at_cursor.filename);
                    }
                    debounce = false;
                    break;
                }

                if (file_at_cursor.is_executable) {
                    sprintf(tmp, "%s\\%s", basepath, file_at_cursor.filename);

                    if (load_firmware(tmp)) {
                        return;
                    }
                }
            }

            for (int i = 0; i < per_page; i++) {
                const auto item = fileItems[offset + i];
                uint8_t color = 11;
                uint8_t bg_color = 1;

                if (i == current_item) {
                    color = 0;
                    bg_color = 3;
                    memset(tmp, 0xCD, TEXTMODE_COLS - 2);
                    tmp[TEXTMODE_COLS - 2] = '\0';
                    draw_text(tmp, 1, per_page + 1, 11, 1);
                    snprintf(tmp, TEXTMODE_COLS - 2, " Size: %iKb, File %lu of %i ", item.size / 1024, offset + i + 1,
                             total_files);
                    draw_text(tmp, 2, per_page + 1, 14, 3);
                }

                const auto len = strlen(item.filename);
                color = item.is_directory ? 15 : color;
                color = item.is_executable ? 10 : color;
                //color = strstr((char *)rom_filename, item.filename) != nullptr ? 13 : color;
                memset(tmp, ' ', TEXTMODE_COLS - 2);
                tmp[TEXTMODE_COLS - 2] = '\0';
                memcpy(&tmp, item.filename, len < TEXTMODE_COLS - 2 ? len : TEXTMODE_COLS - 2);
                draw_text(tmp, 1, i + 1, color, bg_color);
            }
        }
    }
}
#endif

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(252 * KHZ, true);

    keyboard_init();
    keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    nespad_read();
    sleep_ms(50);
    // F12 Boot to USB FIRMWARE UPDATE mode
    if (nespad_state & DPAD_START || input == 0x58) {
        reset_usb_boot(0, 0);
    }

    SCREEN = (uint8_t*)pvPortMalloc(80 * 30 * 2);
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(30);

    clrScr(1);
    //graphics_set_con_color(13, 1);
    const char tmp[] = "                      ZX Murmulator (RP2040) OS v.0.0.1 Alfa                    ";
    draw_text(tmp, 0, 0, 13, 1);
    draw_text(tmp, 0, 29, 13, 1);
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0);
    goutf("SRAM   264 KB\n"
          "FLASH 2048 KB\n\n"
          "%s>", curr_dir
    );

    if (FR_OK != f_mount(&fs, "SD", 1)) {
        graphics_set_con_color(12, 0);
        goutf("SD Card not inserted or SD Card error!");
        while (true);
    }
    f_mkdir(curr_dir);
#if TESTS
    start_test();
#endif
 //   char* app = "\\MOS\\murmulator-os-demo.uf2";
//    if (0 != *((uint32_t*)M_OS_APL_TABLE_BASE)) {
        // boota (startup application) already in flash ROM
 //       run_app(app);
 //   }
//else if (load_firmware(app)) {
 //       run_app(app);
 //   }
//    snprintf(tmp, 80, "application returns #%d", res);
//    draw_text(tmp, 0, 2, 7, 0);
  //  draw_text("RUNING   vTaskStartScheduler    ", 0, 3, 7, 0);
	/* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    size_t i = 0;
    while(sys_table_ptrs[++i]); // to ensure linked (TODO: other way)

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );

    __unreachable();
}
