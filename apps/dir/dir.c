#include "m-os-api.h"

void* memset (void* b, int v, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        ((char*)b)[i] = v;
    }
    return b;
}

int main(void) {
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
    char* buf = (char*)pvPortMalloc(81);
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        for(int i = 0; i < 80; ++i) buf[i] = ' '; buf[80] = 0;
        if (pfileInfo->fattrib & AM_DIR) strncpy(buf, "D ", 2);
        snprintf(buf + 2, 8, "%08d", pfileInfo->fsize);
        size_t sl = strlen(pfileInfo->fname);
        strncpy(buf + 11, pfileInfo->fname, MAX(sl, 69));
        fgoutf(ctx->pstdout, buf);
        total_files++;
    }
    vPortFree(buf);
    vPortFree(pfileInfo);
    f_closedir(pdir);
    vPortFree(pdir);
    fgoutf(ctx->pstdout, "    Total: %d files.\n", total_files);
    return 0;
}

int __required_m_api_verion(void) {
    return 2;
}
