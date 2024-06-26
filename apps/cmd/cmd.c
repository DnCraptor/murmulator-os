#include "m-os-api.h"

static char* cmd = 0;

inline static void cmd_backspace() {
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    cmd[--cmd_pos] = 0;
    gbackspace();
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

inline static int tokenize_cmd(cmd_ctx_t* ctx) {
    if (cmd[0] == 0) {
        return 0;
    }
    ctx->orig_cmd = copy_str(cmd);
    //goutf("orig_cmd: '%s' [%p]; cmd: '%s' [%p]\n", ctx->orig_cmd, ctx->orig_cmd, cmd, cmd);
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = cmd;
    while(*t1) {
        char c = tolower_token(*t1++);
        //goutf("%02X -> %c %02X; t1: '%s' [%p], t2: '%s' [%p]\n", c, *t2, *t2, t1, t1, t2, t2);
        if (inSpace) {
            if(c) { // token started
                inSpace = 0;
                inTokenN++; // new token
            }
        } else if(!c) { // not in space, after the token
            inSpace = true;
        }
        *t2++ = c;
    }
    *t2 = 0;
    //goutf("cmd: %s\n", cmd);
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

inline static void cmd_write_history(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, cmd, strlen(cmd), &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    free(pfh);
    free(cmd_history_file);
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    putc('\n');
    size_t cmd_pos = strlen(cmd);
    if (cmd_pos) {
        cmd_write_history(ctx);
    } else {
        goto r2;
    }
    int tokens = tokenize_cmd(ctx);
    if (tokens == 0) {
        goto r1;
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

inline static char* next_on(char* l, char *bi) {
    char *b = bi;
    while(*l && *b && *l == *b) {
        l++;
        b++;
    }
    return *l == 0 ? b : bi;
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

inline static void cancel_entered() {
    size_t cmd_pos = strlen(cmd);
    while(cmd_pos) {
        cmd[--cmd_pos] = 0;
        gbackspace();
    }
}

static int cmd_history_idx = -2;

inline static int history_steps(cmd_ctx_t* ctx) {
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    size_t cdl = strlen(tmp);
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)malloc(sizeof(FIL));
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
    free(pfh);
    free(cmd_history_file);
    return idx;
}

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    goutf(cmd);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx);
    goutf(cmd);
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    cmd = malloc(512);
    cmd[0] = 0;
    goutf("[%s]$", ctx->curr_dir);
    while(1) {
        char c = getc();
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) cmd_up(ctx);
            else if (c == 18) cmd_down(ctx);
            else if (c == '\t') cmd_tab(ctx);
            else if (c == '\n') {
                if ( cmd_enter(ctx) ) {
                    free(cmd);
                    //goutf("[%s]EXIT to exec, stage: %d\n", ctx->curr_dir, ctx->stage);
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
