#include "app.h"
#include <pico/platform.h>
#include <hardware/flash.h>
#include <pico/stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"
#include "graphics.h"
#include "cmd.h"

#define M_OS_API_TABLE_BASE ((size_t*)0x10001000ul)
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

inline static uint32_t read_flash_block(FIL * f, uint8_t * buffer, uint32_t expected_flash_target_offset, UF2_Block_t* puf2) {
    UINT bytes_read = 0;
    uint32_t data_sector_index = 0;
    for(; data_sector_index < FLASH_SECTOR_SIZE; data_sector_index += 256) {
        fgoutf(get_stdout(), "Read block: %ph; ", f_tell(f));
        f_read(f, puf2, sizeof(UF2_Block_t), &bytes_read);
        fgoutf(get_stdout(), "(%d bytes) ", bytes_read);
        if (!bytes_read) {
            break;
        }
        if (expected_flash_target_offset != puf2->targetAddr - XIP_BASE) {
            f_lseek(f, f_tell(f) - sizeof(UF2_Block_t)); // we will reread this block, it doesnt belong to this continues block
            expected_flash_target_offset = puf2->targetAddr - XIP_BASE;
            fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
            break;
        }
        fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
        memcpy(buffer + data_sector_index, puf2->data, 256);
        expected_flash_target_offset += 256;
    }
    return expected_flash_target_offset;
}

static FIL file;
static FILINFO fileinfo;

void flash_block(uint8_t* buffer, size_t flash_target_offset) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    multicore_lockout_start_blocking();
    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE);
    flash_range_program(flash_target_offset, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

static uint8_t __aligned(4) buffer[FLASH_SECTOR_SIZE];

bool __not_in_flash_func(restore_tbl)(char* fn) {
    if (f_open(&file, fn, FA_READ) != FR_OK) {
        return false;
    }
    UINT rb;
    if (f_read(&file, buffer, FLASH_SECTOR_SIZE, &rb) != FR_OK || rb != FLASH_SECTOR_SIZE) {
        goutf("Failed to read '%s' file to restore OS API\n", fn);
        f_close(&file);
        return false;
    }
    f_close(&file);
    uint8_t* b = buffer;
    uint8_t* fl = (uint8_t*)M_OS_API_TABLE_BASE;
    for (int i = 0; i < 400; ++i) { // it is enough to test first 100 addresses
        if (*b++ != *fl++) {
            b--; fl--;
            goutf("Restoring OS API functions table, because [%p]:%02X <> %02X\n", fl, *fl, *b);
            sleep_ms(15000);

            flash_block(b, M_OS_API_TABLE_BASE - XIP_BASE);
            break;
        }
    }
    return true;
}


bool __not_in_flash_func(load_firmware_sram)(char* pathname) {
    if (FR_OK != f_open(&file, pathname, FA_READ)) {
        return false;
    }
    UF2_Block_t* uf2 = (UF2_Block_t*)pvPortMalloc(sizeof(UF2_Block_t));

    uint32_t flash_target_offset = 0;
    bool toff = false;
    while(true) {
        uint32_t next_flash_target_offset = read_flash_block(&file, buffer, flash_target_offset, uf2);
        if (next_flash_target_offset == flash_target_offset) {
            break;
        }
        //подмена загрузчика boot2 прошивки на записанный ранее
        if (flash_target_offset == 0) {
            memcpy(buffer, (uint8_t *)XIP_BASE, 256);
            fgoutf(get_stdout(), "Replace loader @ offset 0\n");
        }
        fgoutf(get_stdout(), "Erase and write to flash, offset: %ph\n", flash_target_offset);
        flash_block(buffer, flash_target_offset);
        flash_target_offset = next_flash_target_offset;
    }
    f_close(&file);
    f_open(&file, FIRMWARE_MARKER_FN, FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_WRITE);
    fgoutf(&file, "%s", pathname);
    f_close(&file);
    f_close(get_stdout());
    f_close(get_stderr());

    watchdog_enable(100, true);

    vPortFree(uf2);

    while(1);
    __unreachable();
}

bool load_firmware(char* pathname) {
    f_stat(pathname, &fileinfo);
    if (FLASH_SIZE - 64 << 10 < fileinfo.fsize / 2) {
        fgoutf(get_stdout(), "ERROR: Firmware too large (%dK)! Canceled!\n", fileinfo.fsize >> 11);
        return false;
    }
    char* t = concat(get_cmd_startup_ctx()->base, OS_TABLE_BACKUP_FN);
    fgoutf(get_stdout(), "Backup OS functions table tp '%s'\n", t);
    if (FR_OK != f_open(&file, t, FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_WRITE) ) {
        fgoutf(get_stdout(), "ERROR: Unable to open backup file: '%s'!\n", t);
        vPortFree(t);
        return false;
    }
    UINT wb = 0;
    if (FR_OK != f_write(&file, M_OS_API_TABLE_BASE, FLASH_SECTOR_SIZE, &wb) || wb != FLASH_SECTOR_SIZE)  {
        fgoutf(get_stdout(), "ERROR: Unable to write to backup file: '%s'!\n", t);
        vPortFree(t);
        f_close(&file);
        return false;
    }
    vPortFree(t);
    f_close(&file);

    fgoutf(get_stdout(), "Loading firmware: '%s'\n", pathname);
    return load_firmware_sram(pathname);
}

void vAppTask(void *pv) {
    int res = ((boota_ptr_t)M_OS_APP_TABLE_BASE[0])(pv); // TODO:
    vTaskDelete( NULL );
    // TODO: ?? return res;
}

void run_app(char * name) {
    xTaskCreate(vAppTask, name, configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}

#include "elf32.h"
// Декодирование и обновление инструкции BL для ссылки типа R_ARM_THM_PC22
// Функция для разрешения ссылки типа R_ARM_THM_PC22
void resolve_thm_pc22(uint16_t *addr, uint32_t sym_val) {
    uint16_t instr0 = *addr;
    uint16_t instr = *(addr + 1);
    // Декодирование текущего смещения
    uint32_t S = (instr0 >> 10) & 1;
    uint32_t J1 = (instr >> 13) & 1;
    uint32_t J2 = (instr >> 11) & 1;
    uint32_t imm10 = instr0 & 0x03FF;
    uint32_t imm11 = instr & 0x07FF;

    uint32_t I1 = (~(J1 ^ S) & 1);
    uint32_t I2 = (~(J2 ^ S) & 1);
    uint32_t offset = (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
    if (S) {
        offset |= 0xFF800000; // знак расширение
    }

    // Вычисление нового смещения
    uint32_t new_offset = (uint32_t)((int32_t)offset + (int32_t)sym_val - (int32_t)addr);
    S = new_offset >> 31;
    I1 = (new_offset >> 23) & 1;
    I2 = (new_offset >> 22) & 1;
    imm10 = (new_offset >> 12) & 0x03FF;
    imm11 = (new_offset >> 1) & 0x07FF;

    J1 = (~(I1 ^ S) & 1);
    J2 = (~(I2 ^ S) & 1);
    //goutf("ov: %ph off: %ph noff: %ph -> %d:%d:%d:%x:%x", *addr, offset, new_offset, S, J1, J2, imm10, imm11);

    // Обновление инструкции
    *addr++ = 0xF000 | (S << 10) | imm10;
    *addr = (0b11010 << 11) | (J1 << 13) | (J2 << 11) | imm11;
}

static const char* st_predef(const char* v) {
    if(strlen(v) == 2) {
        if (v[0] == '$' && v[1] == 't') {
            return "$t (Thumb)";
        }
        if (strcmp(v, "$d") == 0) {
            return "$d (data)";
        }
    }
    return v;
}

typedef struct {
    FIL *f2;
    elf32_header *pehdr;
    int symtab_off;
    elf32_sym* psym;
    char* pstrtab;
    sect_entry_t* psections_list;
    uint16_t max_sections_in_list;
} load_sec_ctx;

static char* sec_prg_addr(load_sec_ctx* ctx, uint16_t sec_num) {
    for (uint16_t i = 0; i < ctx->max_sections_in_list && ctx->psections_list[i].prg_addr != 0; ++i) {
        if (ctx->psections_list[i].sec_num == sec_num) {
            // goutf("section #%d found\n", sec_num);
            return ctx->psections_list[i].prg_addr;
        }
    }
    return 0;
}

static void add_sec(load_sec_ctx* ctx, char* del_addr, char* prg_addr, uint16_t num) {
    uint16_t i = 0;
    for (; i < ctx->max_sections_in_list - 1 && ctx->psections_list[i].del_addr != 0; ++i);
    if (i == ctx->max_sections_in_list - 1) {
        ctx->max_sections_in_list += 2; // may be more? 
        //goutf("max sections count increased up to %d\n", ctx->max_sections_in_list);
        sect_entry_t* sects_list = (sect_entry_t*)pvPortMalloc(ctx->max_sections_in_list * sizeof(sect_entry_t));
        for (uint16_t j = 0; j < ctx->max_sections_in_list - 2; ++j) {
            sects_list[j] = ctx->psections_list[j];
        }
        vPortFree(ctx->psections_list);
        ctx->psections_list = sects_list;
    }
    //goutf("section #%d inserted @ line #%d\n", num, i);
    ctx->psections_list[i].del_addr = del_addr;
    ctx->psections_list[i].prg_addr = prg_addr;
    ctx->psections_list[i++].sec_num = num;
    if (i < ctx->max_sections_in_list) {
        //goutf("next section line #%d\n", i);
        ctx->psections_list[i].del_addr = 0;
    }
}

inline static uint32_t sec_align(uint32_t addr, uint32_t a) {
    if (a == 0 || a == 1) return addr;
    if (a == (1 << 1)) {
        //goutf("%ph alliged to %ph by %d\n", addr, (addr & 1) ? addr + 1 : addr, a);
        return (addr & 1) ? addr + 1 : addr;
    }
    for (uint8_t b = 2; b < 32; b++) {
        if (a == (1 << b)) {
            if (addr & (a - 1)) {
                //goutf("%ph alliged to %ph by %d\n", addr, (addr & (0xFFFFFFFF ^ (a - 1))) + a, a);
                return (addr & (0xFFFFFFFF ^ (a - 1))) + a;
            }
            return addr;
        }
    }
    goutf("Unsupported allignment: %d\n", a);
    return addr;
}

static const char* st_spec_sec(uint16_t st) {
    if (st >= 0xff00 && st <= 0xff1f)
        return "PROC";
    switch (st)
    {
    case 0xffff:
        return "HIRESERVE";
    case 0xfff2:
        return "COMMON";
    case 0xfff1:
        return "ABS";
    case 0:
        return "UNDEF";
    default:
        break;
    }
    return 0;
}

static const char* s = "Unexpected ELF file";

bool is_new_app(char * fn) {
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        goutf("Unable to open file: '%s'\n", fn);
        return false;
    }
    struct elf32_header ehdr;
    UINT rb;
    if (f_read(f, &ehdr, sizeof(ehdr), &rb) != FR_OK) {
        f_close(f);
        vPortFree(f);
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        return false;
    }
    f_close(f);
    vPortFree(f);
    if (ehdr.common.magic != ELF_MAGIC) {
        goutf("It is not an ELF file: '%s'\n", fn);
        return false;
    }
    if (ehdr.common.version != 1 || ehdr.common.version2 != 1) {
        goutf("%s '%s' version: %d:%d\n", s, fn, ehdr.common.version, ehdr.common.version2);
        return false;
    }
    if (ehdr.common.arch_class != 1 || ehdr.common.endianness != 1) {
        goutf("%s '%s' class: %d; endian: %d\n", s, fn, ehdr.common.arch_class, ehdr.common.endianness);
        return false;
    }
    if (ehdr.common.machine != EM_ARM) {
        goutf("%s '%s' machine type: %d; expected: %d\n", s, fn, ehdr.common.machine, EM_ARM);
        return false;
    }
    if (ehdr.common.abi != 0) {
        goutf("%s '%s' ABI type: %d; expected: %d\n", s, fn, ehdr.common.abi, 0);
        return false;
    }
    if (ehdr.flags & EF_ARM_ABI_FLOAT_HARD) {
        goutf("%s '%s' EF_ARM_ABI_FLOAT_HARD: %04Xh\n", s, fn, ehdr.flags);
        return false;
    }
    return true;
}

void cleanup_bootb_ctx(bootb_ctx_t* bootb_ctx) {
    if (bootb_ctx->sect_entries) {
        for (uint16_t i = 0; bootb_ctx->sect_entries[i].del_addr != 0; ++i) {
            // goutf("#%d: [%p]\n", i, bootb_ctx->sect_entries[i]);
            vPortFree(bootb_ctx->sect_entries[i].del_addr);
        }
        // goutf("[%p] end\n", bootb_ctx->sect_entries);
        vPortFree(bootb_ctx->sect_entries);
        bootb_ctx->sect_entries = 0;
    }
}

int run_new_app(char * fn) {
    bootb_ctx_t* bootb_ctx = (bootb_ctx_t*)pvPortMalloc(sizeof(bootb_ctx_t));
    bootb_ctx->sect_entries = 0;
    bootb_ctx->bootb[0] = 0; bootb_ctx->bootb[1] = 0; bootb_ctx->bootb[2] = 0; bootb_ctx->bootb[3] = 0;
    int ret_code = load_app(fn, bootb_ctx);
    if (ret_code == 0) {
        ret_code = exec(bootb_ctx);
    }
    cleanup_bootb_ctx(bootb_ctx);
    vPortFree(bootb_ctx);
    return ret_code;
}

static uint8_t* load_sec2mem(load_sec_ctx * c, uint16_t sec_num) {
    uint8_t* prg_addr = sec_prg_addr(c, sec_num);
    if (prg_addr != 0) {
        //goutf("Section #%d already in mem @ %p\n", sec_num, prg_addr);
        return prg_addr;
    }
    UINT rb;
    char* del_addr = 0;
    elf32_shdr* psh = (elf32_shdr*)pvPortMalloc(sizeof(elf32_shdr));
    if (f_lseek(c->f2, c->pehdr->sh_offset + sizeof(elf32_shdr) * sec_num) == FR_OK &&
        f_read(c->f2, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)
    ) {
        // todo: enough space?
        uint32_t sz = psh->sh_size;
        del_addr = (char*)pvPortMalloc(sz);
        prg_addr = sec_align((uint32_t)del_addr, psh->sh_addralign);
        if (f_lseek(c->f2, psh->sh_offset) == FR_OK &&
            f_read(c->f2, prg_addr, psh->sh_size, &rb) == FR_OK && rb == psh->sh_size
        ) {
             // goutf("Program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, prg_addr);
            add_sec(c, del_addr, prg_addr, sec_num);
            // links and relocations
            if (f_lseek(c->f2, c->pehdr->sh_offset) != FR_OK) {
                goutf("Unable to locate sections @ %ph\n", c->pehdr->sh_offset);
                prg_addr = 0;
                goto e1;
            }
            while (f_read(c->f2, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) {
                // goutf("Section info: %d type: %d\n", sh.sh_info, sh.sh_type);
                if (psh->sh_type == REL_SEC && psh->sh_info == sec_num) {
                    uint32_t r2 = f_tell(c->f2);
                    elf32_rel rel;
                    for (uint32_t j = 0; j < psh->sh_size / sizeof(rel); ++j) {
                        if (f_lseek(c->f2, psh->sh_offset + j * sizeof(rel)) != FR_OK ||
                            f_read(c->f2, &rel, sizeof(rel), &rb) != FR_OK || rb != sizeof(rel)
                        ) {
                            goutf("Unable to read REL record #%d in section #%d\n", j, psh->sh_info);
                            prg_addr = 0;
                            goto e1;
                        }
                        uint32_t rel_sym = rel.rel_info >> 8;
                        uint8_t rel_type = rel.rel_info & 0xFF;
                        // goutf("rel_offset: %p; rel_sym: %d; rel_type: %d\n", rel.rel_offset, rel_sym, rel_type);
                        if (f_lseek(c->f2, c->symtab_off + rel_sym * sizeof(elf32_sym)) != FR_OK ||
                            f_read(c->f2, c->psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)
                        ) {
                            goutf("Unable to read .symtab section #%d\n", rel_sym);
                            prg_addr = 0;
                            goto e1;
                        }
                        char* rel_str_sym = st_spec_sec(c->psym->st_shndx);
                        if (rel_str_sym != 0) {
                            goutf("Unsupported link from STRTAB record #%d to section #%d (%s): %s\n",
                                  rel_sym, c->psym->st_shndx, rel_str_sym,
                                  st_predef(c->pstrtab + c->psym->st_name)
                            );
                            prg_addr = 0;
                            goto e1;
                        }
                        uint32_t* rel_addr = (uint32_t*)(prg_addr + rel.rel_offset /*10*/); /*f7ff fffe 	bl	0*/
                        // DO NOT resolve it for any case, it may be 16-bit alligned, and will hand to load 32-bit
                        //uint32_t P = *rel_addr; /*f7ff fffe 	bl	0*/
                        uint32_t S = c->psym->st_value;
                        char * sec_addr = prg_addr;
                        // goutf("rel_offset: %p; rel_sym: %d; rel_type: %d -> %d\n", rel.rel_offset, rel_sym, rel_type, c->psym->st_shndx);
                        if (c->psym->st_shndx != sec_num) {
                            sec_addr = load_sec2mem(c, c->psym->st_shndx);
                            if (sec_addr == 0) { // TODO: already loaded (app context)
                                prg_addr = 0;
                                goto e1;
                            }
                        }
                        uint32_t A = sec_addr;
                        // Разрешение ссылки
                        switch (rel_type) {
                            case 2: //R_ARM_ABS32:
                                // goutf("rel_type: %d; *rel_addr += A: %ph + S: %ph\n", rel_type, A, S);
                                *rel_addr += S + A;
                                break;
                          //  case 3: //R_ARM_REL32:
                          //      *rel_addr = S - P + A; // todo: signed?
                          //      break;
                            case 10: //R_ARM_THM_PC22:
                                resolve_thm_pc22(rel_addr, A + S);
                                break;
                            default:
                                goutf("Unsupportel REL type: %d\n", rel_type);
                                prg_addr = 0;
                                goto e1;
                        }
                        //goutf("= %ph\n", *rel_addr);
                    }
                    f_lseek(c->f2, r2);
                }
            }
        } else {
            //goutf("Unable to load program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, del_addr);
            prg_addr = 0;
            goto e1;
        }
        //goutf("Section #%d - load completed @%ph\n", sec_num, prg_addr);
    } else {
        goutf("Unable to read section #%d info\n", sec_num);
        prg_addr = 0;
        goto e1;
    }
e1:
    vPortFree(psh);
    if (!prg_addr && del_addr) vPortFree(del_addr);
    return prg_addr;
}

int load_app(char * fn, bootb_ctx_t* bootb_ctx) {
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        goutf("Unable to open file: '%s'\n", fn);
        return -1;
    }
    elf32_header* pehdr = (elf32_header*)pvPortMalloc(sizeof(elf32_header));
    UINT rb;
    if (f_read(f, pehdr, sizeof(elf32_header), &rb) != FR_OK) {
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        goto e1;
    }
    elf32_shdr* psh = (elf32_shdr*)pvPortMalloc(sizeof(elf32_shdr));
    bool ok = f_lseek(f, pehdr->sh_offset + sizeof(elf32_shdr) * pehdr->sh_str_index) == FR_OK;
    if (!ok || f_read(f, psh, sizeof(elf32_shdr), &rb) != FR_OK || rb != sizeof(elf32_shdr)) {
        goutf("Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(f), sizeof(elf32_shdr), rb);
        goto e11;
    }
    char* symtab = (char*)pvPortMalloc(psh->sh_size);
    ok = f_lseek(f, psh->sh_offset) == FR_OK;
    if (!ok || f_read(f, symtab, psh->sh_size, &rb) != FR_OK || rb != psh->sh_size) {
        goutf("Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(f), psh->sh_size, rb);
        goto e2;
    }
    f_lseek(f, pehdr->sh_offset);
    int symtab_off, strtab_off = -1;
    UINT symtab_len, strtab_len = 0;
    while ((symtab_off < 0 || strtab_off < 0) && f_read(f, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) { 
        if(psh->sh_type == 2 && 0 == strcmp(symtab + psh->sh_name, ".symtab")) {
            symtab_off = psh->sh_offset;
            symtab_len = psh->sh_size;
        }
        if(psh->sh_type == 3 && 0 == strcmp(symtab + psh->sh_name, ".strtab")) {
            strtab_off = psh->sh_offset;
            strtab_len = psh->sh_size;
        }
    }
    if (symtab_off < 0 || strtab_off < 0) {
        goutf("Unable to find .strtab/.symtab sections\n");
        goto e2;
    }
    f_lseek(f, strtab_off);
    char* strtab = (char*)pvPortMalloc(strtab_len);
    if (f_read(f, strtab, strtab_len, &rb) != FR_OK || rb != strtab_len) {
        goutf("Unable to read .strtab section\n");
        goto e3;
    }
    f_lseek(f, symtab_off);
    elf32_sym sym;
    uint32_t _init_idx = 0xFFFFFFFF;
    uint32_t _fini_idx = 0xFFFFFFFF;
    uint32_t main_idx = 0xFFFFFFFF;
    uint32_t req_idx = 0xFFFFFFFF;
    // TODO: precalc req. size
    uint16_t max_sects = pehdr->sh_num - 10; // dynamic, initial val (euristic based on ehdr.sh_num)
    bootb_ctx->sect_entries = (sect_entry_t*)pvPortMalloc(max_sects * sizeof(sect_entry_t));
    bootb_ctx->sect_entries[0].del_addr = 0;
    load_sec_ctx* pctx = (load_sec_ctx*)pvPortMalloc(sizeof(load_sec_ctx));
    pctx->f2 = f;
    pctx->pehdr = pehdr;
    pctx->symtab_off = symtab_off;
    pctx->psym = &sym;
    pctx->pstrtab = strtab;
    pctx->psections_list = bootb_ctx->sect_entries;
    pctx->max_sections_in_list = max_sects;
    for (uint32_t i = 0; i < symtab_len / sizeof(sym); ++i) {
        if (f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)) {
            goutf("Unable to read .symtab section #%d\n", i);
            break;
        }
        if (sym.st_info == STR_TAB_GLOBAL_FUNC) {
            if (0 == strcmp("_init", strtab + sym.st_name)) {
                _init_idx = i;
            }
            if (0 == strcmp("__required_m_api_verion", strtab + sym.st_name)) {
                req_idx = i;
            }
            else if (0 == strcmp("_fini", strtab + sym.st_name)) {
                _fini_idx = i;
            }
            else if (0 == strcmp("main", strtab + sym.st_name)) {
                main_idx = i;
            }
        }
    }
    if (req_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + req_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for __required_m_api_version #%d\n", req_idx);
            goto e3;
        }
        bootb_ctx->bootb[0] = load_sec2mem(pctx, sym.st_shndx) + 1;
    }
    if (_init_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + _init_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for _init #%d\n", _init_idx);
            goto e3;
        }
        bootb_ctx->bootb[1] = load_sec2mem(pctx, sym.st_shndx) + 1;
    }
    if (main_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + main_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for main #%d\n", main_idx);
            goto e3;
        }
        bootb_ctx->bootb[2] = load_sec2mem(pctx, sym.st_shndx) + 1;
    }
    if (_fini_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + _fini_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for _fini #%d\n", _fini_idx);
            goto e3;
        }
        bootb_ctx->bootb[3] = load_sec2mem(pctx, sym.st_shndx) + 1;
    }
    /*
    if (bootb_ctx->sect_entries) {
        for (uint16_t i = 0; bootb_ctx->sect_entries[i].del_addr != 0; ++i) {
            goutf("sec #%d: [%p][%p] %d\n", i, bootb_ctx->sect_entries[i].del_addr,bootb_ctx->sect_entries[i].prg_addr,bootb_ctx->sect_entries[i].sec_num);
        }
    }
    goutf("[%p][%p][%p][%p]\n", bootb_ctx->bootb[0], bootb_ctx->bootb[1], bootb_ctx->bootb[2], bootb_ctx->bootb[3]);
    */
e3:
    vPortFree(strtab);
e2:
    vPortFree(symtab);
e11:
    vPortFree(psh);
e1:
    vPortFree(pehdr);
    f_close(f);
    vPortFree(f);
    bootb_ctx->sect_entries = pctx->psections_list;
    vPortFree(pctx);
    if (bootb_ctx->bootb[2] == 0) {
        goutf("'main' global function is not found in the '%s' elf-file\n", fn);
        return -1;
    }
}

int exec(bootb_ctx_t* bootb_ctx) {
    if (bootb_ctx->bootb[0]) {
        int rav = bootb_ctx->bootb[0]();
        if (rav > M_API_VERSION) {
            goutf("Required M-API version %d is grater than provided M-API version %d\n", rav, M_API_VERSION);
            return -2;
        }
    }
    if (bootb_ctx->bootb[1]) {
        bootb_ctx->bootb[1](); // tood: ensure stack
    }
    int res = bootb_ctx->bootb[2] ? bootb_ctx->bootb[2]() : -3;
    if (bootb_ctx->bootb[3]) {
        bootb_ctx->bootb[3]();
    }
    return res;
}
