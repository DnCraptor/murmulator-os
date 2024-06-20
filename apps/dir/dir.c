#include "m-os-api.h"
#include <hardware/timer.h>

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    char* d = ctx->tokens == 1 ? ctx->curr_dir : (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    DIR* pdir = (DIR*)pvPortMalloc(sizeof(DIR));
    if (FR_OK != f_opendir(pdir, d)) {
        vPortFree(pdir);
        fgoutf(ctx->pstderr, "Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        fgoutf(ctx->pstdout, "D ..\n");
    }
    int total_files = 0;
    FILINFO* pfileInfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        fgoutf(ctx->pstdout, pfileInfo->fattrib & AM_DIR ? "D " : "  ");
        fgoutf(ctx->pstdout, "%s\n", pfileInfo->fname);
        total_files++;
    }
    vPortFree(pfileInfo);
    f_closedir(pdir);
    vPortFree(pdir);
    fgoutf(ctx->pstdout, "    Total: %d files\n", total_files);
    return 0;
}

int __required_m_api_verion() {
    return 2;
}
