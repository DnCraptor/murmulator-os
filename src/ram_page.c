#include "ram_page.h"
#include "f_util.h"
#include "ff.h"
#include <pico.h>
#include <pico/stdlib.h>
#include "graphics.h"
#include <string.h>

static FIL file;
static uint32_t _swap_size = 0;
static uint32_t _swap_base_size = 0;
static uint32_t _swap_page_size = 0;
static uint32_t _swap_page_mask = 0;
static uint32_t _swap_pages = 0;
static uint8_t _swap_page_div = 0;
static char* path = 0;
static char* _swap_base = 0;
static char* _swap_pages_base = 0;

static uint8_t* RAM = 0;
static uint16_t* RAM_PAGES = 0;

static uint16_t oldest_ram_page = 1;
static uint16_t last_ram_page = 0;
static uint32_t last_lba_page = 0;

uint32_t swap_base_size() { return _swap_base_size; }
uint32_t swap_page_size() { return _swap_page_size; }
uint8_t* swap_base() { return RAM; }
uint32_t swap_pages() { return _swap_pages; }
uint16_t* swap_pages_base() { return RAM_PAGES; }

static uint32_t get_ram_page_for(const uint32_t addr32);

inline static bool swap_range_valid(uint32_t addr32, uint32_t size) {
    return RAM && size <= _swap_size && addr32 <= _swap_size - size;
}

inline static void mark_ram_page_dirty(uint32_t ram_page) {
    register uint16_t ram_page_desc = RAM_PAGES[ram_page];
    if (!(ram_page_desc & 0x8000)) {
        // if highest (15) bit is set, it means - the page has changes
        RAM_PAGES[ram_page] = ram_page_desc | 0x8000;
    }
}

uint8_t ram_page_read(uint32_t addr32) {
    if (!swap_range_valid(addr32, 1)) return 0;
    const register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return 0;
    const register uint32_t addr_in_page = addr32 & _swap_page_mask;
#ifdef BOOT_DEBUG_ACC
    uint8_t res = RAM[(ram_page * _swap_page_size) + addr_in_page];
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("R %08X ->   %02X\n", addr32, res);
    }
    return res;
#else
    return RAM[(ram_page * _swap_page_size) + addr_in_page];
#endif
}

inline static uint16_t read16arr(uint8_t* arr, uint32_t base_addr, uint32_t addr32) {
    register uint8_t* ptr = arr + addr32 - base_addr;
    register uint16_t b1 = *ptr++;
    register uint16_t b0 = *ptr;
    return b1 | (b0 << 8);
}

uint16_t ram_page_read16(uint32_t addr32) {
    if (!swap_range_valid(addr32, 2)) return 0;
    const register uint32_t addr_in_page = addr32 & _swap_page_mask;
    if (addr_in_page == _swap_page_mask) {
        uint16_t result = 0;
        for (uint32_t i = 0; i < 2; ++i) {
            result |= (uint16_t)ram_page_read(addr32 + i) << (i << 3);
        }
        return result;
    }
    const register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return 0;
#ifdef BOOT_DEBUG_ACC
    uint16_t res = read16arr(RAM, 0, (ram_page * _swap_page_size) + addr_in_page);
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("R %08X -> %04X\n", addr32, res);
    }
    return res;
#else
    return read16arr(RAM, 0, (ram_page * _swap_page_size) + addr_in_page);
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
    if (!swap_range_valid(addr32, 4)) return 0;
    const register uint32_t addr_in_page = addr32 & _swap_page_mask;
    if (addr_in_page > _swap_page_size - 4) {
        uint32_t result = 0;
        for (uint32_t i = 0; i < 4; ++i) {
            result |= (uint32_t)ram_page_read(addr32 + i) << (i << 3);
        }
        return result;
    }
    const register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return 0;
#ifdef BOOT_DEBUG_ACC
    uint32_t res = read32arr(RAM, 0, (ram_page * _swap_page_size) + addr_in_page);
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("R %08X -> %08X\n", addr32, res);
    }
    return res;
#else
    return read32arr(RAM, 0, (ram_page * _swap_page_size) + addr_in_page);
#endif
}

void ram_page_write(uint32_t addr32, uint8_t value) {
    if (!swap_range_valid(addr32, 1)) return;
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("W %08X <-   %02X\n", addr32, value);
    }
#endif
    register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return;
    register uint32_t addr_in_page = addr32 & _swap_page_mask;
    RAM[(ram_page * _swap_page_size) + addr_in_page] = value;
    mark_ram_page_dirty(ram_page);
}

void ram_page_write16(uint32_t addr32, uint16_t value) {
    if (!swap_range_valid(addr32, 2)) return;
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("W %08X <- %04X\n", addr32, value);
    }
#endif
    register uint32_t addr_in_page = addr32 & _swap_page_mask;
    if (addr_in_page == _swap_page_mask) {
        for (uint32_t i = 0; i < 2; ++i) {
            ram_page_write(addr32 + i, (uint8_t)(value >> (i << 3)));
        }
        return;
    }
    register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return;
    register uint8_t* addr_in_ram = RAM + ram_page * _swap_page_size + addr_in_page;
    *addr_in_ram++     = (uint8_t) value;
    *addr_in_ram       = (uint8_t)(value >> 8);
    mark_ram_page_dirty(ram_page);
}

void ram_page_write32(uint32_t addr32, uint32_t value) {
    if (!swap_range_valid(addr32, 4)) return;
#ifdef BOOT_DEBUG_ACC
    if (addr32 >= BOOT_DEBUG_ACC) {
        goutf("Q %08X <- %08X\n", addr32, value);
    }
#endif
    register uint32_t addr_in_page = addr32 & _swap_page_mask;
    if (addr_in_page > _swap_page_size - 4) {
        for (uint32_t i = 0; i < 4; ++i) {
            ram_page_write(addr32 + i, (uint8_t)(value >> (i << 3)));
        }
        return;
    }
    register uint32_t ram_page = get_ram_page_for(addr32);
    if (!ram_page) return;
    register uint8_t* addr_in_ram = RAM + ram_page * _swap_page_size + addr_in_page;
    *addr_in_ram++     = (uint8_t) value;
    *addr_in_ram++     = (uint8_t)(value >> 8);
    *addr_in_ram++     = (uint8_t)(value >> 16);
    *addr_in_ram       = (uint8_t)(value >> 24);
    mark_ram_page_dirty(ram_page);
}

static uint32_t get_ram_page_for(const uint32_t addr32) {
    const register uint32_t lba_page = (addr32 >> _swap_page_div) + 1; // page idx
    if (last_lba_page == lba_page && last_ram_page) {
        return last_ram_page;
    }
    for (register uint32_t ram_page = 1; ram_page < _swap_pages; ++ram_page) {
        register uint16_t ram_page_desc = RAM_PAGES[ram_page];
        register uint16_t lba_page_in_ram = ram_page_desc & 0x7FFF; // 14-0 - max 32k keys for 4K LBA bloks
        if (lba_page_in_ram == lba_page) {
            last_lba_page = lba_page;
            last_ram_page = ram_page;
#ifdef BOOT_DEBUG_ACC
    goutf("VRAM page: 0x%X (ram_page: %X)\n", lba_page, ram_page);
#endif
            return ram_page;
        }
    }
#ifdef BOOT_DEBUG_ACC
    goutf("VRAM page: 0x%X\n", lba_page);
#endif
    // rolling page usage
    uint16_t ram_page = oldest_ram_page;
    uint16_t ram_page_desc = RAM_PAGES[ram_page];
    bool ro_page_was_found = !(ram_page_desc & 0x8000);
    // higest (15) bit is set, it means - the page has changes (RW page)
    uint32_t old_lba_page = ram_page_desc & 0x7FFF; // 14-0 - max 32k keys for 4K LBA bloks
    uint32_t ram_page_offset = ((uint32_t)ram_page) * _swap_page_size;
    if (!ro_page_was_found) {
        // Lets flush found RW page to sdcard
#ifdef BOOT_DEBUG_ACC
        goutf("2 RAM page 0x%X / VRAM page: 0x%X\n", ram_page, lba_page);
#endif
        uint32_t old_lba_page_offset = (old_lba_page - 1) * _swap_page_size;
#ifdef BOOT_DEBUG_ACC
        goutf("2 RAM offs 0x%X / VRAM offs: 0x%X\n", ram_page_offset, old_lba_page_offset);
#endif
        if (!flush_vram_block(RAM + ram_page_offset, old_lba_page_offset, _swap_page_size)) {
            return 0;
        }
    }
    // use new page; LBA ids are one-based, file offsets are zero-based
    if (ram_page == last_ram_page) {
        last_ram_page = 0;
        last_lba_page = 0;
    }
    uint32_t lba_page_offset = (lba_page - 1) * _swap_page_size;
    if (!read_vram_block(RAM + ram_page_offset, lba_page_offset, _swap_page_size)) {
        RAM_PAGES[ram_page] = 0;
        return 0;
    }
    RAM_PAGES[ram_page] = lba_page;
    if (++oldest_ram_page >= _swap_pages) oldest_ram_page = 1; // page 0 is reserved
    last_lba_page = lba_page;
    last_ram_page = ram_page;
    return ram_page;
}

uint32_t swap_size() {
    return _swap_size;
}

inline static void tokenize(char* s) {
    while(*s) {
        if (*s == ' ') {
            *s = 0;
        }
        s++;
    }
}

inline static uint8_t get_shift(char* s) {
    while(*s) {
        if (*s == 'K' || *s == 'k') {
            *s = 0;
            return 10;
        }
        if (*s == 'M' || *s == 'm') {
            *s = 0;
            return 20;
        }
        s++;
    }
    return 0;
}

static void release_vram(void) {
    if (path) vPortFree(path);
    if (_swap_pages_base) vPortFree(_swap_pages_base);
    if (_swap_base) vPortFree(_swap_base);
    path = 0;
    _swap_pages_base = 0;
    _swap_base = 0;
    RAM = 0;
    RAM_PAGES = 0;
    _swap_size = 0;
    _swap_base_size = 0;
    _swap_page_size = 0;
    _swap_page_mask = 0;
    _swap_page_div = 0;
    _swap_pages = 0;
}

uint32_t init_vram(char* cfg_in) { // "/mos/pagefile.sys 8M 128K 4K"
    size_t sz = strlen(cfg_in);
    if (!sz) {
        return 0;
    }
    char* cfg = (char*)pvPortMalloc(sz + 1);
    if (!cfg) return 0;
    strcpy(cfg, cfg_in);
    tokenize(cfg);
    if (_swap_size) f_close(&file);
    release_vram();
    path = (char*)pvPortMalloc(sz + 1);
    if (!path) goto e_cfg;
    strcpy(path, cfg);
    char* vszs = next_token(cfg);
    char* bszs = next_token(vszs);
    char* pszs = next_token(bszs);
    
    uint8_t shift = get_shift(vszs);
    _swap_size = atoi(vszs) << shift;

    shift = get_shift(bszs);
    _swap_base_size = atoi(bszs) << shift;

    shift = get_shift(pszs);
    _swap_page_size = atoi(pszs) << shift;
    switch(_swap_page_size) {
        case 1024:
            _swap_page_mask = 0x000003FF;
            _swap_page_div = 10;
            break;
        case 2048:
            _swap_page_mask = 0x000007FF;
            _swap_page_div = 11;
            break;
        case 4096:
            _swap_page_mask = 0x00000FFF;
            _swap_page_div = 12;
            break;
        default:
            goutf("Unsupported swap page size %d ignored, 2K is used instead\n", _swap_page_size);
            _swap_page_size = 2048;
            _swap_page_mask = 0x000007FF;
            _swap_page_div = 11;
            break;
    }
    if (!_swap_base_size || !_swap_size || _swap_size < _swap_base_size ||
        (_swap_size & _swap_page_mask) || (_swap_base_size & _swap_page_mask) ||
        _swap_base_size / _swap_page_size < 2 ||
        _swap_size / _swap_page_size > 0x7FFF) {
        goto e_cfg;
    }
    _swap_pages = _swap_base_size / _swap_page_size;
    _swap_pages_base = (char*)pvPortMalloc((_swap_pages << 1) + 3);
    _swap_base = (char*)pvPortMalloc(_swap_base_size + 3);
    if (!_swap_pages_base || !_swap_base) goto e_cfg;
    RAM = (uint8_t*)((uint32_t)(_swap_base + 3) & 0xFFFFFFFC);
    RAM_PAGES = (uint16_t*)((uint32_t)(_swap_pages_base + 3) & 0xFFFFFFFC);
    memset(RAM, 0, _swap_base_size);
    memset(RAM_PAGES, 0, _swap_pages << 1);
    oldest_ram_page = 1;
    last_ram_page = 0;
    last_lba_page = 0;

    f_unlink(path); // ensure it is new file
    FRESULT result = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result == FR_OK) {
        result = f_lseek(&file, _swap_size - 1);
        if (result != FR_OK) {
            goutf("Unable to init %s\n", path);
            goto e;
        }
        UINT bw;
        const char zero = 0;
        result = f_write(&file, &zero, 1, &bw);
        if (result != FR_OK || bw != 1) {
            goutf("Unable to init %s\n", path);
            goto e;
        }
    } else {
        goutf("Unable to create %s\n", path);
        goto e;
    }
    result = f_close(&file);
    if (result != FR_OK) {
        goutf("Unable to close %s after creation\n", path);
        goto e;
    }
    result = f_open(&file, path, FA_READ | FA_WRITE);
    if (result != FR_OK) {
        goutf("Unable to open %s\n", path);
        goto e;
    }
    vPortFree(cfg);
    return _swap_size;
e:
    f_close(&file);
    f_unlink(path);
e_cfg:
    release_vram();
    vPortFree(cfg);
    return 0;
}

FRESULT vram_seek(FIL* fp, uint32_t file_offset) {
    FRESULT result = f_lseek(fp, file_offset);
    if (result != FR_OK) {
        f_close(fp);
        result = f_open(fp, path, FA_READ | FA_WRITE);
        if (result != FR_OK) {
            goutf("Unable to open file '%s': %s (%d)\n", path, FRESULT_str(result), result);
            return result;
        }
        result = f_lseek(fp, file_offset);
        if (result != FR_OK) {
            goutf("Failed to f_lseek(%d): %s (%d)\n", file_offset, FRESULT_str(result), result);
        }
    }
    return result;
}

static bool vram_block(void* buffer, uint32_t file_offset, uint32_t sz, bool write) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
#ifdef BOOT_DEBUG_ACC
    goutf("Pagefile %c 0x%X @ 0x%X\n", write ? 'W' : 'R', buffer, file_offset);
#endif
    FRESULT result = vram_seek(&file, file_offset);
    UINT done = 0;
    if (result == FR_OK) {
        result = write ? f_write(&file, buffer, sz, &done)
                       : f_read(&file, buffer, sz, &done);
    }
    bool ok = result == FR_OK && done == sz;
    if (!ok) goutf("Swap I/O failed: %s (%d), %d/%d bytes\n",
                   FRESULT_str(result), result, done, sz);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return ok;
}

bool read_vram_block(uint8_t* dst, uint32_t file_offset, uint32_t sz) {
    return vram_block(dst, file_offset, sz, false);
}

bool flush_vram_block(const uint8_t* src, uint32_t file_offset, uint32_t sz) {
    return vram_block((void*)src, file_offset, sz, true);
}
