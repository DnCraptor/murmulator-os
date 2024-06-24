#include "m-os-api.h"

static FIL* pfh;
static char* cmd_history_file;
static int cmd_history_idx;
static char* cmd = 0;

static char* next_on(char* l, char *bi) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        l++;
        b++;
    }
    return *l == 0 ? b : bi;
}

inline static void cmd_backspace() {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    cmd[--cmd_pos] = 0;
    gbackspace();
}

inline static int history_steps(cmd_ctx_t* ctx) {
    size_t j = 0;
    int idx = 0;
    UINT br;
    f_open(pfh, cmd_history_file, FA_READ);
    char* b = malloc(512);
    while(f_read(pfh, b, 512, &br) == FR_OK && br) {
        for(size_t i = 0; i < br; ++i) {
            char t = b[i];
            if(t == '\n') { // next line
                cmd[j] = 0;
                j = 0;
                if(cmd_history_idx == idx)
                    break;
                idx++;
            } else {
                cmd[j++] = t;
            }
        }
    }
    free(b);
    f_close(pfh);
    return idx;
}

inline static void type_char(char c) {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
        return;
    }
    putc(c);
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
}

inline static void cmd_tab(cmd_ctx_t* ctx) {
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
    char* b = malloc(512);
    if (p != p2) {
        strncpy(b, p, p2 - p);
        b[p2 - p] = 0;
    }
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    FILINFO* pfileInfo = malloc(sizeof(FILINFO));
    //goutf("\nDIR: %s\n", p != p2 ? b : curr_dir);
    if (FR_OK != f_opendir(pdir, p != p2 ? b : ctx->curr_dir)) {
        free(b);
        return;
    }
    int total_files = 0;
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        p3 = next_on(p2, pfileInfo->fname);
        if (p3 != pfileInfo->fname) {
            strcpy(b, p3);
            total_files++;
        }
        //goutf("p3: %s; p2: %s; fn: %s; cmd_t: %s; fls: %d\n", p3, p2, fileInfo.fname, b, total_files);
    }
    if (total_files == 1) {
        p3 = b;
        while (*p3) {
            type_char(*p3++);
        }
    } else {
        // TODO: blimp
    }
    free(b);
    f_closedir(pdir);
    free(pfileInfo);
    free(pdir);
}

inline static void cmd_up(cmd_ctx_t* ctx) {
    size_t cmd_pos = strlen(cmd);
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    goutf(cmd);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    size_t cmd_pos = strlen(cmd);
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    goutf(cmd);
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

inline static int tokenize_cmd(cmd_ctx_t* ctx) {
    bAppend = false;
    redirect2 = NULL;
    if (cmd[0] == 0) {
        return 0;
    }
    ctx->orig_cmd = copy_str(cmd);
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = cmd;
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

inline static void cd(cmd_ctx_t* ctx, char *d) {
    FILINFO* pfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        goto a;
    } else if (f_stat(d, pfileinfo) != FR_OK || !(pfileinfo->fattrib & AM_DIR)) {
        goutf("Unable to find directory: '%s'\n", d);
        goto r;
    }
a:
    {
        char* bk = ctx->curr_dir;
        ctx->curr_dir = copy_str(d);
        free(bk);
    }
r:
    free(pfileinfo);
}

inline static void cmd_push(char c) {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    cmd[cmd_pos++] = c;
    cmd[cmd_pos] = 0;
    putc(c);
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    UINT br;
    putc('\n');
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos > 0) { // history
        f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
        f_write(pfh, cmd, cmd_pos, &br);
        f_write(pfh, "\n", 1, &br);
        f_close(pfh);
    } else {
        goto r2;
    }
    //goutf("cmd: %s\n", cmd);
    int tokens = tokenize_cmd(ctx);
    if (tokens == 0) {
        goto r1;
    } else if (redirect2) { // TODO: err, pipes
        if (bAppend) {
            FILINFO* pfileinfo = malloc(sizeof(FILINFO));
            if (f_stat(redirect2, pfileinfo) != FR_OK) {
                goutf("Unable to find file for append: '%s'\n", redirect2);
                free(pfileinfo);
                goto t;
            } else {
                ctx->std_out = malloc(sizeof(FIL));
                if (f_open(ctx->std_out, redirect2, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND) != FR_OK) {
                    f_lseek(ctx->std_out, pfileinfo->fsize);
                    goutf("Unable to open file for append: '%s'\n", redirect2);
                }
            }
            free(pfileinfo);
        } else {
t:
            ctx->std_out = malloc(sizeof(FIL));
            if (f_open(ctx->std_out, redirect2, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
                goutf("Unable to open file to redirect: '%s'\n", redirect2);
            }
        }
    } else if (strcmp("exit", cmd) == 0) { // do not extern, due to internal cmd state
        cleanup_ctx(ctx);
        ctx->stage = EXECUTED;
        return true;
    } else if (strcmp("cd", cmd) == 0) { // do not extern, due to internal cmd state
        if (tokens == 1) {
            fgoutf(ctx->std_err, "Unable to change directoy to nothing\n");
        } else if (tokens > 2) {
            fgoutf(ctx->std_err, "Unable to change directoy to more than one target\n");
        } else {
            cd(ctx, (char*)ctx->orig_cmd + (next_token(cmd) - cmd));
        }
        goto r1;
    } else {
        ctx->argc = tokens;
        ctx->argv = (char**)malloc(sizeof(char*) * tokens);
        char* t = cmd;
        for (uint32_t i = 0; i < tokens; ++i) {
            ctx->argv[i] = copy_str(t);
            t = next_token(t);
        }
        ctx->stage = PREPARED;
        return true;
    }
r1:
    cleanup_ctx(ctx);
r2:
    goutf("[%s]$", ctx->curr_dir);
    cmd[0] = 0;
    return false;

}

const char* hist = ".cmd_history"; // TODO: configure it

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    cmd = malloc(512);
    cmd[0] = 0;
    goutf("[%s]$", ctx->curr_dir);
    cmd_history_idx = -2;
    char* tmp = get_ctx_var(ctx, "TEMP");
    goutf("tmp: '%s'\n", tmp);
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    cmd_history_file = concat(tmp, hist);
    goutf("history_file: '%s'\n", cmd_history_file);
    pfh = (FIL*)malloc(sizeof(FIL)); 
    bAppend = false;
    redirect2 = NULL;
    while(1) {
        char c = getc();
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) cmd_up(ctx);
            else if (c == 18) cmd_down(ctx);
            else if (c == '\t') cmd_tab(ctx);
            else if (c == '\n') {
                if ( cmd_enter(ctx) ) {
                    free(pfh);
                    free(cmd_history_file);
                    free(cmd);
                    return 0;
                }
            } else cmd_push(c);
        }
    }
    __unreachable();
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
