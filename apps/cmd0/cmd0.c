#include "m-os-api.h"

inline static void concat (char* t, const char* s1 , const char* s2) {
    size_t s = strlen(s1);
    strncpy(t, s1, 511);
    t[s] = '/';
    strncpy(t + s + 1, s2, 510 - s);
}

inline static bool exists(cmd_startup_ctx_t* ctx, char* t) {
    char * cmd = ctx->cmd_t;
    FILINFO fileinfo;
    bool r = f_stat(cmd, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
    if (r) {
        strncpy(t, cmd, 511);
        return r;
    }
    char* dir = ctx->curr_dir;
    concat(t, dir, cmd);
    r = f_stat(t, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
    if (r) return r;
    dir = ctx->path;
    while (dir) {
        concat(t, dir, cmd);
        r = f_stat(t, &fileinfo) == FR_OK && !(fileinfo.fattrib & AM_DIR);
        if (r) return r;
        dir = next_token(dir);
    }
    return false;
}

inline static void cmd_backspace(cmd_startup_ctx_t* ctx) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos == 0) {
        // TODO: blimp
        return;
    }
    ctx->cmd[--cmd_pos] = 0;
    gbackspace();
}

inline static void type_char(cmd_startup_ctx_t* ctx, char c) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
        return;
    }
    putc(c);
    ctx->cmd[cmd_pos++] = c;
    ctx->cmd[cmd_pos] = 0;
}

inline static char tolower_token(char t) {
    if (t == ' ') {
        return 0;
    }
    if (t >= 'A' && t <= 'Z') {
        return t + ('a' - 'A');
    }
    if (t >= 0x80 && t <= 0x8F /* А-П */) {
        return t + (0xA0 - 0x80);
    }
    if (t >= 0x90 && t <= 0x9F /* Р-Я */) {
        return t + (0xE0 - 0x90);
    }
    if (t >= 0xF0 && t <= 0xF6) return t + 1; // Ё...
    return t;
}

inline static int tokenize_cmd(cmd_startup_ctx_t* ctx) {
    if (ctx->cmd[0] == 0) {
        ctx->cmd_t[0] = 0;
        return 0;
    }
    bool inSpace = true;
    int inTokenN = 0;
    char* t1 = ctx->cmd;
    char* t2 = ctx->cmd_t;
    while(*t1) {
        char c = tolower_token(*t1++);
        if (inSpace) {
            //if (!c) {} // still in space
            if(c) { // token started
                inSpace = 0;
                inTokenN++; // new token
            }
        } else if(!c) { inSpace = true; } // not in space, after the token
        //else {} // still in token
        *t2++ = c;
    }
    *t2 = 0;
    return inTokenN;
}

inline static void cd(cmd_startup_ctx_t* ctx, char *d) {
    FILINFO fileinfo;
    if (strcmp(d, "\\") == 0 || strcmp(d, "/") == 0) {
        strcpy(ctx->curr_dir, d);
    } else if (f_stat(d, &fileinfo) != FR_OK || !(fileinfo.fattrib & AM_DIR)) {
        goutf("Unable to find directory: '%s'\n", d);
    } else {
        strcpy(ctx->curr_dir, d);
    }
}

inline static void cmd_push(cmd_startup_ctx_t* ctx, char c) {
    size_t cmd_pos = strlen(ctx->cmd);
    if (cmd_pos >= 512) {
        // TODO: blimp
    }
    ctx->cmd[cmd_pos++] = c;
    ctx->cmd[cmd_pos] = 0;
    putc(c);
}

#define STR_TAB_FUNC 2
#define STR_TAB_GLOBAL 1
#define STR_TAB_GLOBAL_FUNC ((STR_TAB_GLOBAL << 4) | STR_TAB_FUNC)
#define REL_SEC 9

#pragma pack(push, 1)
struct elf_header {
    uint32_t    magic;
    uint8_t     arch_class;
    uint8_t     endianness;
    uint8_t     version;
    uint8_t     abi;
    uint8_t     abi_version;
    uint8_t     _pad[7];
    uint16_t    type;
    uint16_t    machine;
    uint32_t    version2;
};

typedef struct elf32_header {
    struct elf_header common;
    uint32_t    entry;
    uint32_t    ph_offset;
    uint32_t    sh_offset;
    uint32_t    flags;
    uint16_t    eh_size;
    uint16_t    ph_entry_size;
    uint16_t    ph_num;
    uint16_t    sh_entry_size;
    uint16_t    sh_num;
    uint16_t    sh_str_index;
} elf32_header;

struct elf32_ph_entry {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filez;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

typedef struct {
  uint32_t	sh_name;
  uint32_t	sh_type;
  uint32_t	sh_flags;
  uint32_t	sh_addr;
  uint32_t	sh_offset;
  uint32_t	sh_size;
  uint32_t	sh_link;
  uint32_t	sh_info;
  uint32_t	sh_addralign;
  uint32_t	sh_entsize;
} elf32_shdr;

typedef struct {
  uint32_t	st_name;
  uint32_t	st_value;
  uint32_t	st_size;
  unsigned char	st_info;
  unsigned char	st_other;
  uint16_t	st_shndx;
} elf32_sym;

typedef struct {
  uint32_t rel_offset;
  uint32_t rel_info;
} elf32_rel;

typedef struct {
  uint32_t rela_offset;
  uint32_t rela_info;
  int32_t rela_addend;
} elf32_rela;

#pragma pack(pop)
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
    goutf("ov: %ph off: %ph noff: %ph -> %d:%d:%d:%x:%x", *addr, offset, new_offset, S, J1, J2, imm10, imm11);

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
    const elf32_header *pehdr;
    const int symtab_off;
    const elf32_sym* psym;
    const char* pstrtab;
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

static uint8_t* load_sec2mem(load_sec_ctx * c, uint16_t sec_num) {
    uint8_t* prg_addr = sec_prg_addr(c, sec_num);
    if (prg_addr != 0) {
        goutf("Section #%d already in mem @ %p\n", sec_num, prg_addr);
        return prg_addr;
    }
    UINT rb;
    elf32_shdr sh;
    if (f_lseek(c->f2, c->pehdr->sh_offset + sizeof(elf32_shdr) * sec_num) == FR_OK &&
        f_read(c->f2, &sh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)
    ) {
        // todo: enough space?
        uint32_t sz = sh.sh_size;
        char* del_addr = (char*)pvPortMalloc(sz);
        prg_addr = sec_align((uint32_t)del_addr, sh.sh_addralign);
        if (f_lseek(c->f2, sh.sh_offset) == FR_OK &&
            f_read(c->f2, prg_addr, sh.sh_size, &rb) == FR_OK && rb == sh.sh_size
        ) {
            goutf("Program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, prg_addr);
            add_sec(c, del_addr, prg_addr, sec_num);
            // links and relocations
            if (f_lseek(c->f2, c->pehdr->sh_offset) != FR_OK) {
                goutf("Unable to locate sections @ %ph\n", c->pehdr->sh_offset);
                return 0;
            }
            while (f_read(c->f2, &sh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) {
                // goutf("Section info: %d type: %d\n", sh.sh_info, sh.sh_type);
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
                        // goutf("rel_offset: %p; rel_sym: %d; rel_type: %d\n", rel.rel_offset, rel_sym, rel_type);
                        if (f_lseek(c->f2, c->symtab_off + rel_sym * sizeof(elf32_sym)) != FR_OK ||
                            f_read(c->f2, c->psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)
                        ) {
                            goutf("Unable to read .symtab section #%d\n", rel_sym);
                            return 0;
                        }
                        char* rel_str_sym = st_spec_sec(c->psym->st_shndx);
                        if (rel_str_sym != 0) {
                            goutf("Unsupported link from STRTAB record #%d to section #%d (%s): %s\n",
                                  rel_sym, c->psym->st_shndx, rel_str_sym,
                                  st_predef(c->pstrtab + c->psym->st_name)
                            );
                            return 0;
                        }
                        uint32_t* rel_addr = (uint32_t*)(prg_addr + rel.rel_offset /*10*/); /*f7ff fffe 	bl	0*/
                        // DO NOT resolve it for any case, it may be 16-bit alligned, and will hand to load 32-bit
                        //uint32_t P = *rel_addr; /*f7ff fffe 	bl	0*/
                        uint32_t S = c->psym->st_value;
                        char * sec_addr = prg_addr;
                        goutf("rel_offset: %p; rel_sym: %d; rel_type: %d -> %d\n", rel.rel_offset, rel_sym, rel_type, c->psym->st_shndx);
                        if (c->psym->st_shndx != sec_num) {
                            sec_addr = load_sec2mem(c, c->psym->st_shndx);
                            if (sec_addr == 0) { // TODO: already loaded (app context)
                                return 0;
                            }
                        }
                        uint32_t A = sec_addr;
                        // Разрешение ссылки
                        switch (rel_type) {
                            case 2: //R_ARM_ABS32:
                                goutf("rel_type: %d; *rel_addr += A: %ph + S: %ph\n", rel_type, A, S);
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
                                return 0;
                        }
                        //goutf("= %ph\n", *rel_addr);
                    }
                    f_lseek(c->f2, r2);
                }
            }
        } else {
            vPortFree(del_addr);
            goutf("Unable to load program section #%d (%d bytes) allocated into %ph\n", sec_num, sz, del_addr);
            return 0;
        }
        goutf("Section #%d - load completed @%ph\n", sec_num, prg_addr);
    } else {
        goutf("Unable to read section #%d info\n", sec_num);
        return 0;
    }
    return prg_addr;
}

int load_app2(char * fn, bootb_ctx_t* bootb_ctx) {
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        goutf("Unable to open file: '%s'\n", fn);
        return -1;
    }
    struct elf32_header ehdr;
    UINT rb;
    if (f_read(f, &ehdr, sizeof(ehdr), &rb) != FR_OK) {
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        goto e1;
    }
    elf32_shdr sh;
    bool ok = f_lseek(f, ehdr.sh_offset + sizeof(sh) * ehdr.sh_str_index) == FR_OK;
    if (!ok || f_read(f, &sh, sizeof(sh), &rb) != FR_OK || rb != sizeof(sh)) {
        goutf("Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(f), sizeof(sh), rb);
        goto e1;
    }
    char* symtab = (char*)pvPortMalloc(sh.sh_size);
    ok = f_lseek(f, sh.sh_offset) == FR_OK;
    if (!ok || f_read(f, symtab, sh.sh_size, &rb) != FR_OK || rb != sh.sh_size) {
        goutf("Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(f), sh.sh_size, rb);
        goto e2;
    }
    f_lseek(f, ehdr.sh_offset);
    int symtab_off, strtab_off = -1;
    UINT symtab_len, strtab_len = 0;
    while ((symtab_off < 0 || strtab_off < 0) && f_read(f, &sh, sizeof(sh), &rb) == FR_OK && rb == sizeof(sh)) { 
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
    uint16_t max_sects = ehdr.sh_num - 10; // dynamic, initial val (euristic based on ehdr.sh_num)
    bootb_ctx->sect_entries = (sect_entry_t*)pvPortMalloc(max_sects * sizeof(sect_entry_t));
    bootb_ctx->sect_entries[0].del_addr = 0;
    load_sec_ctx ctx = {
        f,
        &ehdr,
        symtab_off,
        &sym,
        strtab,
        bootb_ctx->sect_entries,
        max_sects
    };
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
        bootb_ctx->bootb[0] = load_sec2mem(&ctx, sym.st_shndx) + 1;
    }
    if (_init_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + _init_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for _init #%d\n", _init_idx);
            goto e3;
        }
        bootb_ctx->bootb[1] = load_sec2mem(&ctx, sym.st_shndx) + 1;
    }
    if (main_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + main_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for main #%d\n", main_idx);
            goto e3;
        }
        bootb_ctx->bootb[2] = load_sec2mem(&ctx, sym.st_shndx) + 1;
    }
    if (_fini_idx != 0xFFFFFFFF) {
        if (f_lseek(f, symtab_off + _fini_idx * sizeof(sym)) != FR_OK ||
            f_read(f, &sym, sizeof(sym), &rb) != FR_OK || rb != sizeof(sym)
        ) {
            goutf("Unable to read .symtab section for _fini #%d\n", _fini_idx);
            goto e3;
        }
        bootb_ctx->bootb[3] = load_sec2mem(&ctx, sym.st_shndx) + 1;
    }
    
    if (bootb_ctx->sect_entries) {
        for (uint16_t i = 0; bootb_ctx->sect_entries[i].del_addr != 0; ++i) {
            goutf("sec #%d: [%p][%p] %d\n", i, bootb_ctx->sect_entries[i].del_addr,bootb_ctx->sect_entries[i].prg_addr,bootb_ctx->sect_entries[i].sec_num);
        }
    }
    goutf("[%p][%p][%p][%p]\n", bootb_ctx->bootb[0], bootb_ctx->bootb[1], bootb_ctx->bootb[2], bootb_ctx->bootb[3]);

e3:
    vPortFree(strtab);
e2:
    vPortFree(symtab);
e1:
    f_close(f);
    vPortFree(f);
    bootb_ctx->sect_entries = ctx.psections_list;
    if (bootb_ctx->bootb[2] == 0) {
        goutf("'main' global function is not found in the '%s' elf-file\n", fn);
        return -1;
    }
}

int run_new_app2(char * fn) {
    bootb_ctx_t* bootb_ctx = (bootb_ctx_t*)pvPortMalloc(sizeof(bootb_ctx_t));
    bootb_ctx->sect_entries = 0;
    bootb_ctx->bootb[0] = 0; bootb_ctx->bootb[1] = 0; bootb_ctx->bootb[2] = 0; bootb_ctx->bootb[3] = 0;
    goutf("loading: %s\n", fn);
    int res = load_app2(fn, bootb_ctx);
    
    if (bootb_ctx->sect_entries) {
        for (uint16_t i = 0; bootb_ctx->sect_entries[i].del_addr != 0; ++i) {
            goutf("sec #%d: [%p][%p] %d\n", i, bootb_ctx->sect_entries[i].del_addr,bootb_ctx->sect_entries[i].prg_addr,bootb_ctx->sect_entries[i].sec_num);
        }
    }
    goutf("res: %d; [%p][%p][%p][%p]\n", res, bootb_ctx->bootb[0], bootb_ctx->bootb[1], bootb_ctx->bootb[2], bootb_ctx->bootb[3]);
    
    if (res < 0) {
        cleanup_bootb_ctx(bootb_ctx);
        vPortFree(bootb_ctx);
        return res;
    }
    res = exec(bootb_ctx);
    cleanup_bootb_ctx(bootb_ctx);
    vPortFree(bootb_ctx);
    return res;
}

inline static bool cmd_enter(cmd_startup_ctx_t* ctx) {
    UINT br;
    putc('\n');
    size_t cmd_pos = strlen(ctx->cmd);
    if (!cmd_pos) {
        goto r;
    }
    int tokens = tokenize_cmd(ctx);
    ctx->tokens = tokens;
    if (strcmp("exit", ctx->cmd_t) == 0) { // do not extern, due to internal cmd state
        return true;
    }
    if (strcmp("cd", ctx->cmd_t) == 0) { // do not extern, due to internal cmd state
        if (tokens == 1) {
            fgoutf(ctx->pstderr, "Unable to change directoy to nothing\n");
        } else {
            cd(ctx, (char*)ctx->cmd + (next_token(ctx->cmd_t) - ctx->cmd_t));
        }
    } else {
        char* t = (char*)pvPortMalloc(512); // TODO: optimize
        if (exists(ctx, t)) {
            int len = strlen(t);
            if (len > 3 && strcmp(t, ".uf2") == 0 && load_firmware(t)) {
                run_app(t);
            } else if(is_new_app(t)) {
                ctx->ret_code = run_new_app2(t);
            } else {
                goutf("Unable to execute command: '%s'\n", t);
            }
        } else {
            goutf("Illegal command: '%s'\n", ctx->cmd);
        }
        vPortFree(t);
    }
r:
    goutf("%s>", ctx->curr_dir);
    ctx->cmd[0] = 0;
    return false;
}

int main(void) {
    cmd_startup_ctx_t* ctx = get_cmd_startup_ctx();
    char* curr_dir = ctx->curr_dir;
    goutf("%s>", curr_dir);
    while(1) {
        char c = getc();
        if (c) {
            if (c == 8) {}
            else if (c == 17) {}
            else if (c == 18) {}
            else if (c == '\t') {}
            else if (c == '\n') {
                if ( cmd_enter(ctx) )
                    return 0;
            } else cmd_push(ctx, c);
        }
    }
    __unreachable();
}

int __required_m_api_verion(void) {
    return 2;
}
