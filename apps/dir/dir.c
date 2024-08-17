#include "m-os-api.h"
#include "m-os-api-sdtfn.h"

static bool sort_by_size(cmd_ctx_t* ctx) {
    if (ctx->argc < 2) {
        return false;
    }
    if (ctx->argc == 2) {
        return strcmp("-S", ctx->argv[1]) == 0;
    }
    if (ctx->argc == 3) {
        return strcmp("-S", ctx->argv[1]) == 0 || strcmp("-S", ctx->argv[2]) == 0;
    }
    return false;
}

static char* get_dir(cmd_ctx_t* ctx) {
    if (ctx->argc == 1) {
        return get_ctx_var(ctx, "CD");
    }
    if (ctx->argc == 2) {
        return strcmp("-S", ctx->argv[1]) == 0 ? get_ctx_var(ctx, "CD") : ctx->argv[1];
    }
    if (ctx->argc == 3) {
        return strcmp("-S", ctx->argv[1]) == 0 ? ctx->argv[2] : ctx->argv[1];
    }
}

typedef struct {
    FSIZE_t fsize;   /* File size */
    string_t* s_name;
    BYTE    fattrib; /* File attribute */
} file_info_t;

static array_t* arr = NULL; // of file_info_t*

static void fi_deallocator(file_info_t* p) {
    delete_string(p->s_name);
    free(p);
}
static file_info_t* new_file_info(FILINFO* p) {
    file_info_t* res = (file_info_t*)malloc(sizeof(file_info_t));
    res->fsize = p->fsize;
    res->s_name = new_string_cc(p->fname);
    res->fattrib = p->fattrib;
    return res;
}

static int m_comp1(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    //printf("m_comp [%s] [%s]\n", e1->s_name->p, e2->s_name->p);
    if ((e1->fattrib & AM_DIR) && !(e2->fattrib & AM_DIR)) return -1;
    if (!(e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) return 1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

static int m_comp2(const void* pe1, const void* pe2) {
    file_info_t* e1 = *((file_info_t**)pe1);
    file_info_t* e2 = *((file_info_t**)pe2);
    //printf("m_comp [%s] [%s]\n", e1->s_name->p, e2->s_name->p);
    if ((e1->fattrib & AM_DIR) && !(e2->fattrib & AM_DIR)) return -1;
    if (!(e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) return 1;
    if ((e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) {
        return strcmp(e1->s_name->p, e2->s_name->p);
    }
    if (e1->fsize > e2->fsize) return -1;
    if (e1->fsize < e2->fsize) return 1;
    return strcmp(e1->s_name->p, e2->s_name->p);
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    char* d = get_dir(ctx);
    bool _sort_by_size = sort_by_size(ctx);
    DIR* pdir = (DIR*)malloc(sizeof(DIR));
    if (FR_OK != f_opendir(pdir, d)) {
        free(pdir);
        fprintf(ctx->std_err, "Failed to open directory: '%s'\n", d);
        return;
    }
    if (strlen(d) > 1) {
        printf("D ..\n");
    }
    arr = new_array_v(NULL, fi_deallocator, NULL);

    int files = 0;
    int folders = 0;
    FILINFO* pfileInfo = (FILINFO*)malloc(sizeof(FILINFO));
    while (f_readdir(pdir, pfileInfo) == FR_OK && pfileInfo->fname[0] != '\0') {
        array_push_back(arr, new_file_info(pfileInfo));
        if (pfileInfo->fattrib & AM_DIR) {
            ++folders;
        } else {
            ++files;
        }
    }
    free(pfileInfo);
    f_closedir(pdir);
    free(pdir);
    // sort by std C-fn
    qsort(arr->p, arr->size, sizeof(void*), _sort_by_size ? m_comp2 : m_comp1);
    for (size_t i = 0; i < arr->size; ++i) {
        file_info_t* pfileInfo = (file_info_t*)array_get_at(arr, i);
        char tmp[] = "..... ";
        if (pfileInfo->fattrib & AM_DIR) {
            // do nothing
        } else if ((pfileInfo->fsize >> 20) > 9) {
            snprintf(tmp, 7, "%04d M", (uint32_t)(pfileInfo->fsize >> 20));
        } else if ((pfileInfo->fsize >> 10) > 9) {
            snprintf(tmp, 7, "%04d K", (uint32_t)(pfileInfo->fsize >> 10));
        } else {
            snprintf(tmp, 7, "%04d B", (uint32_t)pfileInfo->fsize);
        }
        char* t = tmp;
        while(*t == '0' && t[1] != ' ') {
            *t++ = ' ';
        }
        printf("%s %s %s\n", (pfileInfo->fattrib & AM_DIR) ? "D" : " ", tmp, pfileInfo->s_name->p);
    }
    delete_array(arr);
    printf("    Total: %d files, %d folders.\n", files, folders);
    return 0;
}

int __required_m_api_verion(void) {
    return M_API_VERSION;
}
