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

static cmd_startup_ctx_t ctx = { 0 };

extern "C" {
cmd_startup_ctx_t * get_cmd_startup_ctx() {
    return &ctx;
}
FIL * get_stdout() {
    return ctx.pstdout;
}
FIL * get_stderr() {
    return ctx.pstderr;
}
char* get_curr_dir() {
    return ctx.curr_dir;
}
char* next_token(char* t) {
    char *t1 = t + strlen(t);
    while(!*t1++);
    return t1 - 1;
}
}

inline static void tokenizeCfg(char* s, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        if (s[i] == '=' || s[i] == '\n') {
            s[i] = 0;
        }
    }
}

inline static void tokenizePath(char* s) {
    for (size_t i = 0; i < strlen(s); ++i) {
        if (s[i] == ';' || s[i] == ',' || s[i] == ':') {
            s[i] = 0;
        }
    }
}

static char* comspec;

static void load_config_sys() {
    ctx.cmd = (char*)pvPortMalloc(512); ctx.cmd[0] = 0;
    ctx.cmd_t = (char*)pvPortMalloc(512); ctx.cmd_t[0] = 0;
    ctx.curr_dir = (char*)pvPortMalloc(512);
    ctx.pstdout = (FIL*)pvPortMalloc(sizeof(FIL)); memset(ctx.pstdout, 0, 512);
    ctx.pstderr = (FIL*)pvPortMalloc(sizeof(FIL)); memset(ctx.pstderr, 0, 512);
    ctx.path = (char*)pvPortMalloc(512);
    strcpy(ctx.curr_dir, "MOS");
    strcpy(ctx.path, "MOS");
    comspec = "/mos/cmd";
    FIL f;
    if(f_open(&f, "/config.sys", FA_READ) != FR_OK) {
        if(f_open(&f, "/mos/config.sys", FA_READ) != FR_OK) {
            return;
        }
    }
    char buff[512];
    UINT br;
    if (f_read(&f, buff, 512, &br) != FR_OK) {
        goutf("Failed to read config.sys\n");
    }
    f_close(&f);
    tokenizeCfg(buff, br);
    char *t = buff;
    while (t - buff < br) {
        if (strcmp(t, "PATH") == 0) {
            t = next_token(t);
            strcpy(ctx.path, t);
            tokenizePath(ctx.path);
        } else if (strcmp(t, "COMSPEC") == 0) {
            t = next_token(t);
            comspec = (char*)pvPortMalloc(strlen(t));
            strcpy(comspec, t);
        } else if (strcmp(t, "SWAP") == 0) {
            t = next_token(t);
            // TODO:
        } else if (strcmp(t, "GMODE") == 0) {
            t = next_token(t);
            // TODO:
        } else if (strcmp(t, "BASE") == 0) {
            t = next_token(t);
            strcpy(ctx.curr_dir, t);
        }
        t = next_token(t);
    }
}

static bootb_ctx_t bootb_ctx = { 0 };

extern "C" void vCmdTask(void *pv) {
    exec(&bootb_ctx);
    vTaskDelete( NULL );
}

const char tmp[] = "                      ZX Murmulator (RP2040) OS v.0.0.3 Alfa                    ";

int main() {
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
    // F12 Boot to USB FIRMWARE UPDATE mode
    if (nespad_state & DPAD_START || get_last_scancode() == 0x58) {
        reset_usb_boot(0, 0);
    }

    SCREEN = (uint8_t*)pvPortMalloc(80 * 30 * 2);
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(30);

    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
   
    if (FR_OK != f_mount(&fs, "SD", 1)) {
        graphics_set_con_color(12, 0);
        goutf("SD Card not inserted or SD Card error!");
        while (true);
    }
    load_config_sys();

    f_mkdir(get_curr_dir());
    init_psram();
    init_vram();

    draw_text(tmp, 0, 0, 13, 1);
    draw_text(tmp, 0, 29, 13, 1);
    graphics_set_con_pos(0, 1);
    graphics_set_con_color(7, 0);
    char* curr_dir = get_curr_dir();
    uint32_t ram32 = get_cpu_ram_size();
    uint32_t flash32 = get_cpu_flash_size();
    uint32_t psram32 = psram_size();
    uint32_t swap = swap_size();
    FATFS* fs = get_mount_fs();
    goutf("CPU %d MHz\n"
          "SRAM %d KB\n"
          "FLASH %d MB\n"
          "PSRAM %d MB\n"
          "SDCARD %d FATs; %d free clusters; cluster size: %d KB\n"
          "SWAP %d MB\n"
          "\n",
          get_overclocking_khz() / 1000,
          ram32 >> 10,
          flash32 >> 20,
          psram32 >> 20,
          fs->n_fats,
          f_getfree32(fs),
          fs->csize >> 1,
          swap >> 20
    );

    int res = load_app(comspec, "main", &bootb_ctx);
    if (res < 0) {
        goutf("Failed on load COMSPEC=%s for execution\n", comspec);
        cleanup_bootb_ctx(&bootb_ctx);
    } else {
        exec(&bootb_ctx);
     //  xTaskCreate(vCmdTask, "cmd", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
 	}
    /* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    size_t i = 0;
    while(sys_table_ptrs[++i]); // to ensure linked (TODO: other way)

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );

    __unreachable();
}
