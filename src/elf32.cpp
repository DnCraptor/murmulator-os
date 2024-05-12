#include "elf32.h"
#include "graphics.h"
#include "FreeRTOS.h"
#include "task.h"

// we require 256 (as this is the page size supported by the device)
#define LOG2_PAGE_SIZE 8u
#define PAGE_SIZE (1u << LOG2_PAGE_SIZE)

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

int check_address_range(
    FIL* f,
    const address_range* valid_ranges,
    uint32_t addr,
    uint32_t vaddr,
    uint32_t size,
    bool uninitialized,
    address_range &ar
) {
    for(int i = 0; i < 3; i++) {
        const auto& range = valid_ranges[i];
        if (range.from <= addr && range.to >= addr + size) {
            if (range.type == address_range::type::NO_CONTENTS && !uninitialized) {
                goutf("%s; it contains memory contents for uninitialized memory\n", s);
                return -1;
            }
            ar = range;
            fgoutf(f, "%s segment %08x->%08x (%08x->%08x)\n", uninitialized ? "Uninitialized" : "Mapped", addr,
                   addr + size, vaddr, vaddr+size);
            return 0;
        }
    }
    goutf("%s. Memory segment %08x->%08x is outside of valid address range for device", addr, addr+size);
    return -1;
}

static int read_and_check_elf32_ph_entries(
    FIL*f, // stdout
    FIL*f2,
    const elf32_header &eh,
    const address_range* valid_ranges
) {
    if (eh.ph_entry_size != sizeof(elf32_ph_entry)) {
        goutf("%s program header size: %d; expected: %d\n", s, eh.ph_entry_size, sizeof(eh.ph_entry_size));
        return -1;
    }
    if (eh.ph_num) {
        elf32_ph_entry entry;
        if (f_lseek(f2, eh.ph_offset) != FR_OK) {
            goutf("%s f_lseek error to %d\n", s, eh.ph_offset);
            return -1;
        }
        UINT r;
        for(uint i = 0; i < eh.ph_num; i++) {
            if (f_read(f2, &entry, sizeof(struct elf32_ph_entry), &r) != FR_OK || r != sizeof(struct elf32_ph_entry)) {
                goutf("%s f_read error (i=%d)\n", i);
                return -1;
            }
            fgoutf(f, "entry.type: %d; entry.memsz: %d; entry.filez: %d\n", entry.type, entry.memsz, entry.filez);
           // elf32_ph_entry& entry = entries[i];
            if (entry.type == PT_LOAD && entry.memsz) {
                address_range ar;
                int rc;
                uint mapped_size = entry.filez > entry.memsz ? entry.memsz : entry.filez;
                if (mapped_size) {
                    rc = check_address_range(f, valid_ranges, entry.paddr, entry.vaddr, mapped_size, false, ar);
                    if (rc) return rc;
                    // we don't download uninitialized, generally it is BSS and should be zero-ed by crt0.S, or it may be COPY areas which are undefined
                    if (ar.type != address_range::type::CONTENTS) {
                        fgoutf(f, "  ignored\n");
                        continue;
                    }
                    uint addr = entry.paddr;
                    uint remaining = mapped_size;
                    uint file_offset = entry.offset;
                    while (remaining) {
                        uint off = addr & (PAGE_SIZE - 1);
                        uint len = remaining > PAGE_SIZE - off ? PAGE_SIZE - off : remaining;
                   //     auto &fragments = pages[addr - off]; // list of fragments
                        // note if filesz is zero, we want zero init which is handled because the
                        // statement above creates an empty page fragment list
                        // check overlap with any existing fragments
                       // for (const auto &fragment : fragments) {
                //            if ((off < fragment.page_offset + fragment.bytes) != ((off + len) <= fragment.page_offset)) {
                //                fail(ERROR_FORMAT, "In memory segments overlap");
                //            }
                       // }
                      //  fragments.push_back(
                //                page_fragment{file_offset, off, len}
                                //);
                        addr += len;
                        file_offset += len;
                        remaining -= len;
                    }
                }
                if (entry.memsz > entry.filez) {
                    // we have some uninitialized data too
                    rc = check_address_range(f, valid_ranges, entry.paddr + entry.filez,
                                             entry.vaddr + entry.filez, entry.memsz - entry.filez, true,
                                             ar);
                    if (rc) return rc;
                }
            }
        }
    }
    return 0;
}

static char * sh_type2name(uint32_t t) {
    switch (t)
    {
    case 0:
        return "NULL    ";
    case 1:
        return "PROGBITS";
    case 2:
        return "SYMTAB  ";
    case 0xB:
        return "DYNSYM  ";
    case 3:
        return "STRTAB  ";
    case 4:
        return "RELA    ";
    case 6:
        return "DYNAMIC ";
    case 8:
        return "NOBITS  ";
    default:
        break;
    }
    return "?";
}

static const char* sh_flags_w(uint32_t f) {
    return (f & 1) ? "w" : " ";
}
static const char* sh_flags_a(uint32_t f) {
    return (f & 2) ? "a" : " ";
}
static const char* sh_flags_x(uint32_t f) {
    return (f & 4) ? "x" : " ";
}
static const char* sh_flags_s(uint32_t f) {
    return (f & 0x20) ? "s" : " ";
}

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
    const address_range *valid_ranges = ram_style ? rp2040_address_ranges_ram : rp2040_address_ranges_flash;
    int rc = read_and_check_elf32_ph_entries(f, &f2, ehdr, valid_ranges);
    fgoutf(f, "Size of section headers:           %d\n", ehdr.sh_entry_size);
    fgoutf(f, "Number of section headers:         %d\n", ehdr.sh_num);
    fgoutf(f, "Section header string table index: %d\n", ehdr.sh_str_index);
    fgoutf(f, "Start of section headers:          %d\n", ehdr.sh_offset);
    elf32_shdr sh;
    bool ok = f_lseek(&f2, ehdr.sh_offset + sizeof(sh) * ehdr.sh_str_index) == FR_OK;
    if (!ok || f_read(&f2, &sh, sizeof(sh), &rb) != FR_OK || rb != sizeof(sh)) {
        goutf("%s '%s' Unable to read .shstrtab section header @ %d+%d (read: %d)\n", s, fn, f_tell(&f2), sizeof(sh), rb);
    } else {
        // TODO: check type?
        char* symtab = (char*)pvPortMalloc(sh.sh_size);
        ok = f_lseek(&f2, sh.sh_offset) == FR_OK;
        if (!ok || f_read(&f2, symtab, sh.sh_size, &rb) != FR_OK || rb != sh.sh_size) {
            goutf("%s '%s' Unable to read .shstrtab section @ %d+%d (read: %d)\n", s, fn, f_tell(&f2), sh.sh_size, rb);
        }

        f_lseek(&f2, ehdr.sh_offset);
        int i = 0;
        while (f_read(&f2, &sh, sizeof(sh), &rb) == FR_OK && rb == sizeof(sh)) {
            fgoutf(f, "%02d %s(%x) %s%s%s%s (%02xh) %ph (%04d) A%04d E%02d L%02d %s",
                   i,
                   sh_type2name(sh.sh_type), sh.sh_type,
                   sh_flags_w(sh.sh_flags), sh_flags_a(sh.sh_flags), sh_flags_x(sh.sh_flags), sh_flags_s(sh.sh_flags), sh.sh_flags,
                   sh.sh_addr, sh.sh_size,
                   sh.sh_addralign, sh.sh_entsize, sh.sh_link,
                   symtab + sh.sh_name
            );
            if (sh.sh_info) fgoutf(f, " i%ph\n", sh.sh_info);
            else fgoutf(f, "\n");
            i++;
        }
        vPortFree(symtab);
    }
    if (rb > 0) {
        goutf("%s '%s' Unable to read section header @ %d+%d (read: %d)\n", s, fn, f_tell(&f2), sizeof(sh), rb);
    }
    f_close(&f2);
}
