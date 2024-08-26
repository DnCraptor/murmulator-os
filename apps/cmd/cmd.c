#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

// #define DEBUG

static string_t* s_cmd = NULL;

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

inline static void prompt(cmd_ctx_t* ctx) {
    graphics_set_con_color(13, 0);
    printf("[%s]", get_ctx_var(ctx, "CD"));
    graphics_set_con_color(7, 0);
    printf("$ ");
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    putc('\n');
    if ( cmd_enter_helper(ctx, s_cmd) ) {
        return true;
    }
    prompt(ctx);
    string_resize(s_cmd, 0);
    return false;
}

inline static void cancel_entered() {
    while(s_cmd->size) {
        string_resize(s_cmd, s_cmd->size - 1);
        gbackspace();
    }
}

static int cmd_history_idx;

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx, cmd_history_idx, s_cmd);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    gouta(s_cmd->p);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx, cmd_history_idx, s_cmd);
    gouta(s_cmd->p);
}

int main(void) {
    show_logo(false);
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    s_cmd = new_string_v();
    prompt(ctx);
    cmd_history_idx = -2;
    while(1) {
        char c = getch();
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) cmd_up(ctx);
            else if (c == CHAR_CODE_DOWN) cmd_down(ctx);
            else if (c == CHAR_CODE_TAB) cmd_tab(ctx, s_cmd);
            else if (c == CHAR_CODE_ESC) {
                blimp(15, 1);
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
