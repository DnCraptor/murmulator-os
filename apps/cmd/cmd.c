#include "m-os-api.h"

FIL fh;
char* cmd_history_file;
int cmd_history_idx;
bool bExit;

static char* next_on(char* l, char *bi) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        l++;
        b++;
    }
    return *l == 0 ? b : bi;
}

inline static void concat (char* t, const char* s1 , const char* s2) {
    size_t s = strlen(s1);
    strncpy(t, s1, 511);
    t[s] = '/';
    strncpy(t + s + 1, s2, 510 - s);
}

static bool exists(cmd_startup_ctx_t* ctx, char* t) {
    char * cmd = ctx->cmd_t;
    FILINFO fileinfo;
    bool r = f_stat(cmd, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
    if (r) {
        strncpy(t, cmd, 511);
        return r;
    }
    char* dir = ctx->curr_dir;
    concat(t, dir, cmd);
    r = f_stat(t, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
    if (r) return r;
    dir = ctx->path;
    while (dir) {
        concat(t, dir, cmd);
        r = f_stat(t, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
        if (r) return r;
        dir = next_token(dir);
    }
    return false;
}

static void cmd_backspace(cmd_startup_ctx_t* ctx) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    ctx->cmd[--cmd_pos] = 0;
    gbackspace();
}

inline static int history_steps(cmd_startup_ctx_t* ctx) {
    size_t j = 0;
    int idx = 0;
    UINT br;
    f_open(&fh, cmd_history_file, FA_READ); 
    while(f_read(&fh, ctx->cmd_t, 512, &br) == FR_OK && br) {
        for(size_t i = 0; i < br; ++i) {
            char t = ctx->cmd_t[i];
            if(t == '\n') { // next line
                ctx->cmd[j] = 0;
                j = 0;
                if(cmd_history_idx == idx)
                    break;
                idx++;
            } else {
                ctx->cmd[j++] = t;
            }
        }
    }
    f_close(&fh);
    return idx;
}

inline static void type_char(cmd_startup_ctx_t* ctx, char c) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
        return;
    }
    putc(c);
    ctx->cmd[cmd_pos++] = c;
    ctx->cmd[cmd_pos] = 0;
}

inline static void cmd_tab(cmd_startup_ctx_t* ctx) {
    char * p = ctx->cmd;
    char * p2 = ctx->cmd;
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
        strncpy(ctx->cmd_t, p, p2 - p);
        ctx->cmd_t[p2 - p] = 0;
    }
    DIR dir;
    FILINFO fileInfo;
    //goutf("\nDIR: %s\n", p != p2 ? cmd_t : curr_dir);
    if (FR_OK != f_opendir(&dir, p != p2 ? ctx->cmd_t : ctx->curr_dir)) {
        return;
    }
    int total_files = 0;
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        p3 = next_on(p2, fileInfo.fname);
        if (p3 != fileInfo.fname) {
            strcpy(ctx->cmd_t, p3);
            total_files++;
        }
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, ctx->cmd_t, total_files);
    }
    if (total_files == 1) {
        p3 = ctx->cmd_t;
        while (*p3) {
            type_char(ctx, *p3++);
        }
    } else {
        // TODO: blimp
    }
    f_closedir(&dir);
}

inline static void cmd_up(cmd_startup_ctx_t* ctx) {
    size_t cmd_pos = strlen(ctx->cmd);
    while(cmd_pos) {
        ctx->cmd[--cmd_pos] = 0;
        gbackspace();
    }
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    goutf(ctx->cmd);
}

inline static void cmd_down(cmd_startup_ctx_t* ctx) {
    size_t cmd_pos = strlen(ctx->cmd);
    while(cmd_pos) {
        ctx->cmd[--cmd_pos] = 0;
        gbackspace();
    }
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    goutf(ctx->cmd);
}

inline static char tolower_token(char t) {
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

inline static int tokenize_cmd(cmd_startup_ctx_t* ctx) {
    bAppend = false;
    if (ctx->cmd[0] == 0) {
        ctx->cmd_t[0] = 0;
        return 0;
    }
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->cmd;
    char* t2 = ctx->cmd_t;
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

inline static void cd(cmd_startup_ctx_t* ctx, char *d) {
    FILINFO fileinfo;
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        strcpy(ctx->curr_dir, d);
    } else if (f_stat(d, &fileinfo) != FR_OK || !(fileinfo.fattrib & AM_DIR)) {
        goutf("Unable to find directory: '%s'\n", d);
    } else {
        strcpy(ctx->curr_dir, d);
    }
}

inline static void cmd_push(cmd_startup_ctx_t* ctx, char c) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    ctx->cmd[cmd_pos++] = c;
    ctx->cmd[cmd_pos] = 0;
    putc(c);
}

inline static void cmd_enter(cmd_startup_ctx_t* ctx) {
    UINT br;
    putc('\n');
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos > 0) { // history
        f_open(&fh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
        f_write(&fh, ctx->cmd, cmd_pos, &br);
        f_write(&fh, "\n", 1, &br);
        f_close(&fh);
    } else {
        goto r;
    }
    int tokens = tokenize_cmd(ctx);
    ctx->cmd = ctx->cmd;
    ctx->cmd_t = ctx->cmd_t;
    ctx->tokens = tokens;
    if (redirect2) {
        if (bAppend) {
            FILINFO fileinfo;
            if (f_stat(redirect2, &fileinfo) != FR_OK) {
                goutf("Unable to find file: '%s'\n", redirect2);
                goto t;
            } else {
                if (f_open(ctx->pstdout, redirect2, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND) != FR_OK) {
                    f_lseek(ctx->pstdout, fileinfo.fsize);
                    goutf("Unable to open file: '%s'\n", redirect2);
                }
            }
        } else {
t:
            if (f_open(ctx->pstdout, redirect2, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
                goutf("Unable to open file: '%s'\n", redirect2);
            }
        }
    }
    if (strcmp("exit", ctx->cmd_t) == 0) { // do not extern, due to internal cmd state
        bExit = true;
    } else if (strcmp("cd", ctx->cmd_t) == 0) { // do not extern, due to internal cmd state
        if (tokens == 1) {
            fgoutf(ctx->pstderr, "Unable to change directoy to nothing\n");
        } else {
            cd(ctx, (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t));
        }
    } else {
        char* t = pvPortMalloc(512);
        if (exists(ctx, t)) {
            int len = strlen(t);
            if (len > 3 && strcmp(t, ".uf2") == 0 && load_firmware(t)) {
                run_app(t);
            } if(is_new_app(t)) {
                int ret = run_new_app(t);
                // todo
            } else {
                goutf("Unable to execute command: '%s'\n", t);
            }
        } else {
            goutf("Illegal command: '%s'\n", ctx->cmd);
        }
        vPortFree(t);
    }
    if (redirect2) {
        f_close(ctx->pstdout);
    }
r:
    goutf("%s>", ctx->curr_dir);
    ctx->cmd[0] = 0;
}

const char* tmp = "/.cmd_history"; // TODO: configure it

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    char* curr_dir = ctx->curr_dir;
    goutf("%s>", curr_dir);
    bExit = false;
    cmd_history_idx = -2;
    size_t cdl = strlen(curr_dir);
    cmd_history_file = (char*)pvPortMalloc(cdl + strlen(tmp));
    strcpy(cmd_history_file, curr_dir);
    strcpy(cmd_history_file + cdl, tmp);
    bAppend = false;
    redirect2 = NULL;
    while(!bExit) {
        char c = getc();
        if (c) {
            if (c == 8) cmd_backspace(ctx);
            else if (c == 17) cmd_up(ctx);
            else if (c == 18) cmd_down(ctx);
            else if (c == '\t') cmd_tab(ctx);
            else if (c == '\n') cmd_enter(ctx);
            else cmd_push(ctx, c);
            c = 0;
        }
    }
    vPortFree(cmd_history_file);
    return 0;
}

int __required_m_api_verion() {
    return 2;
}
