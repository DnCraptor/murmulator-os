#include <cstdlib>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "psram_spi.h"

#include "graphics.h"

extern "C" {
#include "ps2.h"
#include "ff.h"
//#include "usb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hooks.h"

#include "tests.h"
#include "sys_table.h"
#include "portable.h"

#include "keyboard.h"
#include "cmd.h"
#include "hardfault.h"
#include "hardware/exception.h"
#include "ram_page.h"
#include "overclock.h"
#include "app.h"
}

#include "nespad.h"

static FATFS fs;
extern "C" FATFS* get_mount_fs() { // only one FS is supported foe now
    return &fs;
}
semaphore vga_start_semaphore;

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    // graphics_driver_t* gd = get_graphics_driver();
    // install_graphics_driver(gd);

    sem_acquire_blocking(&vga_start_semaphore);
    // 60 FPS loop
#define frame_tick (16666)
    uint64_t tick = time_us_64();
#ifdef TFT
    uint64_t last_renderer_tick = tick;
#endif
    uint64_t last_input_tick = tick;
    while (true) {
#ifdef TFT
        if (tick >= last_renderer_tick + frame_tick) {
            refresh_lcd();
            last_renderer_tick = tick;
        }
#endif
        // Every 5th frame
        if (tick >= last_input_tick + frame_tick * 5) {
    ///        nespad_read();
            last_input_tick = tick;
        }
        tick = time_us_64();

        tight_loop_contents();
    }

    __unreachable();
}

inline static void tokenizeCfg(char* s, size_t sz) {
    size_t i = 0;
    for (; i < sz; ++i) {
        if (s[i] == '=' || s[i] == '\n' || s[i] == '\r') {
            s[i] = 0;
        }
    }
    s[i] = 0;
}

static void __always_inline run_application() {
    multicore_reset_core1();

    asm volatile (
        "mov r0, %[start]\n"
        "ldr r1, =%[vtable]\n"
        "str r0, [r1]\n"
        "ldmia r0, {r0, r1}\n"
        "msr msp, r0\n"
        "bx r1\n"
        :: [start] "r" (XIP_BASE + 0x100), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
    );

    __unreachable();
}

static void __always_inline check_firmware() {
    FIL f;
    if(f_open(&f, FIRMWARE_MARKER_FN, FA_READ) == FR_OK) {
        f_close(&f);
        f_unmount("SD");
        run_application();
    }
}

inline static void unlink_firmware() {
    f_unlink(FIRMWARE_MARKER_FN);
}

static const char CD[] = "CD"; // current directory 
static const char BASE[] = "BASE"; 
static const char MOS[] = "MOS"; 
static const char TEMP[] = "TEMP"; 
static const char PATH[] = "PATH"; 
static const char SWAP[] = "SWAP"; 
static const char COMSPEC[] = "COMSPEC"; 
static const char ctmp[] = "/tmp"; 
static const char ccmd[] = "/mos/cmd"; 

static void load_config_sys() {
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    set_ctx_var(ctx, CD, MOS);
    set_ctx_var(ctx, BASE, MOS);
    set_ctx_var(ctx, TEMP, ctmp);
    set_ctx_var(ctx, COMSPEC, ccmd);

    bool b_swap = false;
    bool b_base = false;
    bool b_temp = false;
    char* buff = 0;
    FIL* pf = 0;
    
    FILINFO* pfileinfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    size_t file_size = 0;
    char * cfn = "/config.sys";
    if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
        cfn = "/mos/config.sys";
        if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
            goto e;
        } else {
            file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
        }
    } else {
        file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
    }

    pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if(f_open(pf, cfn, FA_READ) != FR_OK) {
        goto e;
    }

    buff = (char*)pvPortMalloc(file_size + 1);
    memset(buff, 0, file_size + 1);
    UINT br;
    if (f_read(pf, buff, file_size, &br) != FR_OK) {
        goutf("Failed to read config.sys\n");
        f_close(pf);
    } else {
        f_close(pf);
        tokenizeCfg(buff, br);
        char *t = buff;
        while (t - buff < br) {
            // goutf("%s\n", t);
            if (strcmp(t, PATH) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, PATH, t);
            } else if (strcmp(t, TEMP) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, TEMP, t);
                f_mkdir(t);
                b_temp = true;
            } else if (strcmp(t, COMSPEC) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, COMSPEC, t);
            } else if (strcmp(t, SWAP) == 0) {
                t = next_token(t);
                init_vram(t);
                b_swap = true;
            } else if (strcmp(t, "GMODE") == 0) {
                t = next_token(t);
                int mode = atoi(t);
               
            } else if (strcmp(t, BASE) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, BASE, t);
                f_mkdir(t);
                b_base = true;
            } else if (strcmp(t, CD) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, CD, t);
                f_mkdir(t);
            } else {
                char* k = copy_str(t);
                t = next_token(t);
                set_ctx_var(ctx, k, t);
            }
            t = next_token(t);
        }
    }
    vPortFree(pfileinfo);
    if (pf) vPortFree(pf);
    if (buff) vPortFree(buff);
e:
    if (!b_temp) f_mkdir(ctmp);
    if (!b_base) f_mkdir(MOS);
    if (!b_swap) {
        char* t1 = "/tmp/pagefile.sys 1M 64K 4K";
        char* t2 = (char*)pvPortMalloc(strlen(t1) + 1);
        strcpy(t2, t1);
        init_vram(t2);
        vPortFree(t2);
    }
}

static inline void try_to_restore_api_tbl(cmd_ctx_t* ctx) {
    char* t = get_ctx_var(ctx, TEMP);
    t = concat(t ? t : "", OS_TABLE_BACKUP_FN);
    restore_tbl(t); // TODO: error handling
    vPortFree(t);
}

extern "C" void vCmdTask(void *pv) {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    try_to_restore_api_tbl(ctx);

    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
    while(1) {
        if (!ctx->argc && !ctx->argv) {
            ctx->argc = 1;
            ctx->argv = (char**)pvPortMalloc(sizeof(char*));
            char* comspec = get_ctx_var(ctx, COMSPEC);
            ctx->argv[0] = copy_str(comspec);
            if(ctx->orig_cmd) vPortFree(ctx->orig_cmd);
            ctx->orig_cmd = copy_str(comspec);
        }
        // goutf("Lookup for: %s\n", ctx->orig_cmd);
        // goutf("be [%p]\n", xPortGetFreeHeapSize());
        bool b_exists = exists(ctx);
        // goutf("ae [%p]\n", xPortGetFreeHeapSize());
        if (b_exists) {
            size_t len = strlen(ctx->orig_cmd); // TODO: more than one?
            // goutf("Command found: %s\n", ctx->orig_cmd);
            if (len > 3 && strcmp(ctx->orig_cmd + len - 4, ".uf2") == 0) {
                if(load_firmware(ctx->orig_cmd)) { // TODO: by ctx
                    ctx->stage = LOAD;
                    run_app(ctx->orig_cmd);
                    ctx->stage = EXECUTED;
                    cleanup_ctx(ctx);
                } else {
                    goutf("Unable to execute command: '%s' (failed to load it)\n", ctx->orig_cmd);
                    ctx->stage = INVALIDATED;
                    goto e;
                }
            } else if(is_new_app(ctx)) {
                // gouta("Command has appropriate format\n");
                if (load_app(ctx)) {
                    // goutf("Load passed for: %s\n", ctx->orig_cmd);
                    exec(ctx);
                   // while(!__getch());
                    // goutf("Exec passed for: %s\n", ctx->orig_cmd);
                } else {
                    goutf("Unable to execute command: '%s' (failed to load it)\n", ctx->orig_cmd);
                    ctx->stage = INVALIDATED;
                    goto e;
                }
            } else {
                goutf("Unable to execute command: '%s' (unknown format)\n", ctx->orig_cmd);
                ctx->stage = INVALIDATED;
                goto e;
            }
        } else {
            goutf("Illegal command: '%s'\n", ctx->orig_cmd);
            ctx->stage = INVALIDATED;
            goto e;
        }
        // repair system context
        ctx = get_cmd_startup_ctx();
        vTaskSetThreadLocalStoragePointer(th, 0, ctx);
        continue;
e:
        if (ctx->stage != PREPARED) { // it is expected cmd/cmd0 will prepare ctx for next run for application, in other case - cleanup ctx
            cleanup_ctx(ctx);
        }
    }
    vTaskDelete( NULL );
}

const char* tmp = "                      ZX Murmulator (RP2040) OS v."
                    MOS_VERSION_STR
                   " Alpha                   ";

extern "C" void mallocFailedHandler() {
    gouta("WARN: vApplicationMallocFailedHook\n");
}

extern "C" void overflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    gouta("WARN: vApplicationStackOverflowHook\n");
}

#ifdef DEBUG_VGA
extern "C" char vga_dbg_msg[1024];
#endif

static void init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(get_overclocking_khz(), true);

    keyboard_init();
    keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    nespad_read();
    sleep_ms(50);

    gpio_put(PICO_DEFAULT_LED_PIN, true);
    bool mount_res = (FR_OK == f_mount(&fs, "SD", 1));

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(30);
    clrScr(1);
#ifdef DEBUG_VGA
    FIL f;
    f_open(&f, "mos-vga.log", FA_OPEN_APPEND | FA_WRITE);
    UINT wr;
    f_write(&f, vga_dbg_msg, strlen(vga_dbg_msg), &wr);
    f_close(&f);
#endif
    // F12 Boot to USB FIRMWARE UPDATE mode
    if (nespad_state & DPAD_START || get_last_scancode() == 0x58 /*F12*/) {
        unlink_firmware();
        reset_usb_boot(0, 0);
    }
    if (nespad_state & DPAD_SELECT || get_last_scancode() == 0x57 /*F11*/) {
        unlink_firmware(); // return to OS
    }
    if (mount_res) {
        check_firmware();
    }

    draw_text(tmp, 0, 0, 13, 1);
    draw_text(tmp, 0, 29, 13, 1);
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0); // TODO: config
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
   
    if (!mount_res) {
        graphics_set_con_color(12, 0);
        gouta("SD Card not inserted or SD Card error!");
        while (true);
    }
    load_config_sys();
    draw_text(tmp, 0, 0, 13, 1);

    init_psram();
}

static void info() {
    uint32_t ram32 = get_cpu_ram_size();
    uint32_t flash32 = get_cpu_flash_size();
    uint32_t psram32 = psram_size();
    FATFS* fs = get_mount_fs();
    goutf("CPU %d MHz\n"
          "SRAM %d KB\n"
          "FLASH %d MB\n"
          "PSRAM %d MB\n"
          "SDCARD %d FATs; %d free clusters; cluster size: %d KB\n"
          "SWAP %d MB; RAM: %d KB; pages index: %d x %d KB\n"
          "VRAM %d KB; video mode: %d x %d x %d bit\n"
          "\n",
          get_overclocking_khz() / 1000,
          ram32 >> 10,
          flash32 >> 20,
          psram32 >> 20,
          fs->n_fats,
          f_getfree32(fs),
          fs->csize >> 1,
          swap_size() >> 20, swap_base_size() >> 10, swap_pages(), swap_page_size() >> 10,
          get_buffer_size() >> 10, get_console_width(), get_console_height(), get_console_bitness()
    );
}

int main() {
    init();
    info();

    xTaskCreate(vCmdTask, "cmd", 1024/*x4=4096k*/, NULL, configMAX_PRIORITIES - 1, NULL);

    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    setApplicationStackOverflowHookPtr(overflowHook);

    /* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    while(sys_table_ptrs[0]); // to ensure linked (TODO: other way)

    __unreachable();
}
