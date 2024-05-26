#include <stdint.h>
#include <stddef.h>
#include "cmd.h"
#include "ff.h"
#include "graphics.h"
#include "app.h"
#include "elf.h"
#include "psram_spi.h"

static char curr_dir[512] = "MOS"; // TODO: configure start dir
static char cmd[512] = { 0 };
static char cmd_t[512] = { 0 }; // tokenised string
static int cmd_pos = 0;
static char cmd_history_file[] = "\\MOS\\.cmd_history"; // TODO: config
static int cmd_history_idx = -2;
static FIL fh = {0}; // history
static FIL f0 = {0}; // output (stdout)
static FIL f1 = {0}; // stderr

static char* next_token(char* t) {
    char *t1 = t + strlen(t);
    while(!*t1++);
    return t1 - 1;
}
void cmd_backspace() {
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    cmd[--cmd_pos] = 0;
    gbackspace();
}
static int history_steps() {
    size_t j = 0;
    int idx = 0;
    UINT br;
    f_open(&fh, cmd_history_file, FA_READ);
    while(f_read(&fh, cmd_t, 512, &br) == FR_OK && br) {
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
    f_close(&fh);
    return idx;
}

void cmd_up() {
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
    cmd_history_idx--;
    int idx = history_steps();
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    goutf(cmd);
}

void cmd_down() {
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps();
    goutf(cmd);
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

void cmd_tab() {
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

bool exists(char* t, const char * cmd) {
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
static char* redirect2 = NULL;

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

static void _test_psram(FIL* f) {
    uint32_t sz = psram_size();
    fgoutf(f, "PSRAM size: %d bytes\n", sz);
    uint32_t a = 0;
    uint32_t psram_begin = time_us_32();
    for (; a < sz; ++a) {
        write8psram(a, a & 0xFF);
    }
    uint32_t psram_elapsed = time_us_32() - psram_begin;
    float psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "8-bit line write speed: %f MBps\n", psram_speed);

    psram_begin = time_us_32();
    for (a = 0; a < sz; ++a) {
        if ((a & 0xFF) != read8psram(a)) {
            fgoutf(f, "8-bit read failed at %ph\n", a);
            break;
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "8-bit line read speed: %f MBps\n", psram_speed);

    psram_begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        write16psram(a, a & 0xFFFF);
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "16-bit line write speed: %f MBps\n", psram_speed);

    psram_begin = time_us_32();
    for (a = 0; a < sz; a += 2) {
        if ((a & 0xFFFF) != read16psram(a)) {
            fgoutf(f, "16-bit read failed at %ph\n", a);
            break;
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "16-bit line read speed: %f MBps\n", psram_speed);

    psram_begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        write32psram(a, a);
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "32-bit line write speed: %f MBps\n", psram_speed);

    psram_begin = time_us_32();
    for (a = 0; a < sz; a += 4) {
        if (a != read32psram(a)) {
            fgoutf(f, "32-bit read failed at %ph\n", a);
            break;
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1.0 * a / psram_elapsed;
    fgoutf(f, "32-bit line read speed: %f MBps\n", psram_speed);
}

void cmd_enter() {
    UINT br;
    if (cmd_pos > 0) { // history
        f_open(&fh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
        f_write(&fh, cmd, cmd_pos, &br);
        f_write(&fh, "\n", 1, &br);
        f_close(&fh);
    }
    int tokens = tokenize_cmd();
    if (redirect2) {
        if (bAppend) {
            FILINFO fileinfo;
            if (f_stat(redirect2, &fileinfo) != FR_OK) {
                goutf("Unable to find file: '%s'\n", redirect2);
                goto t;
            } else {
                if (f_open(&f0, redirect2, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND) != FR_OK) {
                    f_lseek(&f0, fileinfo.fsize);
                    goutf("Unable to open file: '%s'\n", redirect2);
                }
            }
        } else {
t:
            if (f_open(&f0, redirect2, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
                goutf("Unable to open file: '%s'\n", redirect2);
            }
        }
    }
    if ( strcmp("cls", cmd_t) == 0 ) {
        clrScr(1);
        graphics_set_con_pos(0, 0);
        graphics_set_con_color(7, 0);
    } else if( strcmp("psram", cmd_t) == 0 ) {
        _test_psram(&f0);
    } else if (strcmp("dir", cmd_t) == 0 || strcmp("ls", cmd_t) == 0) {
        dir(&f0, tokens == 1 ? curr_dir : (char*)cmd + (next_token(cmd_t) - cmd_t));
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
        type(&f0, tokens == 1 ? curr_dir : (char*)cmd + (next_token(cmd_t) - cmd_t));
    } else if (strcmp("elfinfo", cmd_t) == 0) {
        elfinfo(&f0, tokens == 1 ? curr_dir : (char*)cmd + (next_token(cmd_t) - cmd_t));
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
        f_close(&f0);
    }
    goutf("%s>", curr_dir);
    cmd[0] = 0;
    cmd_pos = 0;
}

void cmd_push(char c) {
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
}

char* get_curr_dir() {
    return curr_dir;
}
