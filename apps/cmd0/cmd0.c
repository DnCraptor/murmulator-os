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

inline static void tokenize_cmd(list_t* lst, cmd_ctx_t* ctx) {
    if (!s_cmd->size) {
        return 0;
    }
    bool inSpace = false;
    string_t* t = new_string_v();
    for (size_t i = 0; i < s_cmd->size; ++i) {
        char c = replace_spaces0(s_cmd->p[i]);
        if (inSpace) {
            if(c) { // token started
                inSpace = false;
                list_push_back(lst, t);
                t = new_string_v();
            }
        } else if(!c) { // not in space, after the token
            inSpace = true;
        }
        string_push_back_c(t, c);
    }
    list_push_back(lst, t);
}

static void prompt(cmd_ctx_t* ctx) {
    string_resize(s_cmd, 0);
    printf("[%s]#", get_ctx_var(ctx, "CD"));
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    UINT br;
    putc('\n');
    if (!s_cmd->size) {
        goto r2;
    }
    list_t* lst = new_list_v(new_string_v, delete_string, NULL);
    tokenize_cmd(lst, ctx);
    if (list_count(lst) == 0) {
        goto r1;
    }
    ctx->argc = lst->size;
    ctx->argv = (char**)calloc(sizeof(char*), ctx->argc);
    node_t* n = lst->first;
    for(size_t i = 0; i < lst->size && n != NULL; ++i, n = n->next) {
        ctx->argv[i] = copy_str(c_str(n->data));
    }
    if (ctx->orig_cmd) free(ctx->orig_cmd);
    ctx->orig_cmd = copy_str(ctx->argv[0]);

    ctx->stage = PREPARED;
    return true;
r1:
    delete_list(lst);
    cleanup_ctx(ctx);
r2:
    prompt(ctx);
    return false;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    s_cmd = new_string_v();
    prompt(ctx);
    while(1) {
        char c = getch();
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) { blimp(15, 1); }
            else if (c == CHAR_CODE_DOWN) { blimp(15, 1); }
            else if (c == CHAR_CODE_TAB) { blimp(15, 1); }
            else if (c == CHAR_CODE_ESC) {
                printf("\n");
                string_resize(s_cmd, 0);
                prompt(ctx);
            }
            else if (c == CHAR_CODE_ENTER) {
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
