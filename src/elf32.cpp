#include "elf32.h"
#include "graphics.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"

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
    goutf("%s. Memory segment %08x->%08x is outside of valid address range for device\n", addr, addr+size);
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
    case 5:
        return "HASH    ";
    case 6:
        return "DYNAMIC ";
    case 8:
        return "NOBITS  ";
    case 9:
        return "REL     ";
    case 10:
        return "SHLIB   ";
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
static const char* st_info_type2str(char c) {
    switch (c)
    {
    case 0:
        return "NOTYPE ";
    case 1:
        return "OBJECT ";
    case 2:
        return "FUNC   ";
    case 3:
        return "SECTION";
    case 4:
        return "FILE   ";
    case 13:
        return "LOPROC ";
    case 15:
        return "HIPROC ";
    case 14:
        return "PROC   ";
    default:
        break;
    }
    return "ST_TYPE?";
}
static const char* st_info_bind2str(char c) {
    switch (c)
    {
    case 0:
        return "LOCAL ";
    case 1:
        return "GLOBAL";
    case 2:
        return "WEAK  ";
    case 13:
        return "LOPROC ";
    case 15:
        return "HIPROC ";
    case 14:
        return "PROC   ";
    default:
        break;
    }
    return "ST_BIND?";
}
static const char* st_predef(const char* v) {
    if(strlen(v) == 2) {
        if (strcmp(v, "%t") == 0) {
            return "%t (Thrumb)";
        }
        if (strcmp(v, "\%d") == 0) {
            return "\%d (data)";
        }
    }
    return v;
}
static const char* st_spec_sec(uint16_t st) {
    if (st >= 0xff00 && st <= 0xff1f)
        return " PROC";
    switch (st)
    {
    case 0xffff:
        return " HIRESERVE";
    case 0xfff2:
        return " COMMON";
    case 0xfff1:
        return " ABS";
    case 0:
        return " UNDEF";
    default:
        break;
    }
    return "";
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
        goutf("Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(&f2), sizeof(sh), rb);
    } else {
        // TODO: check type?
        char* symtab = (char*)pvPortMalloc(sh.sh_size);
        ok = f_lseek(&f2, sh.sh_offset) == FR_OK;
        if (!ok || f_read(&f2, symtab, sh.sh_size, &rb) != FR_OK || rb != sh.sh_size) {
            goutf("Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(&f2), sh.sh_size, rb);
        }

        int symtab_off, strtab_off, dyntab_off, dynstr_off = -1;
        UINT symtab_len, strtab_len, dyntab_len, dynstr_len = 0;
        f_lseek(&f2, ehdr.sh_offset);
        int i = 0;
        while (f_read(&f2, &sh, sizeof(sh), &rb) == FR_OK && rb == sizeof(sh)) {
            fgoutf(f, "%02d %s(%x) %s%s%s%s (%02xh) %p:%p (%04d) A%04d E%02d L%02d %s",
                   i,
                   sh_type2name(sh.sh_type), sh.sh_type,
                   sh_flags_w(sh.sh_flags), sh_flags_a(sh.sh_flags), sh_flags_x(sh.sh_flags), sh_flags_s(sh.sh_flags), sh.sh_flags,
                   sh.sh_addr, sh.sh_offset, sh.sh_size,
                   sh.sh_addralign, sh.sh_entsize, sh.sh_link,
                   symtab + sh.sh_name
            );
            if (sh.sh_info) fgoutf(f, " i%ph\n", sh.sh_info);
            else fgoutf(f, "\n");
            if(sh.sh_type == 2 && 0 == strcmp(symtab + sh.sh_name, ".symtab")) {
                symtab_off = sh.sh_offset;
                symtab_len = sh.sh_size;
            }
            if(sh.sh_type == 3 && 0 == strcmp(symtab + sh.sh_name, ".strtab")) {
                strtab_off = sh.sh_offset;
                strtab_len = sh.sh_size;
            }
            if(sh.sh_type == 0xb && 0 == strcmp(symtab + sh.sh_name, ".dynsym")) {
                dyntab_off = sh.sh_offset;
                dyntab_len = sh.sh_size;
            }
            if(sh.sh_type == 3 && 0 == strcmp(symtab + sh.sh_name, ".dynstr")) {
                dynstr_off = sh.sh_offset;
                dynstr_len = sh.sh_size;
            }
            if(sh.sh_type == 9) {
                uint32_t r2 = f_tell(&f2);
                f_lseek(&f2, sh.sh_offset);
                uint32_t len = sh.sh_size;
                elf32_rel rel;
                while(len) {
                    f_read(&f2, &rel, sizeof(rel), &rb);
                    fgoutf(f, "REL: %p:%p\n", rel.rel_offset, rel.rel_info);
                    len -= sh.sh_entsize;
                }
                f_lseek(&f2, r2);
            }
            i++;
        }
        vPortFree(symtab);
        if (symtab_off < 0) {
            goutf("Unable to find .symtab section header\n");            
        } else if (strtab_off < 0) {
            goutf("Unable to find .strtab section header\n");            
        } else {
            fgoutf(f, "SYMTAB:\n");
            char* strtab = (char*)pvPortMalloc(strtab_len);
            f_lseek(&f2, strtab_off);
            if(f_read(&f2, strtab, strtab_len, &rb) != FR_OK || rb != strtab_len) {
                goutf("%s '%s' Unable to read .strtab section #%d\n", s, fn, i);
            } else {                
                f_lseek(&f2, symtab_off);
                elf32_sym sym;
                for(int i = 0; i < symtab_len / sizeof(sym); ++i) {
                    if(f_read(&f2, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)) {
                        goutf("Unable to read .symtab section #%d\n", i);
                        break;
                    }
                    fgoutf(f, "%02d %ph %s %s %s (%d) -> %d%s\n",
                           i, sym.st_value,
                           st_info_type2str(sym.st_info & 0xF),
                           st_info_bind2str(sym.st_info >> 4),
                           st_predef(strtab + sym.st_name),
                           sym.st_size, sym.st_shndx, st_spec_sec(sym.st_shndx)
                    );
                }
            }
            vPortFree(strtab);
        }
        if (dyntab_off < 0) {
            goutf("Unable to find .dynsym section header\n");            
        } else if (dynstr_off < 0) {
            goutf("Unable to find .dynstr section header\n");            
        } else {
            fgoutf(f, "DYNTAB:\n");
            char* strtab = (char*)pvPortMalloc(dynstr_len);
            f_lseek(&f2, dynstr_off);
            if(f_read(&f2, strtab, dynstr_len, &rb) != FR_OK || rb != dynstr_len) {
                goutf("Unable to read .dynstr section #%d\n", i);
            } else {                
                f_lseek(&f2, dyntab_off);
                elf32_sym sym;
                for(int i = 0; i < dyntab_len / sizeof(sym); ++i) {
                    if(f_read(&f2, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)) {
                        goutf("Unable to read .dyntab section #%d\n", i);
                        break;
                    }
                    fgoutf(f, "%02d %ph %s %s %s (%d) -> %d%s\n",
                           i, sym.st_value, st_info_type2str(sym.st_info & 0xF), st_info_bind2str(sym.st_info >> 4),
                           strtab + sym.st_name, sym.st_size, sym.st_shndx, st_spec_sec(sym.st_shndx)
                    );
                }
            }
            vPortFree(strtab);
        }
    }
    f_close(&f2);
}
