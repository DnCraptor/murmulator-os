#include "elf.h"
#include "graphics.h"

struct address_range {
    enum type {
        CONTENTS,     // may have contents
        NO_CONTENTS,  // must be uninitialized
        IGNORE        // will be ignored
    };
    address_range(uint32_t from, uint32_t to, type type) : from(from), to(to), type(type) {}
    address_range() : address_range(0, 0, IGNORE) {}
    type type;
    uint32_t to;
    uint32_t from;
};

#define MAIN_RAM_START        0x20000000u
#define MAIN_RAM_END          0x20042000u
#define FLASH_START           0x10000000u
#define FLASH_END             0x15000000u
#define XIP_SRAM_START        0x15000000u
#define XIP_SRAM_END          0x15004000u
#define MAIN_RAM_BANKED_START 0x21000000u
#define MAIN_RAM_BANKED_END   0x21040000u

const address_range rp2040_address_ranges_flash[3] = {
    address_range(FLASH_START, FLASH_END, address_range::type::CONTENTS),
    address_range(MAIN_RAM_START, MAIN_RAM_END, address_range::type::NO_CONTENTS),
    address_range(MAIN_RAM_BANKED_START, MAIN_RAM_BANKED_END, address_range::type::NO_CONTENTS)
};

const address_range rp2040_address_ranges_ram[3] {
    address_range(MAIN_RAM_START, MAIN_RAM_END, address_range::type::CONTENTS),
    address_range(XIP_SRAM_START, XIP_SRAM_END, address_range::type::CONTENTS),
    address_range(0x00000000u, 0x00004000u, address_range::type::IGNORE) // for now we ignore the bootrom if present
};

static bool is_address_initialized(const address_range* valid_ranges, uint32_t addr) {
    for(int i = 0; i < 3; ++i) {
        const auto& range = valid_ranges[i];
        if (range.from <= addr && range.to > addr) {
            return address_range::type::CONTENTS == range.type;
        }
    }
    return false;
}

static const char* s = "Unexpected ELF file";

extern "C" void elfinfo(FIL *f, char *fn) {
    FIL f2;
    if (f_open(&f2, fn, FA_READ) != FR_OK) {
        goutf("Unable to open file: '%s'\n", fn);
        return;
    }
    struct elf32_header ehdr;
    UINT rb;
    if (f_read(&f2, &ehdr, sizeof(ehdr), &rb) != FR_OK) {
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        f_close(&f2);
        return;
    }
    if (ehdr.common.magic != ELF_MAGIC) {
        goutf("It is not an ELF file: '%s'\n", fn);
        f_close(&f2);
        return;
    }
    if (ehdr.common.version != 1 || ehdr.common.version2 != 1) {
        goutf("%s '%s' version: %d:%d\n", s, fn, ehdr.common.version, ehdr.common.version2);
    }
    if (ehdr.common.arch_class != 1 || ehdr.common.endianness != 1) {
        goutf("%s '%s' class: %d; endian: %d\n", s, fn, ehdr.common.arch_class, ehdr.common.endianness);
    }
    if (ehdr.eh_size != sizeof(struct elf32_header)) {
        goutf("%s '%s' header size: %d; expected: %d\n", s, fn, ehdr.eh_size, sizeof(ehdr));
    }
    if (ehdr.common.machine != EM_ARM) {
        goutf("%s '%s' machine type: %d; expected: %d\n", s, fn, ehdr.common.machine, EM_ARM);
    }
    if (ehdr.common.abi != 0) {
        goutf("%s '%s' ABI type: %d; expected: %d\n", s, fn, ehdr.common.abi, 0);
    }
    if (ehdr.flags & EF_ARM_ABI_FLOAT_HARD) {
        goutf("%s '%s' EF_ARM_ABI_FLOAT_HARD: %04Xh\n", s, fn, ehdr.flags);
    }
    bool ram_style = is_address_initialized(rp2040_address_ranges_ram, ehdr.entry);
    if (ram_style) fgoutf(f, "RAM style binary\n");
    else fgoutf(f, "FLASH style binary\n");
    f_close(&f2);
}
