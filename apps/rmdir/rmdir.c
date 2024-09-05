#include "m-os-api.h"

static DIR* dir;
static FILINFO* fileInfo;

static int rmdir(cmd_ctx_t* ctx, const char* d) {
    if (marked_to_exit) return 0;
    goutf("rmdir: %s\n", d);
    if (FR_OK != f_opendir(dir, d)) {
        fgoutf(ctx->std_err, "Failed to open directory: '%s'\n", d);
        return 0;
    }
    size_t total_files = 0;
    while (!marked_to_exit && f_readdir(dir, fileInfo) == FR_OK && fileInfo->fname[0] != '\0') {
        char* t = concat(d, fileInfo->fname);
        if(fileInfo->fattrib & AM_DIR) {
            f_closedir(dir);
            total_files += rmdir(ctx, t);
            if (FR_OK != f_opendir(dir, d)) {
                fgoutf(ctx->std_err, "Failed to open directory: '%s'\n", d);
                return total_files;
            }
        } else if (f_unlink(t) == FR_OK) {
            total_files++;
        } else {
            fgoutf(ctx->std_err, "Failed to remove file/dir: '%s'\n", t);
        }
        vPortFree(t);
    }
    f_closedir(dir);
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
    dir = (DIR*)malloc(sizeof(DIR));
    fileInfo = (FILINFO*)malloc(sizeof(FILINFO));
    marked_to_exit = false;
    int files = rmdir(ctx, ctx->argv[1]);
    free(fileInfo);
    free(dir);
    goutf("    Total: %d files removed\n", files);
    return 0;
}
