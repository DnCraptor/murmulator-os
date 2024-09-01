#include "m-os-api.h"

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

static FIL file;
static FILINFO fi;

#define FLASH_SECTOR_SIZE (1u << 12)
#define XIP_BASE (0x10000000)

inline static uint32_t read_flash_block(FIL * f, uint8_t * buffer, uint32_t expected_flash_target_offset, UF2_Block_t* puf2, size_t* psz) {
    *psz = 0;
    UINT bytes_read = 0;
    uint32_t data_sector_index = 0;
    for(; data_sector_index < FLASH_SECTOR_SIZE; data_sector_index += 256) {
        //fgoutf(get_stdout(), "Read block: %ph; ", f_tell(f));
        f_read(f, puf2, sizeof(UF2_Block_t), &bytes_read);
        *psz += bytes_read;
        //fgoutf(get_stdout(), "(%d bytes) ", bytes_read);
        if (!bytes_read) {
            break;
        }
        if (expected_flash_target_offset != puf2->targetAddr - XIP_BASE) {
            f_lseek(f, f_tell(f) - sizeof(UF2_Block_t)); // we will reread this block, it doesnt belong to this continues block
            expected_flash_target_offset = puf2->targetAddr - XIP_BASE;
            *psz -= bytes_read;
            //fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
            break;
        }
        //fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
        memcpy(buffer + data_sector_index, puf2->data, 256);
        expected_flash_target_offset += 256;
    }
    return expected_flash_target_offset;
}

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc != 2) {
        fprintf(ctx->std_err, "Impossimle to update OS by absent filename.");
    }
    if (FR_OK != f_stat(ctx->argv[1], &fi) || FR_OK != f_open(&file, ctx->argv[1], FA_READ)) {
        fprintf(ctx->std_err, "Unable to open `%s` file.", ctx->argv[1]);
        return 1;
    }
    UF2_Block_t* uf2 = (UF2_Block_t*)pvPortMalloc(sizeof(UF2_Block_t));
    char* alloc = (char*)pvPortMalloc(FLASH_SECTOR_SIZE << 1);
    char* buffer = (char*)((uint32_t)(alloc + FLASH_SECTOR_SIZE - 1) & 0xFFFFFE00); // align 512
    uint32_t flash_target_offset = 0;
    bool boot_replaced = false;
    uint32_t expected_to_write_size = fi.fsize >> 1;
    uint32_t already_written = 0;
    while(true) {
        size_t sz = 0;
        uint32_t next_flash_target_offset = read_flash_block(&file, buffer, flash_target_offset, uf2, &sz);
        if (next_flash_target_offset == flash_target_offset) {
            break;
        }
        if (sz == 0) { // replace target
            flash_target_offset = next_flash_target_offset; 
            printf("Replace targe offset: %ph\n", flash_target_offset);
            continue;
        }
        //подмена загрузчика boot2 прошивки на записанный ранее
        if (flash_target_offset == 0) {
            boot_replaced = true;
            memcpy(buffer, (uint8_t *)XIP_BASE, 256);
            printf("Replace loader @ offset 0\n");
        }
        already_written += FLASH_SECTOR_SIZE;
        printf("Erase and write to flash, offset: %ph (%d%%)\n", flash_target_offset, already_written * 100 / expected_to_write_size);
        flash_block(buffer, flash_target_offset);
        flash_target_offset = next_flash_target_offset;
    }
    vPortFree(alloc);
    f_close(&file);
    vPortFree(uf2);
    printf("Done.\n Reboot is required...");
    return 0;
}
