#ifndef FUNC_H
#define FUNC_H

typedef void (*fn_vi_t)(void*, int);
typedef void (*fn_vp_t)(void*, unsigned char*);
typedef unsigned char (*fn_read_t)(void*, unsigned short);
typedef void (*fn_write_t)(void*, unsigned short, unsigned char);

typedef struct func_vi_t {
    fn_vi_t fn;
    void* p;
} func_vi_t;

typedef struct func_vp_t {
    fn_vp_t fn;
    void* p;
} func_vp_t;

typedef struct func_read_t {
    fn_read_t fn;
    void* p;
} func_read_t;

typedef struct func_write_t {
    fn_write_t fn;
    void* p;
} func_write_t;

#endif
