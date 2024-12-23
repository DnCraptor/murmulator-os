#include <cstdlib>
#include <cstring>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <hardware/watchdog.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "psram_spi.h"

#include "graphics.h"

extern "C" {
#include "ps2.h"
#include "ff.h"
#include "usb.h"
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
#include "sound.h"
}

#include "nespad.h"

extern "C" uint32_t flash_size;;

static FATFS fs;
extern "C" FATFS* get_mount_fs() { // only one FS is supported for now
    return &fs;
}
semaphore vga_start_semaphore;
static int drv = DEFAULT_VIDEO_DRIVER;
extern "C" volatile bool reboot_is_requested;

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init(drv);
    // graphics_driver_t* gd = get_graphics_driver();
    // install_graphics_driver(gd);
    sem_acquire_blocking(&vga_start_semaphore);
    uint64_t t1 = time_us_64();
    while(!reboot_is_requested) {
        pcm_call();
#ifdef TFT
        uint64_t t2 = time_us_64();
        if (t2 - t1 > 20000ull) {
            refresh_lcd();
            t1 = t2;
        }
#endif
        tight_loop_contents();
    }
    watchdog_enable(10, true);
    while(true) sleep_ms(10) ;
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

static const char SD[] = "SD";
static const char CD[] = "CD"; // current directory 
static const char BASE[] = "BASE"; 
static const char MOS[] = "MOS"; 
static const char TEMP[] = "TEMP"; 
static const char PATH[] = "PATH"; 
static const char SWAP[] = "SWAP"; 
static const char COMSPEC[] = "COMSPEC"; 
static const char ctmp[] = "/tmp"; 
static const char ccmd[] = "/mos/cmd";

static void __always_inline check_firmware() {
    FIL f;
    if(f_open(&f, FIRMWARE_MARKER_FN, FA_READ) == FR_OK) {
        f_close(&f);
        f_unmount(SD);
        run_application();
    }
}

inline static void unlink_firmware() {
    f_unlink(FIRMWARE_MARKER_FN);
}

static cmd_ctx_t* set_default_vars() {
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    set_ctx_var(ctx, CD, MOS);
    set_ctx_var(ctx, BASE, MOS);
    set_ctx_var(ctx, TEMP, ctmp);
    set_ctx_var(ctx, COMSPEC, ccmd);
    return ctx;
}

static char* open_config(UINT* pbr) {
    FILINFO* pfileinfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    size_t file_size = 0;
    char * cfn = "/config.sys";
    if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
        cfn = "/mos/config.sys";
        if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
            vPortFree(pfileinfo);
            return 0;
        } else {
            file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
        }
    } else {
        file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
    }
    vPortFree(pfileinfo);

    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if(f_open(pf, cfn, FA_READ) != FR_OK) {
        vPortFree(pf);
        return 0;
    }
    char* buff = (char*)pvPortCalloc(file_size + 1, 1);
    if (f_read(pf, buff, file_size, pbr) != FR_OK) {
        goutf("Failed to read config.sys\n");
        vPortFree(buff);
        buff = 0;
    }
    f_close(pf);
    vPortFree(pf);
    return buff;
}

static void load_config_sys() {
    cmd_ctx_t* ctx = set_default_vars();
    bool b_swap = false;
    bool b_base = false;
    bool b_temp = false;
    UINT br;
    char* buff = open_config(&br);
    if (buff) {
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
            } else if (strcmp(t, "GMODE") == 0) { // TODO: FONT
                t = next_token(t);
                int mode = atoi(t);
                graphics_set_mode(mode);
            } else if (strcmp(t, "DRIVER") == 0) {
                t = next_token(t);
                // TODO:
            } else if (strcmp(t, "CPU") == 0) {
                t = next_token(t);
                int cpu = atoi(t);
                if (cpu > 123 && cpu < 450) {
                    set_last_overclocking(cpu * 1000);
                }
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
    if (buff) vPortFree(buff);
    if (!b_temp) f_mkdir(ctmp);
    if (!b_base) f_mkdir(MOS);
    if (!b_swap) {
        char* t1 = "/tmp/pagefile.sys 1M 64K 4K";
        char* t2 = (char*)pvPortMalloc(strlen(t1) + 1);
        strcpy(t2, t1);
        init_vram(t2);
        vPortFree(t2);
    }
    uint32_t overclocking = get_overclocking_khz();
    set_sys_clock_khz(overclocking, true);
    set_last_overclocking(overclocking);
}

const char* tmp = "Murmulator (RP2040) OS v." MOS_VERSION_STR;

#ifdef DEBUG_VGA
extern "C" char vga_dbg_msg[1024];
#endif

extern "C" void show_logo(bool with_top) {
    uint32_t w = get_console_width();
    uint32_t y = get_console_height() - 1;
    uint32_t sz = strlen(tmp);
    uint32_t sps = (w - sz) / 2;

    for(uint32_t x = 0; x < w; ++x) {
        if(with_top) draw_text(" ", x, 0, 13, 1);
        draw_text(" ", x, y, 13, 1);
    }
    if(with_top) draw_text(tmp, sps, 0, 13, 1);
    draw_text(tmp, sps, y, 13, 1);
    graphics_set_con_color(7, 0); // TODO: config
}

static void startup_vga(void) {
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(300);
    clrScr(0);
}

static void info(bool with_sd) {
    uint32_t ram32 = 264 << 10;// get_cpu_ram_size();
    uint8_t rx[4];
    get_cpu_flash_jedec_id(rx);
    flash_size = (1 << rx[3]);
    goutf("CPU %d MHz\n"
          "SRAM %d KB\n"
          "FLASH %d MB; JEDEC ID: %02X-%02X-%02X-%02X\n",
          get_overclocking_khz() / 1000,
          ram32 >> 10,
          flash_size >> 20, rx[0], rx[1], rx[2], rx[3]
    );
    uint32_t psram32 = psram_size();
    uint8_t rx8[8];
    psram_id(rx8);
    if (psram32) {
        goutf("PSRAM %d MB; MF ID: %02x; KGD: %02x; EID: %02X%02X-%02X%02X-%02X%02X\n",
              psram32 >> 20, rx8[0], rx8[1], rx8[2], rx8[3], rx8[4], rx8[5], rx8[6], rx8[7]
        );
    }
    if (!with_sd) {
        goutf("\n");
        return;
    }
    FATFS* fs = get_mount_fs();
    goutf("SDCARD %d FATs; %d free clusters (%d KB each)\n",
          fs->n_fats, f_getfree32(fs), fs->csize >> 1
    );
    goutf("SWAP %d MB; RAM: %d KB; pages index: %d x %d KB\n",
          swap_size() >> 20, swap_base_size() >> 10, swap_pages(), swap_page_size() >> 10
    );
    goutf("VRAM %d KB; video mode: %d x %d x %d bit\n"
          "\n",
          get_buffer_size() >> 10, get_screen_width(), get_screen_height(), get_console_bitness()
    );
}

void usb_on_boot() {
    usb_driver(true);
   	vTaskStartScheduler();
    for(;;) { vTaskDelay(10); }
}

void caseF10(void) {
            if (FR_OK == f_mount(&fs, SD, 1)) {
                FIL f;
                link_firmware(&f, "unknown");
            }
            watchdog_enable(1, false);
            while(1);
}

void caseF12(void) {
            if (FR_OK == f_mount(&fs, SD, 1)) {
                unlink_firmware();
            }
            reset_usb_boot(0, 0);
            while(1);
}

#ifndef TFT
void selectDRV1(void) {
            switch(DEFAULT_VIDEO_DRIVER) {
                case VGA_DRV:
                    #ifdef HDMI
                        drv = HDMI_DRV;
                    #else
                    #ifdef TV
                        drv = RGB_DRV;
                    #endif
                    #endif
                    break;
                #ifdef HDMI
                case HDMI_DRV:
                    drv = VGA_DRV;
                    break;
                #endif
                #ifdef TV
                case RGB_DRV:
                    drv = VGA_DRV;
                    break;
                #endif
                #ifdef SOFTTV
                case SOFTTV_DRV:
                    drv = VGA_DRV;
                    break;
                #endif
            }
}

void selectDRV2(void) {
            switch(DEFAULT_VIDEO_DRIVER) {
                case VGA_DRV:
                    #ifdef HDMI
                        drv = HDMI_DRV;
                    #else
                        #ifdef TV
                            drv = RGB_DRV;
                        #endif
                    #endif
                    break;
                #ifdef HDMI
                case HDMI_DRV:
                    drv = VGA_DRV;
                    break;
                #endif
                #ifdef TV
                case RGB_DRV:
                    #ifdef HDMI
                        drv = HDMI_DRV;
                    #else
                        drv = VGA_DRV;
                    #endif
                    break;
                #endif
                #ifdef SOFTTV
                case SOFTTV_DRV:
                    drv = HDMI_DRV;
                    break;
                #endif
            }
}
#endif

static kbd_state_t* process_input_on_boot() {
    kbd_state_t* ks = get_kbd_state();
    for (int a = 0; a < 20; ++a) {
        uint8_t sc = ks->input & 0xFF;
        if ( sc == 1 /* Esc */) {
            break;
        }
        if ( (nespad_state & DPAD_START) && (nespad_state & DPAD_SELECT) || (sc ==0x44) /*F10*/ ) {
            caseF10();
        }
        // F12 or ENTER or START Boot to USB FIRMWARE UPDATE mode
        if ((nespad_state & DPAD_START) || (sc == 0x58) /*F12*/ || (sc == 0x1C) /*ENTER*/) {
            caseF12();
        }
        // F11 or SPACE or SELECT unlink prev uf2 firmware
        if ((nespad_state & DPAD_SELECT) || (sc == 0x57) /*F11*/  || (sc == 0x39) /*SPACE*/) {
            if (FR_OK == f_mount(&fs, SD, 1)) {
                if (nespad_state & DPAD_B) {
                    usb_on_boot();
                }
                unlink_firmware(); // return to M-OS
                break;
            }
        } else if (sc == 0x47 /*HOME*/ && FR_OK == f_mount(&fs, SD, 1)) {
            usb_on_boot();
        }
        #ifndef TFT
        // DPAD A/TAB start with HDMI, if default is VGA, and vice versa
        if ((nespad_state & DPAD_A) || (sc == 0x0F) /*TAB*/) {
            selectDRV1();
            break;
        }
        // DPAD B start with VGA, if default is TV
        if ((nespad_state & DPAD_B)) {
            selectDRV2();
            break;
        }
        #endif
        sleep_ms(30);
        nespad_read();
    }
    return ks;
}

static void init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);

    uint32_t overclocking = get_overclocking_khz();
    set_sys_clock_khz(overclocking, true);
    set_last_overclocking(overclocking);

    keyboard_init();
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
    nespad_read();
    sleep_ms(50);

    gpio_put(PICO_DEFAULT_LED_PIN, true);

#ifdef DEBUG_VGA
    FIL f;
    f_open(&f, "mos-vga.log", FA_OPEN_APPEND | FA_WRITE);
    UINT wr;
    f_write(&f, vga_dbg_msg, strlen(vga_dbg_msg), &wr);
    f_close(&f);
#endif
    kbd_state_t* ks = process_input_on_boot();
    // send kbd reset only after initial process passed
    keyboard_send(0xFF);

    if (FR_OK == f_mount(&fs, SD, 1)) {
        check_firmware();
    } else {
        startup_vga();
        graphics_set_mode(graphics_get_default_mode());
        graphics_set_con_pos(0, 1);
        show_logo(true);
        init_psram();
        info(false);
        graphics_set_con_color(12, 0);
        gouta("SD Card not inserted or SD Card error!\nPls. insert it and reboot...\n");
        while (true) {
            nespad_read();
            int y = graphics_con_y();
            goutf("Scancodes tester: %Xh   \n", ks->input);
            goutf("Joysticks' states: %02Xh %02Xh\n", nespad_state, nespad_state2);
            sleep_ms(50);
            graphics_set_con_pos(0, y);
        }
    }

    startup_vga();
    graphics_set_mode(graphics_get_default_mode());
    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
   
    load_config_sys();
    init_psram();
    show_logo(true);
    graphics_set_con_pos(0, 1);
    init_sound();
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

int main() {
    init();
    info(true);

    xTaskCreate(vCmdTask, "cmd", 1024/*x4=4096*/, NULL, configMAX_PRIORITIES - 1, NULL);

    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    setApplicationStackOverflowHookPtr(overflowHook);

    /* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    while(sys_table_ptrs[0]); // to ensure linked (TODO: other way)

    __unreachable();
}
