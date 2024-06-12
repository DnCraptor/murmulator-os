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

static const char* s = "Unexpected ELF file";

void run_new_app(char * fn) {
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
    goutf("Size of section headers:           %d\n", ehdr.sh_entry_size);
    goutf("Number of section headers:         %d\n", ehdr.sh_num);
    goutf("Section header string table index: %d\n", ehdr.sh_str_index);
    goutf("Start of section headers:          %d\n", ehdr.sh_offset);
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
    while (symtab_off < 0 && strtab_off < 0 && f_read(&f2, &sh, sizeof(sh), &rb) == FR_OK && rb == sizeof(sh)) { 
        if(sh.sh_type == 2 && 0 == strcmp(symtab + sh.sh_name, ".symtab")) {
            symtab_off = sh.sh_offset;
            symtab_len = sh.sh_size;
        }
        if(sh.sh_type == 3 && 0 == strcmp(symtab + sh.sh_name, ".strtab")) {
            strtab_off = sh.sh_offset;
            strtab_len = sh.sh_size;
        }
    }
    if (symtab_off >= 0 && strtab_off >= 0) {
        goutf("Unable to find .strtab/.symtab sections\n");
        goto e2;
    }
e2:
    vPortFree(symtab);
e1:
    f_close(&f2);
}
