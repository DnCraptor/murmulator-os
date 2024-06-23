#include "ram_page.h"
#include "f_util.h"
#include "ff.h"
#include <pico.h>
#include <pico/stdlib.h>
#include "graphics.h"

#define TOTAL_VIRTUAL_MEMORY_KBS (8*1024)
__aligned(4) uint8_t RAM[RAM_SIZE] = { 0 };
__aligned(4) uint16_t RAM_PAGES[RAM_BLOCKS] = { 0 };

uint32_t swap_base_size() { return RAM_SIZE; }
uint8_t* swap_base() { return RAM; }

static uint32_t get_ram_page_for(const uint32_t addr32);

uint8_t ram_page_read(uint32_t addr32) {
    const register uint32_t ram_page = get_ram_page_for(addr32);
    const register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
#ifdef BOOT_DEBUG_ACC
    uint8_t res = RAM[(ram_page * RAM_PAGE_SIZE) + addr_in_page];
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "R %08X ->   %02X", addr32, res); logMsg(tmp);
    }
    return res;
#else
    return RAM[(ram_page * RAM_PAGE_SIZE) + addr_in_page];
#endif
}

inline static uint16_t read16arr(uint8_t* arr, uint32_t base_addr, uint32_t addr32) {
    register uint8_t* ptr = arr + addr32 - base_addr;
    register uint16_t b1 = *ptr++;
    register uint16_t b0 = *ptr;
    return b1 | (b0 << 8);
}

uint16_t ram_page_read16(uint32_t addr32) {
    const register uint32_t ram_page = get_ram_page_for(addr32);
    const register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
#ifdef BOOT_DEBUG_ACC
    uint16_t res = read16arr(RAM, 0, (ram_page * RAM_PAGE_SIZE) + addr_in_page);
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "R %08X -> %04X", addr32, res); logMsg(tmp);
    }
    return res;
#else
    return read16arr(RAM, 0, (ram_page * RAM_PAGE_SIZE) + addr_in_page);
#endif
}

inline static uint32_t read32arr(uint8_t* arr, uint32_t base_addr, uint32_t addr32) {
    register uint8_t* ptr = arr + addr32 - base_addr;
    register uint32_t b3 = *ptr++;
    register uint32_t b2 = *ptr++;
    register uint32_t b1 = *ptr++;
    register uint32_t b0 = *ptr;
    return b3 | (b2 << 8) | (b1 << 16) | (b0 << 24);
}

uint32_t ram_page_read32(uint32_t addr32) {
    const register uint32_t ram_page = get_ram_page_for(addr32);
    const register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
#ifdef BOOT_DEBUG_ACC
    uint16_t res = read32arr(RAM, 0, (ram_page * RAM_PAGE_SIZE) + addr_in_page);
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "R %08X -> %08X", addr32, res); logMsg(tmp);
    }
    return res;
#else
    return read32arr(RAM, 0, (ram_page * RAM_PAGE_SIZE) + addr_in_page);
#endif
}

void ram_page_write(uint32_t addr32, uint8_t value) {
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "W %08X <-   %02X", addr32, value); logMsg(tmp);
    }
#endif
    register uint32_t ram_page = get_ram_page_for(addr32);
    register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
    RAM[(ram_page * RAM_PAGE_SIZE) + addr_in_page] = value;
    register uint16_t ram_page_desc = RAM_PAGES[ram_page];
    if (!(ram_page_desc & 0x8000)) {
        // if higest (15) bit is set, it means - the page has changes
        RAM_PAGES[ram_page] = ram_page_desc | 0x8000; // mark it as changed - bit 15
    }
}

void ram_page_write16(uint32_t addr32, uint16_t value) {
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "W %08X <- %04X", addr32, value); logMsg(tmp);
    }
#endif
    register uint32_t ram_page = get_ram_page_for(addr32);
    register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
    register uint8_t* addr_in_ram = RAM + ram_page * RAM_PAGE_SIZE + addr_in_page;
    *addr_in_ram++     = (uint8_t) value;
    *addr_in_ram       = (uint8_t)(value >> 8);
    register uint16_t ram_page_desc = RAM_PAGES[ram_page];
    if (!(ram_page_desc & 0x8000)) {
        // if higest (15) bit is set, it means - the page has changes
        RAM_PAGES[ram_page] = ram_page_desc | 0x8000; // mark it as changed - bit 15
    }
}

void ram_page_write32(uint32_t addr32, uint32_t value) {
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        sprintf(tmp, "Q %08X <- %08X", addr32, value); logMsg(tmp);
    }
#endif
    register uint32_t ram_page = get_ram_page_for(addr32);
    register uint32_t addr_in_page = addr32 & RAM_IN_PAGE_ADDR_MASK;
    register uint8_t* addr_in_ram = RAM + ram_page * RAM_PAGE_SIZE + addr_in_page;
    *addr_in_ram++     = (uint8_t) value;
    *addr_in_ram++     = (uint8_t)(value >> 8);
    *addr_in_ram++     = (uint8_t)(value >> 16);
    *addr_in_ram       = (uint8_t)(value >> 24);
    register uint16_t ram_page_desc = RAM_PAGES[ram_page];
    if (!(ram_page_desc & 0x8000)) {
        // if higest (15) bit is set, it means - the page has changes
        RAM_PAGES[ram_page] = ram_page_desc | 0x8000; // mark it as changed - bit 15
    }
}

static uint16_t oldest_ram_page = 1;
static uint16_t last_ram_page = 0;
static uint32_t last_lba_page = 0;

uint32_t get_ram_page_for(const uint32_t addr32) {
    const register uint32_t lba_page = addr32 / RAM_PAGE_SIZE; // 4KB page idx
    if (last_lba_page == lba_page) {
        return last_ram_page;
    }
    last_lba_page = lba_page;
    for (register uint32_t ram_page = 1; ram_page < RAM_BLOCKS; ++ram_page) {
        register uint16_t ram_page_desc = RAM_PAGES[ram_page];
        register uint16_t lba_page_in_ram = ram_page_desc & 0x7FFF; // 14-0 - max 32k keys for 4K LBA bloks
        if (lba_page_in_ram == lba_page) {
            last_ram_page = ram_page;
            return ram_page;
        }
    }
#ifdef BOOT_DEBUG_ACC
    sprintf(tmp, "VRAM page: 0x%X", lba_page); logMsg(tmp);
#endif
    // rolling page usage
    uint16_t ram_page = oldest_ram_page++;
    if (oldest_ram_page >= RAM_BLOCKS - 1) oldest_ram_page = 1; // do not use first page (id == 0)
    uint16_t ram_page_desc = RAM_PAGES[ram_page];
    bool ro_page_was_found = !(ram_page_desc & 0x8000);
    // higest (15) bit is set, it means - the page has changes (RW page)
    uint32_t old_lba_page = ram_page_desc & 0x7FFF; // 14-0 - max 32k keys for 4K LBA bloks
    RAM_PAGES[ram_page] = lba_page;
    if (ro_page_was_found) {
        // just replace RO page (faster than RW flush to sdcard)
#ifdef BOOT_DEBUG_ACC
        sprintf(tmp, "1 RAM page 0x%X / VRAM page: 0x%X", ram_page, lba_page); logMsg(tmp);
#endif
        uint32_t ram_page_offset = ((uint32_t)ram_page) * RAM_PAGE_SIZE;
        uint32_t lba_page_offset = lba_page * RAM_PAGE_SIZE;
        read_vram_block(RAM + ram_page_offset, lba_page_offset, RAM_PAGE_SIZE);
        last_ram_page = ram_page;
        return ram_page;
    }
    // Lets flush found RW page to sdcard
#ifdef BOOT_DEBUG_ACC
    sprintf(tmp, "2 RAM page 0x%X / VRAM page: 0x%X", ram_page, lba_page); logMsg(tmp);
#endif
    uint32_t ram_page_offset = ram_page * RAM_PAGE_SIZE;
    uint32_t lba_page_offset = old_lba_page * RAM_PAGE_SIZE;
#ifdef BOOT_DEBUG_ACC
    sprintf(tmp, "2 RAM offs 0x%X / VRAM offs: 0x%X", ram_page_offset, lba_page_offset); logMsg(tmp);
#endif
    flush_vram_block(RAM + ram_page_offset, lba_page_offset, RAM_PAGE_SIZE);
    // use new page:
    lba_page_offset = lba_page * RAM_PAGE_SIZE;
    read_vram_block(RAM + ram_page_offset, lba_page_offset, RAM_PAGE_SIZE);
    last_ram_page = ram_page;
    return ram_page;
}

static const char* path = "\\MOS\\pagefile.sys";
static FIL file;
static uint32_t _swap_size = 0;

uint32_t swap_size() {
    return _swap_size;
}

uint32_t init_vram() {
    FRESULT fresult = f_stat(path , &file);
    f_unlink(path); // ensure it is new file
    FRESULT result = f_open(&file, path, FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    if (result == FR_OK) {
        result = f_lseek(&file, TOTAL_VIRTUAL_MEMORY_KBS * 1024);
        if (result != FR_OK) {
            goutf("Unable to init <SD-card>\\MOS\\pagefile.sys\n");
            return 0;
        }
    } else {
        goutf("Unable to create <SD-card>\\MOS\\pagefile.sys\n");
        return 0;
    }
    f_close(&file);
    result = f_open(&file, path, FA_READ | FA_WRITE);
    if (result != FR_OK) {
        goutf("Unable to open <SD-card>\\MOS\\pagefile.sys\n");
    }
    _swap_size = TOTAL_VIRTUAL_MEMORY_KBS << 10;
    return _swap_size;
}

FRESULT vram_seek(FIL* fp, uint32_t file_offset) {
    FRESULT result = f_lseek(&file, file_offset);
    if (result != FR_OK) {
        result = f_open(&file, path, FA_READ | FA_WRITE);
        if (result != FR_OK) {
            goutf("Unable to open pagefile.sys: %s (%d)", FRESULT_str(result), result);
            return result;
        }
        goutf("Failed to f_lseek: %s (%d)", FRESULT_str(result), result);
    }
    return result;
}

void read_vram_block(char* dst, uint32_t file_offset, uint32_t sz) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
  //  if (file_offset >= 0x100000) {
  //      sprintf(tmp, "Read  pagefile 0x%X<-0x%X", dst, file_offset);
  //      logMsg(tmp);
  //  }
    FRESULT result = vram_seek(&file, file_offset);
    if (result != FR_OK) {
        return;
    }
    UINT br;
    result = f_read(&file, dst, sz, &br);
    if (result != FR_OK) {
        goutf("Failed to f_read: %s (%d)\n", FRESULT_str(result), result);
    }
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

void flush_vram_block(const char* src, uint32_t file_offset, uint32_t sz) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
   // if (file_offset >= 0x100000) {
   //     sprintf(tmp, "Flush pagefile 0x%X->0x%X", src, file_offset);
   //     logMsg(tmp);
   // }
    FRESULT result = vram_seek(&file, file_offset);
    if (result != FR_OK) {
        return;
    }
    UINT bw;
    result = f_write(&file, src, sz, &bw);
    if (result != FR_OK) {
        goutf("Failed to f_write: %s (%d)\n", FRESULT_str(result), result);
    }
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}
