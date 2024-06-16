#include "m-os-api.h"

static int rmdir(const char* d) {
    DIR dir;
    FILINFO fileInfo;
    if (FR_OK != f_opendir(&dir, d)) {
        cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
        fgoutf(ctx->pstderr, "Failed to open directory: '%s'\n", d);
        return 0;
    }
    size_t total_files = 0;
    char* t = (char*)pvPortMalloc(512);
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        size_t s = strlen(d);
        strncpy(t, d, 512);
        t[s] = '/';
        strncpy(t + s + 1, fileInfo.fname, 511 - s);
        if(fileInfo.fattrib & AM_DIR) {
            total_files += rmdir(t);
        }
        if (f_unlink(t) == FR_OK)
            total_files++;
        else {
            cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
            fgoutf(ctx->pstderr, "Failed to remove file: '%s'\n", t);
        }
    }
    vPortFree(t);
    f_closedir(&dir);
    if (f_unlink(d) == FR_OK) {
        total_files++;
    }
    goutf("    Total: %d files removed\n", total_files);
    return total_files;
}

int main() {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    if (ctx->tokens == 1) {
        fgoutf(ctx->pstderr, "Unable to remove directoy with no name\n");
        return 1;
    }
    char * d = (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t);
    rmdir(d);
    return 0;
}
