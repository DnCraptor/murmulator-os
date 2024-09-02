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

void* memmove(void* dst, const void* src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[232])(dst, src, sz);
}

void _exit(int status) { goutf("_exit(%d)\n", status); }
void _kill(void) { gouta("_kill\n"); }
int _getpid(void) { gouta("_getpid\n"); return 0; }
void _sbrk(void) { gouta("_sbrk\n"); }
void _write(void) { gouta("_write\n"); }
void _close(void) { gouta("_close\n"); }
void _fstat(void) { gouta("_fstat\n"); }
void _isatty(void) { gouta("_isatty\n"); }
void _lseek(void) { gouta("_lseek\n"); }
void _read(void) { gouta("_read\n"); }

#ifdef __cplusplus
}
#endif
