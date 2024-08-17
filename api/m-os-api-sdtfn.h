#pragma once

// use it to resolve issues like memset and/or memcpy are not found on elf32 obj execution attempt

#ifdef __cplusplus
extern "C" {
#endif

void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

char* strcpy(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[60])(t, s);
}

#ifdef __cplusplus
}
#endif
