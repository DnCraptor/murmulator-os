#include "m-os-api.h"

static int rmdir(const char* d) {
    DIR* dir = (DIR*)pvPortMalloc(sizeof(DIR));
    if (FR_OK != f_opendir(&dir, d)) {
        vPortFree(dir);
        fgoutf(get_cmd_startup_ctx()->pstderr, "Failed to open directory: '%s'\n", d);
        return 0;
    }
    size_t total_files = 0;
    FILINFO* fileInfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    while (f_readdir(dir, &fileInfo) == FR_OK && fileInfo->fname[0] != '\0') {
        char* t = concat(d, fileInfo->fname);
        if(fileInfo->fattrib & AM_DIR) {
            total_files += rmdir(t);
        }
        if (f_unlink(t) == FR_OK)
            total_files++;
        else {
            fgoutf(get_cmd_startup_ctx()->pstderr, "Failed to remove file: '%s'\n", t);
        }
        vPortFree(t);
    }
    vPortFree(fileInfo);
    f_closedir(dir);
    vPortFree(dir);
    if (f_unlink(d) == FR_OK) {
        total_files++;
    }
    return total_files;
}

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to remove directoy with no name\n");
        return 1;
    }
    char * d = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    int files = rmdir(d);
    goutf("    Total: %d files removed\n", files);
    return 0;
}

int __required_m_api_verion(void) {
    return 2;
}
