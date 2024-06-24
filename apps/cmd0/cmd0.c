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
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = cmd;
    while(*t1) {
        char c = tolower_token(*t1++);
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
    FILINFO fileinfo;
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        strcpy(ctx->curr_dir, d);
    } else if (f_stat(d, &fileinfo) != FR_OK || !(fileinfo.fattrib & AM_DIR)) {
        goutf("Unable to find directory: '%s'\n", d);
    } else {
        strcpy(ctx->curr_dir, d);
    }
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
    if (!cmd_pos) {
        goto r;
    }
    int tokens = tokenize_cmd(ctx);
    if (strcmp("exit", cmd) == 0) { // do not extern, due to internal cmd state
        cmd[0] = 0;
        free(ctx->orig_cmd);
        ctx->orig_cmd = 0;
        return true;
    } else if (strcmp("cd", cmd) == 0) { // do not extern, due to internal cmd state
        if (tokens == 1) {
            fgoutf(ctx->std_err, "Unable to change directoy to nothing\n");
        } else {
            cd(ctx, (char*)ctx->orig_cmd + (next_token(cmd) - cmd));
        }
        goto r;
    } else {
        ctx->argc = tokens;
        ctx->argv = malloc(sizeof(char*) * tokens);
        char* t = cmd;
        for (uint32_t i = 0; i < tokens; ++i) {
            ctx->argv[i] = copy_str(t);
            t = next_token(t);
        }
        return true;
    }
r:
    goutf("[%s]#", ctx->curr_dir);
    cmd[0] = 0;
    free(ctx->orig_cmd);
    ctx->orig_cmd = 0;
    return false;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* curr_dir = ctx->curr_dir;
    cleanup_ctx(ctx);
    cmd = malloc(512);
    cmd[0] = 0;
    goutf("[%s]#", curr_dir);
    while(1) {
        char c = getc();
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) {}
            else if (c == 18) {}
            else if (c == '\t') {}
            else if (c == '\n') {
                if ( cmd_enter(ctx) ) {
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
