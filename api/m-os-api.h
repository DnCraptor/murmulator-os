#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !M_API_VERSION
#define M_API_VERSION 4
#endif

#define M_OS_API_SYS_TABLE_BASE ((void*)0x10001000ul)
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "m-os-api-tasks.h"
#include "m-os-api-ff.h"

#define __in_boota(group) __attribute__((section(".boota" group)))
#define	__aligned(x)	__attribute__((__aligned__(x)))

#define _snprintfPtrIdx 29

#define _draw_text_ptr_idx 30
#define _draw_window_ptr_idx 31

/*! \brief Macro to return the maximum of two comparable values
 *  \ingroup pico_platform
 */
#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

/*! \brief Macro to return the minimum of two comparable values
 *  \ingroup pico_platform
 */
#ifndef MIN
#define MIN(a, b) ((b)>(a)?(a):(b))
#endif


//#include <stdio.h>
typedef int	(*snprintf_ptr_t)(char *__restrict, size_t, const char *__restrict, ...) _ATTRIBUTE ((__format__ (__printf__, 3, 4)));
#define snprintf(...) ((snprintf_ptr_t)_sys_table_ptrs[_snprintfPtrIdx])(__VA_ARGS__)

// grapthics.h
typedef void (*draw_text_ptr_t)(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor);
inline static void draw_text(const char *string, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor) {
    ((draw_text_ptr_t)_sys_table_ptrs[_draw_text_ptr_idx])(string, x, y, color, bgcolor);
}

typedef void (*draw_window_ptr_t)(const char*, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
inline static void draw_window(const char* t, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    ((draw_window_ptr_t)_sys_table_ptrs[_draw_window_ptr_idx])(t, x, y, width, height);
}

typedef __builtin_va_list __gnuc_va_list;
#define __VALIST __gnuc_va_list

typedef int (*vsnprintf_ptr_t)(char *__restrict, size_t, const char *__restrict, __VALIST)
               _ATTRIBUTE ((__format__ (__printf__, 3, 0)));
inline static int vsnprintf(char *__restrict buff, size_t lim, const char *__restrict msg, __VALIST lst) {
    return ((vsnprintf_ptr_t)_sys_table_ptrs[67])(buff, lim, msg, lst);
}

typedef void (*goutf_ptr_t)(const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
#define goutf(...) ((goutf_ptr_t)_sys_table_ptrs[41])(__VA_ARGS__)
#define printf(...) ((goutf_ptr_t)_sys_table_ptrs[41])(__VA_ARGS__)

/* typedef va_list only when required */
#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE
#ifdef __GNUC__
#ifndef _VA_LIST_DEFINED
typedef __gnuc_va_list va_list;
#define _VA_LIST_DEFINED
#endif
#else /* !__GNUC__ */
#include <stdarg.h>
#endif
#endif /* __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE */
#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)

inline static void gouta(char* string) {
    typedef void (*t_ptr_t)(char*);
    ((t_ptr_t)_sys_table_ptrs[127])(string);
}
/*
static void goutf(const char *__restrict str, ...) {
    va_list ap;
    char* buf = (char*)pvPortMalloc(512);
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    gouta(buf);
    vPortFree(buf);
}
*/
typedef void (*cls_ptr_t)( uint8_t color );
inline static void clrScr(uint8_t color) {
    ((cls_ptr_t)_sys_table_ptrs[44])(color);
}

typedef void (*graphics_set_con_pos_ptr_t)(int x, int y);
typedef void (*graphics_set_con_color_ptr_t)(uint8_t color, uint8_t bgcolor);
inline static void graphics_set_con_pos(int x, int y) {
    ((graphics_set_con_pos_ptr_t)_sys_table_ptrs[42])(x, y);
}
inline static void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    ((graphics_set_con_color_ptr_t)_sys_table_ptrs[43])(color, bgcolor);
}

typedef FIL* (*FIL_getter_ptr_t)();
inline static FIL* get_stdout() {
    return ((FIL_getter_ptr_t)_sys_table_ptrs[68])();
}
inline static FIL* get_stderr() {
    return ((FIL_getter_ptr_t)_sys_table_ptrs[69])();
}

typedef void (*fgoutf_ptr_t)(FIL*, const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
#define fgoutf(...) ((fgoutf_ptr_t)_sys_table_ptrs[70])(__VA_ARGS__)

typedef uint32_t (*u32v_ptr_t)();
inline static uint32_t psram_size() {
    return ((u32v_ptr_t)_sys_table_ptrs[71])();
}
typedef void (*vv_ptr_t)();
inline static void psram_cleanup() {
    return ((vv_ptr_t)_sys_table_ptrs[72])();
}
typedef void (*vu32u8_ptr_t)(uint32_t addr32, uint8_t v);
inline static void write8psram(uint32_t addr32, uint8_t v) {
    return ((vu32u8_ptr_t)_sys_table_ptrs[73])(addr32, v);
}
typedef void (*vu32u16_ptr_t)(uint32_t addr32, uint16_t v);
inline static void write16psram(uint32_t addr32, uint16_t v) {
    return ((vu32u16_ptr_t)_sys_table_ptrs[74])(addr32, v);
}
typedef void (*vu32u32_ptr_t)(uint32_t addr32, uint32_t v);
inline static void write32psram(uint32_t addr32, uint32_t v) {
    return ((vu32u32_ptr_t)_sys_table_ptrs[75])(addr32, v);
}
typedef uint8_t (*u8u32_ptr_t)(uint32_t addr32);
inline static uint8_t read8psram(uint32_t addr32) {
    return ((u8u32_ptr_t)_sys_table_ptrs[76])(addr32);
}
typedef uint16_t (*u16u32_ptr_t)(uint32_t addr32);
inline static uint16_t read16psram(uint32_t addr32) {
    return ((u16u32_ptr_t)_sys_table_ptrs[77])(addr32);
}
typedef uint32_t (*u32u32_ptr_t)(uint32_t addr32);
inline static uint32_t read32psram(uint32_t addr32) {
    return ((u32u32_ptr_t)_sys_table_ptrs[78])(addr32);
}

typedef uint32_t (*u32u32u32_ptr_t)(uint32_t, uint32_t);
inline static uint32_t __u32u32u32_div(uint32_t x, uint32_t y) {
    return ((u32u32u32_ptr_t)_sys_table_ptrs[79])(x, y);
}
inline static uint32_t __u32u32u32_rem(uint32_t x, uint32_t y) {
    return ((u32u32u32_ptr_t)_sys_table_ptrs[80])(x, y);
}

typedef float (*fff_ptr_t)(float, float);
inline static float __fff_div(float x, float y) {
    return ((fff_ptr_t)_sys_table_ptrs[81])(x, y);
}
inline static float __fff_mul(float x, float y) {
    return ((fff_ptr_t)_sys_table_ptrs[82])(x, y);
}
typedef float (*ffu32_ptr_t)(float, uint32_t);
inline static float __ffu32_mul(float x, uint32_t y) {
    return ((ffu32_ptr_t)_sys_table_ptrs[83])(x, y);
}

typedef double (*ddd_ptr_t)(double, double);
inline static double __ddd_div(double x, double y) {
    return ((ddd_ptr_t)_sys_table_ptrs[84])(x, y);
}
inline static double __ddd_mul(double x, double y) {
    return ((ddd_ptr_t)_sys_table_ptrs[85])(x, y);
}
typedef double (*ddu32_ptr_t)(double, uint32_t);
inline static double __ddu32_mul(double x, uint32_t y) {
    return ((ddu32_ptr_t)_sys_table_ptrs[86])(x, y);
}
typedef double (*ddf_ptr_t)(double, float);
inline static double __ddf_mul(double x, float y) {
    return ((ddf_ptr_t)_sys_table_ptrs[87])(x, y);
}

inline static float __ffu32_div(float x, uint32_t y) {
    return ((ffu32_ptr_t)_sys_table_ptrs[88])(x, y);
}
inline static double __ddu32_div(double x, uint32_t y) {
    return ((ddu32_ptr_t)_sys_table_ptrs[89])(x, y);
}

inline static uint32_t swap_size() {
    return ((u32v_ptr_t)_sys_table_ptrs[90])();
}
inline static uint32_t swap_base_size() {
    return ((u32v_ptr_t)_sys_table_ptrs[91])();
}
typedef uint8_t* (*pu8v_ptr_t)();
inline static uint8_t* swap_base() {
    return ((pu8v_ptr_t)_sys_table_ptrs[92])();
}

inline static uint8_t ram_page_read(uint32_t addr32) {
    return ((u8u32_ptr_t)_sys_table_ptrs[93])(addr32);
}
inline static uint16_t ram_page_read16(uint32_t addr32) {
    return ((u16u32_ptr_t)_sys_table_ptrs[94])(addr32);
}
inline static uint32_t ram_page_read32(uint32_t addr32) {
    return ((u32u32_ptr_t)_sys_table_ptrs[95])(addr32);
}

inline static void ram_page_write(uint32_t addr32, uint8_t value) {
    ((vu32u8_ptr_t)_sys_table_ptrs[96])(addr32, value);
}
inline static void ram_page_write16(uint32_t addr32, uint16_t value) {
    ((vu32u16_ptr_t)_sys_table_ptrs[97])(addr32, value);
}
inline static void ram_page_write32(uint32_t addr32, uint32_t value) {
    ((vu32u32_ptr_t)_sys_table_ptrs[98])(addr32, value);
}

typedef struct {
    char* del_addr;
    char* prg_addr;
    uint16_t sec_num;
} sect_entry_t;

typedef int (*bootb_ptr_t)( void );

typedef struct {
    bootb_ptr_t bootb[4];
    sect_entry_t* sect_entries;
} bootb_ctx_t;

typedef struct {
    const char* key;
    char* value;
} vars_t;

typedef enum {
    INITIAL,
    PREPARED,
    FOUND,
    VALID,
    LOAD,
    EXECUTED,
    INVALIDATED
} cmd_exec_stage_t;

typedef struct cmd_ctx {
    uint32_t argc;
    char** argv;
    char* orig_cmd;

    FIL* std_in;
    FIL* std_out;
    FIL* std_err;
    int ret_code;
    bool detached;

    char* curr_dir;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* pipe;

    cmd_exec_stage_t stage;
} cmd_ctx_t;
/*
inline static cmd_startup_ctx_t* get_cmd_startup_ctx() {
    typedef cmd_startup_ctx_t* (*f_ptr_t)();
    return ((f_ptr_t)_sys_table_ptrs[99])();
}
*/
typedef int (*ipc_ptr_t)(const char *);
inline static int atoi (const char * s) {
    return ((ipc_ptr_t)_sys_table_ptrs[100])(s);
}
inline static void overclocking() {
    return ((vv_ptr_t)_sys_table_ptrs[101])();
}
typedef void (*vu32u32u32_ptr_t)(uint32_t, uint32_t, uint32_t);
inline static void overclocking_ex(uint32_t vco, int32_t postdiv1, int32_t postdiv2) {
    return ((vu32u32u32_ptr_t)_sys_table_ptrs[102])(vco, postdiv1, postdiv2);
}
typedef void (*vu32_ptr_t)(uint32_t);
inline static void set_overclocking(uint32_t khz) {
    return ((vu32_ptr_t)_sys_table_ptrs[104])(khz);
}
inline static uint32_t get_overclocking_khz() {
    return ((u32v_ptr_t)_sys_table_ptrs[103])();
}

inline static void set_sys_clock_pll(uint32_t vco_freq, uint32_t post_div1, uint32_t post_div2) {
    ((vu32u32u32_ptr_t)_sys_table_ptrs[105])(vco_freq, post_div1, post_div2);
} 
typedef bool (*bu32pu32pu32pu32_ptr_t)(uint32_t, uint32_t*, uint32_t*, uint32_t*);
inline static bool check_sys_clock_khz(uint32_t freq_khz, uint32_t *vco_out, uint32_t *postdiv1_out, uint32_t *postdiv2_out) {
    return ((bu32pu32pu32pu32_ptr_t)_sys_table_ptrs[106])(freq_khz, vco_out, postdiv1_out, postdiv2_out);
}

typedef char* (*pcpc_ptr_t)(char*);
inline static char* next_token(char* t) {
    return ((pcpc_ptr_t)_sys_table_ptrs[107])(t);
}

inline static size_t strlen(const char * s) {
    typedef size_t (*fn_ptr_t)(const char * s);
    return ((fn_ptr_t)_sys_table_ptrs[62])(s);
}

inline static char* strncpy(char* t, const char * s, size_t sz) {
    typedef char* (*fn_ptr_t)(char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[63])(t, s, sz);
}

inline static char* strcpy(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[60])(t, s);
}

inline static int strcmp(const char * s1, const char * s2) {
    typedef int (*fn_ptr_t)(const char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[108])(s1, s2);
}

inline static int strncmp(const char * s1, const char * s2, size_t sz) {
    typedef int (*fn_ptr_t)(const char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[109])(s1, s2, sz);
}

/* Used to pass information about the heap out of vPortGetHeapStats(). */
typedef struct xHeapStats
{
    size_t xAvailableHeapSpaceInBytes;      /* The total heap size currently available - this is the sum of all the free blocks, not the largest block that can be allocated. */
    size_t xSizeOfLargestFreeBlockInBytes;  /* The maximum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xSizeOfSmallestFreeBlockInBytes; /* The minimum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xNumberOfFreeBlocks;             /* The number of free memory blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xMinimumEverFreeBytesRemaining;  /* The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted. */
    size_t xNumberOfSuccessfulAllocations;  /* The number of calls to pvPortMalloc() that have returned a valid memory block. */
    size_t xNumberOfSuccessfulFrees;        /* The number of calls to vPortFree() that has successfully freed a block of memory. */
} HeapStats_t;
inline static void vPortGetHeapStats( HeapStats_t * pxHeapStats ) {
    typedef void (*fn_ptr_t)(HeapStats_t *);
    ((fn_ptr_t)_sys_table_ptrs[110])(pxHeapStats);
}

inline static uint32_t get_cpu_ram_size() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[111])();
}
inline static uint32_t get_cpu_flash_size() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[112])();
}

inline static FATFS* get_mount_fs() { // only one FS is supported foe now
    typedef FATFS* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[113])();
}
inline static uint32_t f_getfree32(FATFS * fs) {
    typedef uint32_t (*fn_ptr_t)(FATFS*);
    return ((fn_ptr_t)_sys_table_ptrs[114])(fs);
}

typedef bool (*scancode_handler_t)(const uint32_t);
inline static scancode_handler_t get_scancode_handler() {
    typedef scancode_handler_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[115])();
}
inline static void set_scancode_handler(scancode_handler_t h) {
    typedef void (*fn_ptr_t)(scancode_handler_t);
    ((fn_ptr_t)_sys_table_ptrs[116])(h);
}
typedef bool (*cp866_handler_t)(const char, uint32_t);
inline static cp866_handler_t get_cp866_handler() {
    typedef cp866_handler_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[117])();
}
inline static void set_cp866_handler(cp866_handler_t h) {
    typedef void (*fn_ptr_t)(cp866_handler_t);
    ((fn_ptr_t)_sys_table_ptrs[118])(h);
}
inline static void gbackspace() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[119])();
}

inline static bool load_firmware(char* path) {
    typedef bool (*fn_ptr_t)(char*);
    return ((fn_ptr_t)_sys_table_ptrs[65])(path);
}
inline static void run_app(char * name) {
    typedef void (*fn_ptr_t)(char*);
    ((fn_ptr_t)_sys_table_ptrs[66])(name);
}

inline static bool is_new_app(char * name) {
    typedef bool (*fn_ptr_t)(char*);
    return ((fn_ptr_t)_sys_table_ptrs[120])(name);
}
inline static int run_new_app(char * name) {
    typedef int (*fn_ptr_t)(char*);
    return ((fn_ptr_t)_sys_table_ptrs[121])(name);
}

inline static char getc(void) {
    typedef char (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[122])();
}
inline static void putc(char c) {
    typedef void (*fn_ptr_t)(char);
    ((fn_ptr_t)_sys_table_ptrs[123])(c);
}

inline static void cleanup_bootb_ctx(bootb_ctx_t* b) {
    typedef void (*fn_ptr_t)(bootb_ctx_t*);
    ((fn_ptr_t)_sys_table_ptrs[124])(b);
}
inline static int load_app(char * fn, bootb_ctx_t* bootb_ctx) {
    typedef int (*fn_ptr_t)(char * fn, bootb_ctx_t* bootb_ctx);
    return ((fn_ptr_t)_sys_table_ptrs[125])(fn, bootb_ctx);
}
inline static void exec(cmd_ctx_t* ctx) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx);
    ((fn_ptr_t)_sys_table_ptrs[126])(ctx);
}

inline static char* exists(cmd_ctx_t* ctx) {
    typedef char* (*fn_ptr_t)(cmd_ctx_t*);
    return ((fn_ptr_t)_sys_table_ptrs[128])(ctx);
}
inline static char* concat(const char* s1, const char* s2) {
    typedef char* (*fn_ptr_t)(const char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[129])(s1, s2);
}

inline static size_t get_heap_total() {
    typedef size_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[132])();
}
inline static uint32_t swap_pages() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[133])();
}
inline static uint16_t* swap_pages_base() {
    typedef uint16_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[134])();
}
inline static uint32_t swap_page_size() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[135])();
}

inline static char* copy_str(const char* s) {
    typedef char* (*fn_ptr_t)(const char*);
    return ((fn_ptr_t)_sys_table_ptrs[137])(s);
}
inline static cmd_ctx_t* get_cmd_ctx() {
    typedef cmd_ctx_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[138])();
}
inline static void cleanup_ctx(cmd_ctx_t* src) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*);
    ((fn_ptr_t)_sys_table_ptrs[139])(src);
}

inline static void* malloc(size_t xWantedSize) {
    typedef void* (*pvPortMalloc_ptr_t)( size_t xWantedSize );
    return ((pvPortMalloc_ptr_t)_sys_table_ptrs[32])(xWantedSize);
}
inline static void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}
inline static void* calloc(size_t xWantedSize) {
    typedef char* (*pvPortMalloc_ptr_t)( size_t xWantedSize );
    char* t = ((pvPortMalloc_ptr_t)_sys_table_ptrs[32])(xWantedSize); // allign?
    memset(t, 0, xWantedSize);
    return t;
}
inline static void free(void * pv) {
    typedef void (*vPortFree_ptr_t)( void * pv );
    ((vPortFree_ptr_t)_sys_table_ptrs[33])(pv);
}
inline static char* get_ctx_var(cmd_ctx_t* src, const char* k) {
    typedef char* (*fn_ptr_t)(cmd_ctx_t*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[140])(src, k);
}
inline static cmd_ctx_t* set_ctx_var(cmd_ctx_t* src, const char* k, char* v) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*, const char*, char*);
    ((fn_ptr_t)_sys_table_ptrs[141])(src, k, v);
}
inline static  cmd_ctx_t* clone_ctx(cmd_ctx_t* src) {
    typedef cmd_ctx_t* (*fn_ptr_t)(cmd_ctx_t*);
    return ((fn_ptr_t)_sys_table_ptrs[143])(src);
}

#ifdef __cplusplus
}
#endif
