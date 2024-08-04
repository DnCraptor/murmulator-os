#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !M_API_VERSION
#define M_API_VERSION 14
#endif

#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
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
inline static bool graphics_set_mode(int color) {
    typedef bool (*ptr_t)( int color );
    return ((ptr_t)_sys_table_ptrs[34])(color);
}
inline static void clrScr(uint8_t color) {
    typedef void (*cls_ptr_t)( uint8_t color );
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
#define fprintf(...) ((fgoutf_ptr_t)_sys_table_ptrs[70])(__VA_ARGS__)

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

    struct cmd_ctx* prev;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* next;

    cmd_exec_stage_t stage;
} cmd_ctx_t;

inline static cmd_ctx_t* get_cmd_startup_ctx() {
    typedef cmd_ctx_t* (*f_ptr_t)();
    return ((f_ptr_t)_sys_table_ptrs[99])();
}

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

inline static bool is_new_app(cmd_ctx_t* c) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t* c);
    return ((fn_ptr_t)_sys_table_ptrs[120])(c);
}
inline static bool run_new_app(cmd_ctx_t* c) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t* c);
    return ((fn_ptr_t)_sys_table_ptrs[121])(c);
}

inline static char getch(void) {
    typedef char (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[122])();
}
inline static void putc(char c) {
    typedef void (*fn_ptr_t)(char);
    ((fn_ptr_t)_sys_table_ptrs[123])(c);
}

inline static void cleanup_bootb_ctx(cmd_ctx_t* ctx) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx);
    ((fn_ptr_t)_sys_table_ptrs[124])(ctx);
}
inline static bool load_app(cmd_ctx_t* ctx) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t* ctx);
    return ((fn_ptr_t)_sys_table_ptrs[125])(ctx);
}
inline static void exec(cmd_ctx_t* ctx) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx);
    ((fn_ptr_t)_sys_table_ptrs[126])(ctx);
}

inline static bool exists(cmd_ctx_t* ctx) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t*);
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
/*
inline static void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}
*/
inline static void* calloc(size_t cnt, size_t sz) {
    typedef char* (*pvPortCalloc_ptr_t)( size_t cnt, size_t sz );
    return ((pvPortCalloc_ptr_t)_sys_table_ptrs[166])(cnt, sz);
}
inline static void* pvPortCalloc(size_t cnt, size_t sz) {
    typedef char* (*pvPortCalloc_ptr_t)( size_t cnt, size_t sz );
    return ((pvPortCalloc_ptr_t)_sys_table_ptrs[166])(cnt, sz);
}
inline static size_t xPortGetFreeHeapSize( void ) {
    typedef size_t (*_ptr_t)(void);
    return ((_ptr_t)_sys_table_ptrs[145])();
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
inline static cmd_ctx_t* clone_ctx(cmd_ctx_t* src) {
    typedef cmd_ctx_t* (*fn_ptr_t)(cmd_ctx_t*);
    return ((fn_ptr_t)_sys_table_ptrs[143])(src);
}
inline static void remove_ctx(cmd_ctx_t* src) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*);
    ((fn_ptr_t)_sys_table_ptrs[144])(src);
}

inline static uint32_t get_console_width() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[146])();
}
inline static uint32_t get_console_height() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[147])();
}
inline static uint32_t get_screen_width() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[158])();
}
inline static uint32_t get_screen_height() {
    typedef uint32_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[159])();
}
typedef FIL FILE;
inline static int getc(FILE* in) { // return -1 for EOF
    typedef int (*fn_ptr_t)(FILE* in);
    return ((fn_ptr_t)_sys_table_ptrs[148])(in);
}

inline static uint8_t* get_buffer() {
    typedef uint8_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[152])();
}
inline static size_t get_buffer_size() {
    typedef size_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[153])();
}
inline static uint8_t get_screen_bitness() {
    typedef uint8_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[154])();
}
inline static void cleanup_graphics() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[155])();
}
inline static uint8_t get_console_bitness() {
    typedef uint8_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[157])();
}

typedef void (*vv_fn)(void);
typedef void (*vb_fn)(bool);
typedef bool (*bi_fn)(int);
typedef int (*iv_fn)(void);
typedef bool (*bv_fn)(void);
typedef uint32_t (*u32v_fn)(void);
typedef uint8_t* (*pu8v_fn)(void);
typedef uint8_t (*u8v_fn)(void);
typedef void (*vpu8_fn)(uint8_t*);
typedef void (*vu8_fn)(uint8_t);
typedef void (*dt_fn)(const char*, int, int, uint8_t, uint8_t);
typedef void (*vii_fn)(const int, const int);
typedef void (*vu8u8_fn)(uint8_t, uint8_t);
typedef void (*vcu32_fn)(const uint32_t);
typedef uint8_t* (*dma_handler_impl_fn)(void);
typedef void (*vu32u32)(uint32_t, uint32_t);

typedef struct graphics_driver {
    cmd_ctx_t* ctx;
    vv_fn init;
    vv_fn cleanup;
    bi_fn set_mode;
    bv_fn is_text;
    u32v_fn console_width;
    u32v_fn console_height;
    u32v_fn screen_width;
    u32v_fn screen_height;
    pu8v_fn buffer;
    vpu8_fn set_buffer;
    vu8_fn cls;
    dt_fn draw_text;
    u8v_fn console_bitness;
    u8v_fn screen_bitness;
    vii_fn set_offsets;
    vcu32_fn set_bgcolor;
    u32v_fn allocated;
    vii_fn set_con_pos;
    iv_fn pos_x;
    iv_fn pos_y;
    vu8u8_fn set_con_color;
    vpu8_fn print;
    vv_fn backspace;
    vb_fn lock_buffer;
    iv_fn get_mode;
    bi_fn is_mode_text;
    vu8_fn set_cursor_color;
} graphics_driver_t;
inline static void install_graphics_driver(graphics_driver_t* gd) {
    typedef void (*fn_ptr_t)(graphics_driver_t* gd);
    ((fn_ptr_t)_sys_table_ptrs[156])(gd);
}
inline static graphics_driver_t* get_graphics_driver() {
    typedef graphics_driver_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[160])();
}
inline static bool is_buffer_text(void) {
    typedef bool (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[161])();
}
inline static int graphics_get_mode(void) {
    typedef int (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[162])();
}
inline static bool graphics_is_mode_text(int mode) {
    typedef bool (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[163])(mode);
}

typedef uint8_t* (*dma_handler_impl_fn)(void);
inline static void set_vga_dma_handler_impl(dma_handler_impl_fn impl) {
    typedef void (*fn_ptr_t)(dma_handler_impl_fn);
    return ((fn_ptr_t)_sys_table_ptrs[164])(impl);
}

inline static void set_graphics_clkdiv(uint32_t pixel_clock, uint32_t line_size) {
    typedef void (*fn_ptr_t)(uint32_t, uint32_t);
    ((fn_ptr_t)_sys_table_ptrs[165])(pixel_clock, line_size);
}

inline static void vga_dma_channel_set_read_addr(const volatile void* addr) {
    typedef void (*fn_ptr_t)(const volatile void*);
    ((fn_ptr_t)_sys_table_ptrs[168])(addr);
}

#ifndef __compar_fn_t_defined
#define __compar_fn_t_defined
typedef int (*__compar_fn_t) (const void *, const void *);
#endif
inline static void qsort(void *__base, size_t __nmemb, size_t __size, __compar_fn_t _compar) {
    typedef void (*fn_ptr_t)(void *__base, size_t __nmemb, size_t __size, __compar_fn_t _compar);
    ((fn_ptr_t)_sys_table_ptrs[169])(__base, __nmemb, __size, _compar);
}

inline static size_t strnlen(const char *str, size_t sz) {
    typedef size_t (*fn_ptr_t)(const char *str, size_t sz);
    return ((fn_ptr_t)_sys_table_ptrs[170])(str, sz);
}

inline static void psram_id(uint8_t rx[8]) {
    typedef void (*fn_ptr_t)(uint8_t rx[8]);
    ((fn_ptr_t)_sys_table_ptrs[178])(rx);
}

#define DPAD_LEFT 0x40
#define DPAD_RIGHT 0x80
#define DPAD_DOWN 0x20
#define DPAD_UP 0x10
#define DPAD_START 0x08
#define DPAD_SELECT 0x04
#define DPAD_B 0x02
#define DPAD_A 0x01

#define PS2_LED_SCROLL_LOCK 1
#define PS2_LED_NUM_LOCK    2
#define PS2_LED_CAPS_LOCK   4

inline static uint8_t get_leds_stat() {
    typedef uint8_t (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[64])();
}

// USB
// (better use usb_driver)
inline static void init_pico_usb_drive() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[179])();
}

// (better use usb_driver)
inline static void pico_usb_drive_heartbeat() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[180])();
}

inline static bool tud_msc_ejected() {
    typedef bool (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[181])();
}

// (better use usb_driver)
inline static void set_tud_msc_ejected(bool v) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[182])(v);
}

inline static void show_logo(bool with_top) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[183])(with_top);
}

inline static char getch_now(void) {
    typedef char (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[184])();
}

inline static void usb_driver(bool on) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[185])(on);
}

inline static void set_cursor_color(uint8_t c) {
    typedef void (*fn_ptr_t)(uint8_t);
    ((fn_ptr_t)_sys_table_ptrs[186])(c);
}

#ifdef __cplusplus
}
// TODO: separate h-file
inline void* operator new(size_t sz) {
    return malloc(sz);
}
inline void operator delete(void* p) {
    return free(p);
}
inline void operator delete(void* p, size_t) {
    return free(p);
}
inline void* operator new[](size_t sz) {
    return malloc(sz);
}
inline void operator delete[](void* p) {
    return free(p);
}
inline void operator delete[](void* p, size_t) {
    return free(p);
}
// TODO: separate h-file
class string {
    size_t sz;
    size_t alloc;
    char* p;
public:
    inline string(): sz(1), alloc(16), p(new char[16]) { p[0] = 0; }
    inline ~string() { delete[] p; }
    inline string(const char* s) {
        sz = strlen(s) + 1;
        alloc = sz + 16;
        p = new char[alloc];
        strncpy(p, s, sz);
    }
    inline string(const string& s): sz(s.sz), alloc(s.alloc) {
        p = new char[alloc];
        strncpy(p, s.p, sz);
    }
    inline const char* c_str() const { return p; }
    inline string& operator=(const string& s) {
        if (this == &s) {
            return *this;
        }
        delete[] p;
        sz = s.sz;
        alloc = s.alloc;
        p = new char[alloc];
        strncpy(p, s.p, sz);
        return *this;
    }
    inline string& operator=(const char* s) {
        sz = strlen(s) + 1;
        if (sz < alloc) {
            strncpy(p, s, sz);
            return *this;
        }
        delete[] p;
        alloc = sz + 16;
        p = new char[alloc];
        strncpy(p, s, sz);
        return *this;
    }
    inline char operator[](size_t i) const {
        return p[i];
    }
    inline size_t size() const { return sz - 1; }
    inline string& operator+=(const char c) {
        if (sz < alloc) {
            p[sz - 1] = c;
            p[sz++] = 0;
            return *this;
        }
        alloc += 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        p[sz - 1] = c;
        p[sz++] = 0;
        return *this;
    }
    inline string& operator+=(const char* s) {
        size_t n_sz = strlen(s);
        if (sz + n_sz < alloc) {
            strncpy(p + sz - 1, s, n_sz + 1);
            sz += n_sz;
            return *this;
        }
        alloc += n_sz + 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        strncpy(p + sz - 1, s, n_sz + 1);
        sz += n_sz;
        return *this;
    }
    inline string& operator+=(const string& s) {
        if (sz + s.sz - 1 < alloc) {
            strncpy(p + sz - 1, s.p, s.sz);
            sz += s.sz - 1;
            return *this;
        }
        alloc += s.sz + 16;
        char* n_p = new char[alloc];
        strncpy(n_p, p, sz);
        delete[] p;
        p = n_p;
        strncpy(p + sz - 1, s.p, s.sz);
        sz += s.sz - 1;
        return *this;
    }
    inline string& operator+(const string& s2) {
        *this += s2;
        return *this;
    }
    inline friend string operator+(const string& s1, const string& s2) {
        string s(s1);
        s += s2;
        return s;
    }
};
#else

typedef struct string {
    size_t size; // string size excluding end-0
    size_t alloc; // really allocated bytes
    char* p;
} string_t;

inline static string_t* new_string_v(void) {
    string_t* res = malloc(sizeof(string_t));
    res->size = 0,
    res->alloc = 16;
    res->p = malloc(16);
    res->p[0] = 0;
    return res;
}

inline static void delete_string(string_t* s) {
    free(s->p);
    free(s);
}

inline static string_t* new_string_cc(const char* s) {
    string_t* res = malloc(sizeof(string_t));
    res->size = strlen(s);
    res->alloc = res->size + 1;
    res->p = malloc(res->alloc);
    strncpy(res->p, s, res->alloc);
    return res;
}

inline static void string_reseve(string_t* s, size_t alloc) {
    if (s->alloc >= alloc) return; // already more or eq. than requested
    char* n_p = malloc(alloc);
    strncpy(n_p, s->p, s->alloc);
    free(s->p);
    s->p = n_p;
    s->alloc = alloc;
}

inline static void string_push_back_c(string_t* s, const char c) {
    if (s->size + 1 == s->alloc) {
        string_reseve(s, s->alloc + 16);
    }
    s->p[s->size] = c;
    s->p[++s->size] = 0;
}

inline static void string_push_back_cs(string_t* s, const string_t* cs) {
    size_t sz = s->size + cs->size;
    if (sz >= s->alloc) {
        string_reseve(s, sz + 1);
    }
    memcpy(s->p + s->size, cs->p, cs->size);
    s->p[sz] = 0;
    s->size = sz;
}

inline void string_clip(string_t* s, size_t idx) {
    if (idx >= s->size) return;
    for (size_t i = idx; i <= s->size; ++i) {
        s->p[i] = s->p[i + 1];
    }
    if (s->size) --s->size;
}

inline void string_insert_c(string_t* s, char c, size_t idx) {
    string_reseve(s, idx + 1);
    if (idx >= s->size) {
        size_t sps = idx - s->size;
        memset(s->p + s->size, ' ', sps);
        s->size += sps;
        string_push_back_c(s, c);
        return;
    }
    for (size_t i = s->size; i > idx; --i) {
        s->p[i] = s->p[i - 1];
    }
    s->p[idx] = c;
    s->p[++s->size] = 0;
}

inline string_t* string_split_at(string_t* s, size_t idx) {
    if (idx >= s->size) {
        return new_string_v();
    }
    string_t* res = new_string_cc(s->p + idx);
    s->p[idx] = 0;
    s->size = idx;
    return res;
}

inline static const char* c_str(const string_t* s) {
    return s->p;
}
inline static size_t c_strlen(const string_t* s) {
    return s->size;
}

typedef struct node {
    struct node* prev;
    struct node* next;
    void* data;
} node_t;

typedef void (*dealloc_fn_ptr_t)(void*);
typedef void* (*alloc_fn_ptr_t)(void);
typedef struct list {
    alloc_fn_ptr_t allocator;
    dealloc_fn_ptr_t deallocator;
    node_t* first;
    node_t* last;
} list_t;

inline static list_t* new_list_v(alloc_fn_ptr_t allocator, dealloc_fn_ptr_t deallocator) {
    list_t* res = malloc(sizeof(list_t));
    res->allocator = allocator ? allocator : malloc;
    res->deallocator = deallocator ? deallocator : free;
    res->first = NULL;
    res->last = NULL;
    return res;
}

inline static void list_push_back(list_t* lst, void* s) {
    node_t* i = malloc(sizeof(node_t));
    i->prev = lst->last;
    i->next = NULL;
    i->data = s;
    if (lst->first == NULL) lst->first = i;
    if (lst->last != NULL) lst->last->next = i;
    lst->last = i;
}

inline static void delete_list(list_t* lst) {
    node_t* i = lst->last;
    while(i) {
        node_t* prev = i->prev;
        if (i->data) {
            lst->deallocator(i->data);
        }
        free(i);
        i = prev;
    }
    free(lst);
}

inline static node_t* list_inject_till(list_t* lst, size_t idx) {
    node_t* i = lst->first;
    size_t n = 0;
    while (i) {
        if (!i->next) {
            list_push_back(lst, lst->allocator());
        }
        if (n == idx) {
            return i;
        }
        i = i->next;
        ++n;
    }
    // unreachable
    return NULL;
}

inline static node_t* list_get_node_at(list_t* lst, size_t idx) {
    node_t* i = lst->first;
    size_t n = 0;
    while (i) {
        if (n == idx) {
            return i;
        }
        i = i->next;
        ++n;
    }
    return NULL;
}

inline static void* list_get_data_at(list_t* lst, size_t idx) {
    node_t* i = list_get_node_at(lst, idx);
    return i ? i->data : NULL;
}

inline static void list_inset_node_after(list_t* lst, node_t* n, node_t* i) {
    i->prev = n;
    i->next = n->next;
    n->next = i;
    if (n == lst->last) lst->last = i;
}

inline static void list_inset_data_after(list_t* lst, node_t* n, void* s) {
    node_t* i = malloc(sizeof(node_t));
    i->data = s;
    list_inset_node_after(lst, n, i);
}

inline static void list_erase_node(list_t* lst, node_t* n) {
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    if (n->data) lst->deallocator(n->data);
    if (lst->first == n) lst->first = n->next;
    if (lst->last == n) lst->last = n->prev;
    free(n);
}

#endif
