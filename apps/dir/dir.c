#include "m-os-api.h"
#include <hardware/timer.h>

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    char* d = ctx->tokens == 1 ? ctx->curr_dir : (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    DIR dir;
    FILINFO fileInfo;
    if (FR_OK != f_opendir(&dir, d)) {
        fgoutf(ctx->pstderr, "Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        fgoutf(ctx->pstdout, "D ..\n");
    }
    int total_files = 0;
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        fgoutf(ctx->pstdout, fileInfo.fattrib & AM_DIR ? "D " : "  ");
        fgoutf(ctx->pstdout, "%s\n", fileInfo.fname);
        total_files++;
    }
    f_closedir(&dir);
    fgoutf(ctx->pstdout, "    Total: %d files\n", total_files);
    return 0;
}

int __required_m_api_verion() {
    return 2;
}
