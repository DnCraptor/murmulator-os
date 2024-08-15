#include "m-os-api.h"

static string_t* s_cmd = 0;

inline static void cmd_backspace() {
    if (s_cmd->size == 0) {
        blimp(10, 5);
        return;
    }
    string_resize(s_cmd, s_cmd->size - 1);
    gbackspace();
}

inline static void type_char(char c) {
    putc(c);
    string_push_back_c(s_cmd, c);
}

inline static char replace_spaces0(char t) {
    return (t == ' ') ? 0 : t;
}

inline static int tokenize_cmd(cmd_ctx_t* ctx) {
    if (!s_cmd->size) {
        return 0;
    }
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(s_cmd->p);
    //goutf("orig_cmd: '%s' [%p]; cmd: '%s' [%p]\n", ctx->orig_cmd, ctx->orig_cmd, cmd, cmd);
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->orig_cmd;
    char* t2 = s_cmd->p;
    while(*t1) {
        char c = replace_spaces0(*t1++);
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

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    UINT br;
    putc('\n');
    if (!s_cmd->size) {
        goto r2;
    }
    int tokens = tokenize_cmd(ctx);
    if (tokens == 0) {
        goto r1;
    }
    ctx->argc = tokens;
    ctx->argv = (char**)malloc(sizeof(char*) * tokens);
    char* t = s_cmd->p;
    for (uint32_t i = 0; i < tokens; ++i) {
        ctx->argv[i] = copy_str(t);
        t = next_token(t);
    }
    ctx->stage = PREPARED;
    return true;
r1:
    cleanup_ctx(ctx);
r2:
    goutf("[%s]#", get_ctx_var(ctx, "CD"));
    string_resize(s_cmd, 0);
    return false;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    s_cmd = new_string_v();
    goutf("[%s]#", get_ctx_var(ctx, "CD"));
    while(1) {
        char c = getch();
        if (c) {
            if (c == 8) cmd_backspace();
            else if (c == 17) {}
            else if (c == 18) {}
            else if (c == '\t') {}
            else if (c == '\n') {
                if ( cmd_enter(ctx) ) {
                    delete_string(s_cmd);
                    //goutf("[%s]EXIT to exec, stage: %d\n", ctx->curr_dir, ctx->stage);
                    return 0;
                }
            } else type_char(c);
        }
    }
    __unreachable();
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
