#include <stdint.h>
#include <stddef.h>
#include "cmd.h"
#include "graphics.h"
#include "app.h"

static char curr_dir[512] = "MOS"; // TODO: configure start dir
static char cmd[512] = { 0 };
static char cmd_t[512] = { 0 }; // tokenised string
static int cmd_pos = 0;
static char cmd_history_file[] = "\\MOS\\.cmd_history"; // TODO: config
static int cmd_history_idx = -2;
static FIL fh = {0}; // history
static FIL f0 = {0}; // output (stdout)
static FIL f1 = {0}; // stderr

FIL * get_stdout() {
    return &f0;
}
FIL * get_stderr() {
    return &f1;
}

char* next_token(char* t) {
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

static cmd_startup_ctx_t cmd_startup_ctx = { cmd, cmd_t, 0 }; // TODO: per process
cmd_startup_ctx_t * get_cmd_startup_ctx() {
    return &cmd_startup_ctx;
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
    cmd_startup_ctx.tokens = tokens;
    cmd_startup_ctx.curr_dir = curr_dir;
    cmd_startup_ctx.pstdout = &f0;
    cmd_startup_ctx.pstrerr = &f1;
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
    if (strcmp("cd", cmd_t) == 0) { // do not extern, due to internal cmd state
        if (tokens == 1) {
            fgoutf(&f1, "Unable to change directoy to nothing\n");
        } else {
            cd((char*)cmd + (next_token(cmd_t) - cmd_t));
        }
    } else {
        char t[512] = {0};
        if (exists(t, cmd_t)) {
            int len = strlen(t);
            if (len > 3 && strcmp(t, ".uf2") == 0 && load_firmware(t)) {
                run_app(t);
            } if(is_new_app(t)) {
                run_new_app(t, "main");
            } else {
                goutf("Unable to execute command: '%s'\n", t);
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
