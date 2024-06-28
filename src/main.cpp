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
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)

static uint8_t* SCREEN = 0;

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    const auto buffer = (uint8_t *)SCREEN;
    graphics_set_buffer(buffer, TEXTMODE_COLS, TEXTMODE_ROWS);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(0, 0);
    graphics_set_mode(TEXTMODE_DEFAULT);
    clrScr(1);

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

static char* comspec = "/mos/cmd";

static void load_config_sys() {
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    set_ctx_var(ctx, "BASE", "MOS");
    set_ctx_var(ctx, "TEMP", "tmp");
    FIL f;
    if(f_open(&f, "/config.sys", FA_READ) != FR_OK) {
        if(f_open(&f, "/mos/config.sys", FA_READ) != FR_OK) {
            f_mkdir("MOS"); f_mkdir("tmp");
            ctx->curr_dir = copy_str("MOS");
            return;
        }
    }
    char* buff = (char*)pvPortMalloc(512); // todo: filesize
    UINT br;
    if (f_read(&f, buff, 512, &br) != FR_OK) {
        goutf("Failed to read config.sys\n");
    }
    f_close(&f);
    tokenizeCfg(buff, br);
    char *t = buff;
    bool b_swap = false;
    bool b_base = false;
    bool b_temp = false;
    while (t - buff < br) {
        if (strcmp(t, "PATH") == 0) {
            t = next_token(t);
            set_ctx_var(ctx, "PATH", t);
        } else if (strcmp(t, "TEMP") == 0) {
            t = next_token(t);
            set_ctx_var(ctx, "TEMP", t);
            f_mkdir(t);
            b_temp = true;
        } else if (strcmp(t, "COMSPEC") == 0) {
            t = next_token(t);
            comspec = (char*)pvPortMalloc(strlen(t) + 1);
            strcpy(comspec, t);
        } else if (strcmp(t, "SWAP") == 0) {
            t = next_token(t);
            init_vram(t);
            b_swap = true;
        } else if (strcmp(t, "GMODE") == 0) {
            t = next_token(t);
            graphics_mode_t mode = (graphics_mode_t)atoi(t);
            if (mode == TEXTMODE_DEFAULT || mode == TEXTMODE_53x30 || mode == TEXTMODE_160x100) {
                graphics_set_mode(mode);
            } else {
                goutf("Unsupported GMODE: %d\n", mode);
            }
        } else if (strcmp(t, "BASE") == 0) {
            t = next_token(t);
            set_ctx_var(ctx, "BASE", t);
            f_mkdir(t);
            b_base = true;
        } else {
            char* k = copy_str(t);
            t = next_token(t);
            set_ctx_var(ctx, k, t);
        }
        t = next_token(t);
    }
    if (!b_temp) f_mkdir("tmp");
    if (!b_base) f_mkdir("MOS");
    if (!b_swap) {
        char* t1 = "/tmp/pagefile.sys 1M 64K 4K";
        char* t2 = (char*)pvPortMalloc(strlen(t1) + 1);
        strcpy(t2, t1);
        init_vram(t2);
        vPortFree(t2);
    }
    ctx->curr_dir = copy_str(b_base ? get_ctx_var(ctx, "BASE") : "MOS");
    vPortFree(buff);
}

static inline void try_to_restore_api_tbl(cmd_ctx_t* ctx) {
    char* t = get_ctx_var(ctx, "TEMP");
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
            ctx->argv[0] = copy_str(comspec);
            if(ctx->orig_cmd) vPortFree(ctx->orig_cmd);
            ctx->orig_cmd = copy_str(comspec);
        }
        // goutf("[%s]Lookup for: %s\n", ctx->curr_dir, ctx->orig_cmd);
        // goutf("be [%p]\n", xPortGetFreeHeapSize());
        bool b_exists = exists(ctx);
        // goutf("ae [%p]\n", xPortGetFreeHeapSize());
        if (b_exists) {
            size_t len = strlen(ctx->argv[0]); // TODO: more than one?
            // goutf("[%s]Command found: %s)\n", ctx->curr_dir, t);
            if (len > 3 && strcmp(ctx->argv[0] + len - 4, ".uf2") == 0) {
                if(load_firmware(ctx->argv[0])) { // TODO: by ctx
                    ctx->stage = LOAD;
                    run_app(ctx->argv[0]);
                    ctx->stage = EXECUTED;
                }
            } else if(is_new_app(ctx)) {
                // goutf("[%s]Command has appropriate format\n", ctx->curr_dir);
                if (load_app(ctx)) {
                    exec(ctx);
                        // ctx->stage; // to be changed in exec to PREPAREAD or to EXCUTED
                        // goutf("EXEC RET_CODE: %d; tokens: %d [%p]:\n", ctx->ret_code, ctx->argc, ctx->argv);
                        /*if (ctx->argc && ctx->argv) {
                            for (int i = 0; i < ctx->argc; ++i) {
                                goutf(" argv[%d]='%s'\n", i, ctx->argv[i]);
                            }
                        }*/
                        //vTaskDelay(5000);
                }
            } else {
                goutf("Unable to execute command: '%s'\n", ctx->orig_cmd);
                ctx->stage = INVALIDATED;
                goto e;
            }
        } else {
            goutf("Illegal command: '%s'\n", ctx->orig_cmd);
            ctx->stage = INVALIDATED;
            goto e;
        }
        continue;
e:
        if (ctx->pboot_ctx) {
            cleanup_bootb_ctx(ctx->pboot_ctx);
            vPortFree(ctx->pboot_ctx);
            ctx->pboot_ctx = 0;
        }
        if (ctx->stage != PREPARED) { // it is expected cmd/cmd0 will prepare ctx for next run for application, in other case - cleanup ctx
            cleanup_ctx(ctx);
        }

    }
    vTaskDelete( NULL );
}

const char tmp[] = "                      ZX Murmulator (RP2040) OS v."
                    MOS_VERSION_STR
                   " Alpha                   ";

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

    SCREEN = (uint8_t*)pvPortMalloc(80 * 30 * 2);
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(30);

    gpio_put(PICO_DEFAULT_LED_PIN, true);
    bool mount_res = (FR_OK == f_mount(&fs, "SD", 1));

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
          "SWAP %d MB; BASE: %ph (%d KB); PAGES INDEX: %ph (%d x %d KB)\n"
          "\n",
          get_overclocking_khz() / 1000,
          ram32 >> 10,
          flash32 >> 20,
          psram32 >> 20,
          fs->n_fats,
          f_getfree32(fs),
          fs->csize >> 1,
          swap_size() >> 20, swap_base(), swap_base_size() >> 10, swap_pages_base(), swap_pages(), swap_page_size() >> 10
    );
}

int main() {
    init();
    info();

    xTaskCreate(vCmdTask, "cmd", 1024/*x4=4096k*/, NULL, configMAX_PRIORITIES - 1, NULL);
    /* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    while(sys_table_ptrs[0]); // to ensure linked (TODO: other way)

    __unreachable();
}
