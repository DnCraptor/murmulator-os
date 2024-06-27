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
    if (ctx->orig_cmd) free(ctx->orig_cmd);
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
    //goutf("cmd_pos: %d\n", cmd_pos);
    if (!cmd_pos) {
        goto r2;
    }
    int tokens = tokenize_cmd(ctx);
    if (tokens == 0) {
        goto r1;
    } else if (strcmp("exit", cmd) == 0) { // do not extern, due to internal cmd state
        cleanup_ctx(ctx);
        ctx->stage = EXECUTED;
        return true;
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
    goutf("[%s]#", ctx->curr_dir);
    cmd[0] = 0;
    return false;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    cmd = malloc(512);
    cmd[0] = 0;
    goutf("[%s]#", ctx->curr_dir);
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
