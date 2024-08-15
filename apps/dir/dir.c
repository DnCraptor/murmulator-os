#include "m-os-api.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* d = ctx->argc == 1 ? get_ctx_var(ctx, "CD") : ctx->argv[1];
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    if (FR_OK != f_opendir(pdir, d)) {
        free(pdir);
        fprintf(ctx->std_err, "Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        printf("D ..\n");
    }
    int total_files = 0;
    FILINFO* pfileInfo = (FILINFO*)malloc(sizeof(FILINFO));
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        char tmp[] = ".....";
        if (pfileInfo->fattrib & AM_DIR) {
            // do nothing
        } else if ((pfileInfo->fsize >> 20) > 9) {
            snprintf(tmp, 6, "%04dM", (uint32_t)(pfileInfo->fsize >> 20));
        } else if ((pfileInfo->fsize >> 10) > 9) {
            snprintf(tmp, 6, "%04dK", (uint32_t)(pfileInfo->fsize >> 10));
        } else {
            snprintf(tmp, 6, "%04dB", (uint32_t)pfileInfo->fsize);
        }
        char* t = tmp;
        while(*t == '0') { *t++ = ' '; }
        printf("%s %s %s\n", (pfileInfo->fattrib & AM_DIR) ? "D" : " ", tmp, pfileInfo->fname);
        total_files++;
    }
    free(pfileInfo);
    f_closedir(pdir);
    free(pdir);
    printf("    Total: %d files.\n", total_files);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
