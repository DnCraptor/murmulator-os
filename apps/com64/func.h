#ifndef FUNC_H
#define FUNC_H

typedef unsigned char (*fn_read_t)(void*, unsigned short);
typedef void (*fn_write_t)(void*, unsigned short, unsigned char);

typedef struct func_read_t {
    fn_read_t fn;
    void* p;
} func_read_t;

typedef struct func_write_t {
    fn_write_t fn;
    void* p;
} func_write_t;

#endif

