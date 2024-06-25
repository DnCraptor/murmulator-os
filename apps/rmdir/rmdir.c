#include "m-os-api.h"

static FILINFO* fileInfo;
static int rmdir(const char* d) {
    goutf("rmdir: %s\n", d);
    DIR* dir = (DIR*)pvPortMalloc(sizeof(DIR));
    if (FR_OK != f_opendir(dir, d)) {
        vPortFree(dir);
        fgoutf(get_cmd_ctx()->std_err, "Failed to open directory: '%s'\n", d);
        return 0;
    }
    size_t total_files = 0;
    while (f_readdir(dir, fileInfo) == FR_OK && fileInfo->fname[0] != '\0') {
        char* t = concat(d, fileInfo->fname);
        if(fileInfo->fattrib & AM_DIR) {
            vTaskDelay(1000);
            total_files += rmdir(t);
        }
        if (f_unlink(t) == FR_OK)
            total_files++;
        else {
            fgoutf(get_cmd_ctx()->std_err, "Failed to remove file: '%s'\n", t);
        }
        vPortFree(t);
    }
    f_closedir(dir);
    vPortFree(dir);
    if (f_unlink(d) == FR_OK) {
        total_files++;
    }
    return total_files;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc == 1) {
        fgoutf(ctx->std_err, "Unable to remove directoy with no name\n");
        return 1;
    }
    FILINFO* fileInfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    int files = rmdir(ctx->argv[1]);
    vPortFree(fileInfo);
    goutf("    Total: %d files removed\n", files);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
