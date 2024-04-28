#include <cstdlib>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

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


#define M_OS_APL_TABLE_BASE ((size_t*)0x10002000ul)
typedef int (*boota_ptr_t)( void );

inline static int run_application() {
    boota_ptr_t fn_ptr = (boota_ptr_t)(M_OS_APL_TABLE_BASE[0]);
    return fn_ptr();
}

}

#include "nespad.h"

static FATFS fs;
semaphore vga_start_semaphore;
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)

typedef struct {
    // 32 byte header
    uint32_t magicStart0;
    uint32_t magicStart1;
    uint32_t flags;
    uint32_t targetAddr;
    uint32_t payloadSize;
    uint32_t blockNo;
    uint32_t numBlocks;
    uint32_t fileSize; // or familyID;
    uint8_t data[476];
    uint32_t magicEnd;
} UF2_Block_t;


static uint8_t* SCREEN = 0;

static uint32_t input;

extern "C" {
bool __time_critical_func(handleScancode)(const uint32_t ps2scancode) {
    if (ps2scancode)
        input = ps2scancode;

    return true;
}
}

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

/*
void __always_inline run_application() {
    multicore_reset_core1();
    asm volatile (
        "mov r0, %[start]\n"
        "ldr r1, =%[vtable]\n"
        "str r0, [r1]\n"
        "ldmia r0, {r0, r1}\n"
        "msr msp, r0\n"
        "bx r1\n"
        :: [start] "r" (0x10002000), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
    );
    __unreachable();
}
*/

bool __not_in_flash_func(load_firmware)(const char pathname[256]) {
    UINT bytes_read = 0;
    FIL file;
    char tmp[80] = {0};

    constexpr int window_y = (TEXTMODE_ROWS - 5) / 2;
    constexpr int window_x = (TEXTMODE_COLS - 43) / 2;

    draw_window(" Loading application", window_x, window_y, 43, 5);
/*
    FILINFO fileinfo;
    f_stat(pathname, &fileinfo);

    if (FLASH_SIZE - 64 << 10 < fileinfo.fsize / 2) {
        draw_text(" ERROR: Firmware too large! Canceled!!", window_x + 1, window_y + 2, 13, 1);
        sleep_ms(5000);
        return false;
    }
*/
    draw_text(pathname, window_x + 2, window_y + 2, 10, 1);
   // sleep_ms(500);

    if (FR_OK != f_open(&file, pathname, FA_READ)) {
        draw_text(" ERROR: Unable to load file!!", window_x + 1, window_y + 2, 10, 1);
        return false;
    }

    uint8_t* buffer = (uint8_t*)pvPortMalloc(FLASH_SECTOR_SIZE);
    UF2_Block_t* uf2 = (UF2_Block_t*)pvPortMalloc(sizeof(UF2_Block_t));

    uint32_t flash_target_offset = 0x2000;
    uint32_t data_sector_index = 0;

    multicore_lockout_start_blocking();
    const uint32_t ints = save_and_disable_interrupts();
    int i = 0;
    do {
        f_read(&file, uf2, sizeof(UF2_Block_t), &bytes_read); // err?
       // snprintf(tmp, 80, "#%d (%d) %ph", uf2->blockNo, uf2->payloadSize, uf2->targetAddr);
       // draw_text(tmp, 0, uf2->blockNo % 30, 7, 0);
        memcpy(buffer + data_sector_index, uf2->data, uf2->payloadSize);
        data_sector_index += uf2->payloadSize;
        if (flash_target_offset == 0)
            flash_target_offset = uf2->targetAddr - XIP_BASE;
            // TODO: order?

        if (data_sector_index == FLASH_SECTOR_SIZE || bytes_read == 0) {
            data_sector_index = 0;
            //подмена загрузчика boot2 прошивки на записанный ранее
        //    if (flash_target_offset == 0) {
        //        memcpy(buffer, (uint8_t *)XIP_BASE, 256);
        //    }
            flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE);
            flash_range_program(flash_target_offset, buffer, FLASH_SECTOR_SIZE);

        //    gpio_put(PICO_DEFAULT_LED_PIN, flash_target_offset & 1);
            //snprintf(tmp, 80, "#%d %ph -> %ph", uf2->blockNo, uf2->targetAddr, flash_target_offset);
            //draw_text(tmp, 0, i++, 7, 0);
        
            flash_target_offset = 0;
        }
    }
    while (bytes_read != 0);

    restore_interrupts(ints);
    multicore_lockout_end_blocking();

    gpio_put(PICO_DEFAULT_LED_PIN, false);
    f_close(&file);
    vPortFree(buffer);
    vPortFree(uf2);
    return true;
}

typedef struct __attribute__((__packed__)) {
    bool is_directory;
    bool is_executable;
    size_t size;
    char filename[79];
} file_item_t;

constexpr int max_files = 2500;
static file_item_t fileItems[max_files];

int compareFileItems(const void* a, const void* b) {
    const auto* itemA = (file_item_t *)a;
    const auto* itemB = (file_item_t *)b;
    // Directories come first
    if (itemA->is_directory && !itemB->is_directory)
        return -1;
    if (!itemA->is_directory && itemB->is_directory)
        return 1;
    // Sort files alphabetically
    return strcmp(itemA->filename, itemB->filename);
}

static inline bool isExecutable(const char pathname[256], const char* extensions) {
    const char* extension = strrchr(pathname, '.');
    if (extension == nullptr) {
        return false;
    }
    extension++; // Move past the '.' character

    const char* token = strtok((char *)extensions, "|"); // Tokenize the extensions string using '|'

    while (token != nullptr) {
        if (strcmp(extension, token) == 0) {
            return true;
        }
        token = strtok(NULL, "|");
    }

    return false;
}
#if 0
void __not_in_flash_func(filebrowser)(const char pathname[256], const char* executables) {
    bool debounce = true;
    char basepath[256];
    char tmp[TEXTMODE_COLS + 1];
    strcpy(basepath, pathname);
    constexpr int per_page = TEXTMODE_ROWS - 3;

    DIR dir;
    FILINFO fileInfo;

    if (FR_OK != f_mount(&fs, "SD", 1)) {
        draw_text("SD Card not inserted or SD Card error!", 0, 0, 12, 0);
        while (true);
    }

    while (true) {
        memset(fileItems, 0, sizeof(file_item_t) * max_files);
        int total_files = 0;

        snprintf(tmp, TEXTMODE_COLS, "SD:\\%s", basepath);
        draw_window(tmp, 0, 0, TEXTMODE_COLS, TEXTMODE_ROWS - 1);
        memset(tmp, ' ', TEXTMODE_COLS);

#ifndef TFT
        draw_text(tmp, 0, 29, 0, 0);
        auto off = 0;
        draw_text("START", off, 29, 7, 0);
        off += 5;
        draw_text(" Run at cursor ", off, 29, 0, 3);
        off += 16;
        draw_text("SELECT", off, 29, 7, 0);
        off += 6;
        draw_text(" Run previous  ", off, 29, 0, 3);
        off += 16;
        draw_text("ARROWS", off, 29, 7, 0);
        off += 6;
        draw_text(" Navigation    ", off, 29, 0, 3);
        off += 16;
        draw_text("A/F10", off, 29, 7, 0);
        off += 5;
        draw_text(" USB DRV ", off, 29, 0, 3);
#endif

        if (FR_OK != f_opendir(&dir, basepath)) {
            draw_text("Failed to open directory", 1, 1, 4, 0);
            while (true);
        }

        if (strlen(basepath) > 0) {
            strcpy(fileItems[total_files].filename, "..\0");
            fileItems[total_files].is_directory = true;
            fileItems[total_files].size = 0;
            total_files++;
        }

        while (f_readdir(&dir, &fileInfo) == FR_OK &&
               fileInfo.fname[0] != '\0' &&
               total_files < max_files
        ) {
            // Set the file item properties
            fileItems[total_files].is_directory = fileInfo.fattrib & AM_DIR;
            fileItems[total_files].size = fileInfo.fsize;
            fileItems[total_files].is_executable = isExecutable(fileInfo.fname, executables);
            strncpy(fileItems[total_files].filename, fileInfo.fname, 78);
            total_files++;
        }
        f_closedir(&dir);

        qsort(fileItems, total_files, sizeof(file_item_t), compareFileItems);

        if (total_files > max_files) {
            draw_text(" Too many files!! ", TEXTMODE_COLS - 17, 0, 12, 3);
        }

        int offset = 0;
        int current_item = 0;

        while (true) {
            sleep_ms(100);

            if (!debounce) {
                debounce = !(nespad_state & DPAD_START) && input != 0x1C;
            }

            // ESCAPE
            if (nespad_state & DPAD_B || input == 0x01) {
                return;
            }

            // F10
            if (nespad_state & DPAD_A || input == 0x44) {
                constexpr int window_x = (TEXTMODE_COLS - 40) / 2;
                constexpr int window_y = (TEXTMODE_ROWS - 4) / 2;
                draw_window("SD Cardreader mode ", window_x, window_y, 40, 4);
                draw_text("Mounting SD Card. Use safe eject ", window_x + 1, window_y + 1, 13, 1);
                draw_text("to conitinue...", window_x + 1, window_y + 2, 13, 1);

                sleep_ms(500);

                init_pico_usb_drive();

                while (!tud_msc_ejected()) {
                    pico_usb_drive_heartbeat();
                }

                int post_cicles = 1000;
                while (--post_cicles) {
                    sleep_ms(1);
                    pico_usb_drive_heartbeat();
                }
            }

            if (nespad_state & DPAD_DOWN || input == 0x50) {
                if (offset + (current_item + 1) < total_files) {
                    if (current_item + 1 < per_page) {
                        current_item++;
                    }
                    else {
                        offset++;
                    }
                }
            }

            if (nespad_state & DPAD_UP || input == 0x48) {
                if (current_item > 0) {
                    current_item--;
                }
                else if (offset > 0) {
                    offset--;
                }
            }

            if (nespad_state & DPAD_RIGHT || input == 0x4D || input == 0x7A) {
                offset += per_page;
                if (offset + (current_item + 1) > total_files) {
                    offset = total_files - (current_item + 1);
                }
            }

            if (nespad_state & DPAD_LEFT || input == 0x4B) {
                if (offset > per_page) {
                    offset -= per_page;
                }
                else {
                    offset = 0;
                    current_item = 0;
                }
            }

            if (debounce && ((nespad_state & DPAD_START) != 0 || input == 0x1C)) {
                auto file_at_cursor = fileItems[offset + current_item];

                if (file_at_cursor.is_directory) {
                    if (strcmp(file_at_cursor.filename, "..") == 0) {
                        const char* lastBackslash = strrchr(basepath, '\\');
                        if (lastBackslash != nullptr) {
                            const size_t length = lastBackslash - basepath;
                            basepath[length] = '\0';
                        }
                    }
                    else {
                        sprintf(basepath, "%s\\%s", basepath, file_at_cursor.filename);
                    }
                    debounce = false;
                    break;
                }

                if (file_at_cursor.is_executable) {
                    sprintf(tmp, "%s\\%s", basepath, file_at_cursor.filename);

                    if (load_firmware(tmp)) {
                        return;
                    }
                }
            }

            for (int i = 0; i < per_page; i++) {
                const auto item = fileItems[offset + i];
                uint8_t color = 11;
                uint8_t bg_color = 1;

                if (i == current_item) {
                    color = 0;
                    bg_color = 3;
                    memset(tmp, 0xCD, TEXTMODE_COLS - 2);
                    tmp[TEXTMODE_COLS - 2] = '\0';
                    draw_text(tmp, 1, per_page + 1, 11, 1);
                    snprintf(tmp, TEXTMODE_COLS - 2, " Size: %iKb, File %lu of %i ", item.size / 1024, offset + i + 1,
                             total_files);
                    draw_text(tmp, 2, per_page + 1, 14, 3);
                }

                const auto len = strlen(item.filename);
                color = item.is_directory ? 15 : color;
                color = item.is_executable ? 10 : color;
                //color = strstr((char *)rom_filename, item.filename) != nullptr ? 13 : color;
                memset(tmp, ' ', TEXTMODE_COLS - 2);
                tmp[TEXTMODE_COLS - 2] = '\0';
                memcpy(&tmp, item.filename, len < TEXTMODE_COLS - 2 ? len : TEXTMODE_COLS - 2);
                draw_text(tmp, 1, i + 1, color, bg_color);
            }
        }
    }
}
#endif

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(252 * KHZ, true);

    keyboard_init();
    keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    SCREEN = (uint8_t*)pvPortMalloc(80 * 30 * 2);
#if 0
    for (int i = 20; i--;) {
        nespad_read();
        sleep_ms(50);

        // F12 Boot to USB FIRMWARE UPDATE mode
        if (nespad_state & DPAD_START || input == 0x58) {
            reset_usb_boot(0, 0);
        }

        // F11 Run launcher
        if (nespad_state && !(nespad_state & DPAD_START) || input && input != 0x58) {
#endif
            sem_init(&vga_start_semaphore, 0, 1);
            multicore_launch_core1(render_core);
            sem_release(&vga_start_semaphore);

            sleep_ms(30);
    clrScr(1);
    char tmp[80];
    snprintf(tmp, 80, "sys_table_ptrs: %ph; snprintf: %ph", &sys_table_ptrs[0], sys_table_ptrs[29]);
    draw_text(tmp, 0, 0, 13, 1);
#if 0
            filebrowser("", "uf2");
        }
    }

#endif
    if (FR_OK != f_mount(&fs, "SD", 1)) {
        draw_text("SD Card not inserted or SD Card error!", 0, 0, 12, 0);
        while (true);
    }
#if TESTS
    start_test();
#endif
    snprintf(tmp, 80, "application @ %ph: %08Xh", M_OS_APL_TABLE_BASE, *(uint32_t*)M_OS_APL_TABLE_BASE);
    draw_text(tmp, 0, 1, 13, 2);
    int res = -1;
    if (0 != *((uint32_t*)M_OS_APL_TABLE_BASE)) {
        // boota (startup application) already in flash ROM
        res = run_application();
    }
    else if (load_firmware("\\MOS\\murmulator-os-demo.uf2")) {
        res = run_application();
    } else {
        // ??
    }
    snprintf(tmp, 80, "application returns #%d", res);
    draw_text(tmp, 0, 2, 7, 0);
    draw_text("RUNING   vTaskStartScheduler    ", 0, 3, 7, 0);
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
