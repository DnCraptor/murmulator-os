#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* d = ctx->argc == 1 ? ctx->curr_dir : ctx->argv[1];
    DIR* pdir = (DIR*)pvPortMalloc(sizeof(DIR));
    if (FR_OK != f_opendir(pdir, d)) {
        vPortFree(pdir);
        fgoutf(ctx->std_err, "Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        fgoutf(ctx->std_out, "D ..\n");
    }
    int total_files = 0;
    FILINFO* pfileInfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        fgoutf( ctx->std_out, (pfileInfo->fattrib & AM_DIR) ? "D " : "  ");
        if ((pfileInfo->fsize >> 20) > 9) {
            fgoutf( ctx->std_out, "%04dM",  (uint32_t)(pfileInfo->fsize >> 20));
        } else if ((pfileInfo->fsize >> 10) > 9) {
            fgoutf( ctx->std_out, "%04dK",  (uint32_t)(pfileInfo->fsize >> 10));
        } else {
            fgoutf( ctx->std_out, "%04dB",  (uint32_t)pfileInfo->fsize);
        }
        fgoutf( ctx->std_out, " %s\n", pfileInfo->fname);
        total_files++;
    }
    vPortFree(pfileInfo);
    f_closedir(pdir);
    vPortFree(pdir);
    fgoutf(ctx->std_out, "    Total: %d files.\n", total_files);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
