#include "app.h"
#include <pico/platform.h>
#include <hardware/flash.h>
#include <pico/stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"

#define M_OS_APP_TABLE_BASE ((size_t*)0x10002000ul)
typedef int (*boota_ptr_t)( void *argv );

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

bool __not_in_flash_func(load_firmware)(const char pathname[256]) {
    UINT bytes_read = 0;
    FIL file;
    if (FR_OK != f_open(&file, pathname, FA_READ)) {
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

void vAppTask(void *pv) {
    int res = ((boota_ptr_t)M_OS_APP_TABLE_BASE[0])(pv); // TODO:
    vTaskDelete( NULL );
    // TODO: ?? return res;
}

void run_app(char * name) {
    xTaskCreate(vAppTask, name, configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}

bool new_app(char * name) {
    // TODO:
    return true;
}

#include "elf32.h"
// Декодирование и обновление инструкции BL для ссылки типа R_ARM_THM_PC22
// Функция для разрешения ссылки типа R_ARM_THM_PC22
void resolve_thm_pc22(uint32_t *addr, uint32_t sym_val, uint32_t rel_addr) {
    register uint32_t instr = *addr;
    // Декодирование текущего смещения
    uint32_t S = (instr >> 26) & 1;
    uint32_t J1 = (instr >> 13) & 1;
    uint32_t J2 = (instr >> 11) & 1;
    uint32_t imm10 = (instr >> 16) & 0x03FF;
    uint32_t imm11 = instr & 0x07FF;

    uint32_t I1 = !(J1 ^ S);
    uint32_t I2 = !(J2 ^ S);
    uint32_t offset = (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
    if (S) {
        offset |= 0xFF800000; // знак расширение
    }

    // Вычисление нового смещения
    uint32_t new_offset = (uint32_t)((int32_t)offset + sym_val - rel_addr);
    S = new_offset >> 31;
    I1 = (new_offset >> 23) & 1;
    I2 = (new_offset >> 22) & 1;
    imm10 = (new_offset >> 12) & 0x03FF;
    imm11 = (new_offset >> 1) & 0x07FF;

    J1 = !(I1 ^ S);
    J2 = !(I2 ^ S);

    // Обновление инструкции
    *addr = (0b11110 << 27) | (S << 26) | (imm10 << 16) || (0b11010 << 11) | (J1 << 13) | (J2 << 11) | imm11;
}

typedef struct {
    FIL *f2;
    const elf32_header *pehdr;
    char* addr; // updatable
    const int symtab_off;
    const elf32_sym* psym
} load_sec_ctx;

inline static uint32_t sec_align(uint32_t addr, uint32_t a) {
    if (a == 0 || a == 1) return addr;
    if (a == (1 << 1)) return (addr & 1) ? addr + 1 : addr;
    for (uint8_t b = 2; b < 32; b++) {
        if (a == (1 << b)) {
            if (addr & (a - 1)) {
                return (addr & (0xFFFFFFFF ^ (a - 1))) + a;
            }
            return addr;
        }
    }
    goutf("Unsupported allignment: %d\n", a);
    return addr;
}

static uint8_t* load_sec2mem(load_sec_ctx * c, uint16_t sec_num) {
    UINT rb;
    uint8_t* addr = c->addr;
    elf32_shdr sh;
    if (f_lseek(c->f2, c->pehdr->sh_offset + sizeof(elf32_shdr) * sec_num) == FR_OK &&
        f_read(c->f2, &sh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)
    ) {
        addr = sec_align((uint32_t)addr, sh.sh_addralign);
        // todo: enough space?
        uint32_t sz = sh.sh_size;
        if (f_lseek(c->f2, sh.sh_offset) == FR_OK &&
            f_read(c->f2, addr, sh.sh_size, &rb) == FR_OK && rb == sh.sh_size
        ) { // section in memory now
            goutf("Program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, addr);
            // links and relocations
            f_lseek(c->f2, c->pehdr->sh_offset);
            while (f_read(c->f2, &sh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) {
                if(sh.sh_type == REL_SEC && sh.sh_info == sec_num) {
                    uint32_t r2 = f_tell(c->f2);
                    elf32_rel rel;
                    for (uint32_t j = 0; j < sh.sh_size / sizeof(rel); ++j) {
                        if (f_lseek(c->f2, sh.sh_offset + j * sizeof(rel)) != FR_OK ||
                            f_read(c->f2, &rel, sizeof(rel), &rb) != FR_OK || rb != sizeof(rel)
                        ) {
                            goutf("Unable to read REL record #%d in section #%d\n", j, sh.sh_info);
                            return 0;
                        }
                        uint32_t rel_sym = rel.rel_info >> 8;
                        uint8_t rel_type = rel.rel_info & 0xFF;
                        if (f_lseek(c->f2, c->symtab_off + rel_sym * sizeof(elf32_sym)) != FR_OK ||
                            f_read(c->f2, c->psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)
                        ) {
                            goutf("Unable to read .symtab section #%d\n", rel_sym);
                            return 0;
                        }
                        char * sec_addr = addr;
                        goutf("rel_offset: %p; rel_sym: %d; rel_type: %d; -> %d\n", rel.rel_offset, rel_sym, rel_type, c->psym->st_shndx);
                        if (c->psym->st_shndx != sec_num) {
                            // lets load other section
                            c->addr += sz;
                            // todo: enough space?
                            sec_addr = load_sec2mem(c, c->psym->st_shndx);
                            if (sec_addr == 0) { // TODO: already loaded (app context)
                                return 0;
                            }
                        }
                        uint32_t* rel_addr = (uint32_t*)(sec_addr + rel.rel_offset);
                        uint32_t A = *rel_addr;
                        uint32_t P = rel.rel_offset;;
                        uint32_t S = c->psym->st_value;
                        // Разрешение ссылки
                        switch (rel_type) {
                            case 2: //R_ARM_ABS32:
                                *rel_addr = S + A;
                                break;
                            case 3: //R_ARM_REL32:
                                *rel_addr = S - P + A; // todo: signed?
                                break;
                            case 10: //R_ARM_THM_PC22:
                                resolve_thm_pc22(rel_addr, S, P);
                                break;
                            default:
                                goutf("Unsupportel REL type: %d\n", rel_type);
                                return 0;
                        }
                    }
                    f_lseek(c->f2, r2);
                }
            }
        } else {
            goutf("Unable to load program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, addr);
        }
    } else {
        goutf("Unable to read section #%d info\n", sec_num);
        return 0;
    }
    return addr;
}

static const char* s = "Unexpected ELF file";

void run_new_app(char * fn, char * fn1) {
    FIL f2;
    if (f_open(&f2, fn, FA_READ) != FR_OK) {
        goutf("Unable to open file: '%s'\n", fn);
        return;
    }
    struct elf32_header ehdr;
    UINT rb;
    if (f_read(&f2, &ehdr, sizeof(ehdr), &rb) != FR_OK) {
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        goto e1;
    }
    if (ehdr.common.magic != ELF_MAGIC) {
        goutf("It is not an ELF file: '%s'\n", fn);
        goto e1;
    }
    if (ehdr.common.version != 1 || ehdr.common.version2 != 1) {
        goutf("%s '%s' version: %d:%d\n", s, fn, ehdr.common.version, ehdr.common.version2);
        goto e1;
    }
    if (ehdr.common.arch_class != 1 || ehdr.common.endianness != 1) {
        goutf("%s '%s' class: %d; endian: %d\n", s, fn, ehdr.common.arch_class, ehdr.common.endianness);
        goto e1;
    }
// TODO: ???
//    if (ehdr.eh_size != sizeof(struct elf32_header)) {
//        goutf("%s '%s' header size: %d; expected: %d\n", s, fn, ehdr.eh_size, sizeof(ehdr));
//        goto e1;
//    }
    if (ehdr.common.machine != EM_ARM) {
        goutf("%s '%s' machine type: %d; expected: %d\n", s, fn, ehdr.common.machine, EM_ARM);
        goto e1;
    }
    if (ehdr.common.abi != 0) {
        goutf("%s '%s' ABI type: %d; expected: %d\n", s, fn, ehdr.common.abi, 0);
        goto e1;
    }
    if (ehdr.flags & EF_ARM_ABI_FLOAT_HARD) {
        goutf("%s '%s' EF_ARM_ABI_FLOAT_HARD: %04Xh\n", s, fn, ehdr.flags);
        goto e1;
    }
    // TODO: remove it later
    //goutf("Size of section headers:           %d\n", ehdr.sh_entry_size);
    //goutf("Number of section headers:         %d\n", ehdr.sh_num);
    //goutf("Section header string table index: %d\n", ehdr.sh_str_index);
    //goutf("Start of section headers:          %d\n", ehdr.sh_offset);
    elf32_shdr sh;
    bool ok = f_lseek(&f2, ehdr.sh_offset + sizeof(sh) * ehdr.sh_str_index) == FR_OK;
    if (!ok || f_read(&f2, &sh, sizeof(sh), &rb) != FR_OK || rb != sizeof(sh)) {
        goutf("Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(&f2), sizeof(sh), rb);
        goto e1;
    }
    char* symtab = (char*)pvPortMalloc(sh.sh_size);
    ok = f_lseek(&f2, sh.sh_offset) == FR_OK;
    if (!ok || f_read(&f2, symtab, sh.sh_size, &rb) != FR_OK || rb != sh.sh_size) {
        goutf("Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(&f2), sh.sh_size, rb);
        goto e2;
    }
    f_lseek(&f2, ehdr.sh_offset);
    int symtab_off, strtab_off = -1;
    UINT symtab_len, strtab_len = 0;
    while ((symtab_off < 0 || strtab_off < 0) && f_read(&f2, &sh, sizeof(sh), &rb) == FR_OK && rb == sizeof(sh)) { 
        if(sh.sh_type == 2 && 0 == strcmp(symtab + sh.sh_name, ".symtab")) {
            symtab_off = sh.sh_offset;
            symtab_len = sh.sh_size;
        }
        if(sh.sh_type == 3 && 0 == strcmp(symtab + sh.sh_name, ".strtab")) {
            strtab_off = sh.sh_offset;
            strtab_len = sh.sh_size;
        }
    }
    if (symtab_off < 0 || strtab_off < 0) {
        goutf("Unable to find .strtab/.symtab sections\n");
        goto e2;
    }
    f_lseek(&f2, strtab_off);
    char* strtab = (char*)pvPortMalloc(strtab_len);
    if (f_read(&f2, strtab, strtab_len, &rb) != FR_OK || rb != strtab_len) {
        goutf("Unable to read .strtab section\n");
        goto e3;
    }
    f_lseek(&f2, symtab_off);
    elf32_sym sym;
    bool lst = strcmp(fn1, "lst") == 0;
    if (lst) {
        goutf("Global function names:\n");
    }
    for (int i = 0; i < symtab_len / sizeof(sym); ++i) {
        if (f_read(&f2, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)) {
            goutf("Unable to read .symtab section #%d\n", i);
            break;
        }
        if (sym.st_info == STR_TAB_GLOBAL_FUNC) {
            if (lst) {
                goutf(" - %s\n", strtab + sym.st_name);
            } else {
                if (0 == strcmp(fn1, strtab + sym.st_name)) { // found req. function
                    char* page = (char*)pvPortMalloc(4 << 10); // a 4k page for the process
                    load_sec_ctx ctx = {
                        &f2,
                        &ehdr,
                        page,
                        symtab_off,
                        &sym
                    };
                    if (load_sec2mem(&ctx, sym.st_shndx) == 0) {
                        // err
                    }
                    // start it
                    goutf("OK\n");
                    
                    vPortFree(page);
                    // TODO:
                    break;
                }
            }
        }
    }
e3:
    vPortFree(strtab);
e2:
    vPortFree(symtab);
e1:
    f_close(&f2);
}
