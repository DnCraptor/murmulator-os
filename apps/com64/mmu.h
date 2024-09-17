//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mmu_class.h                           //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 23.03.2022                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MMU_H
#define MMU_H

#include "stdbool.h"
#include "mos6510_port.h"
#include "func.h"

typedef struct MMU
{
    func_read_t  CPUReadProcTbl [256];
    func_write_t CPUWriteProcTbl[256];

    MOS6510_PORT *CPU_PORT;

    bool *GAME;
    bool *EXROM;
    bool *RAM_H;
    bool *RAM_L;

    unsigned char MEMORY_MAP;
    unsigned char MEMORY_MAP_OLD;

    bool          *EasyFlashDirty1;
    unsigned char *EasyFlashByte1;
    bool          *EasyFlashDirty2;
    unsigned char *EasyFlashByte2;

    /// Variablen ///
    unsigned char RAM[0x10000];			// 64KB
///    unsigned char BASIC_ROM[0x2000];		// 8KB ab 0xA000
    unsigned char CHAR_ROM[0x1000];		// 4KB ab 0xD000
    unsigned char FARB_RAM[0x0400];		// 1KB ab 0xD800
///    unsigned char KERNAL_ROM[0x2000];		// 8KB ab 0xE000

    /// AktMemoryMap Visuall ///
    unsigned char MapReadSource[0x100];
    unsigned char MapWriteDestination[0x100];
} MMU;

static void MMU_MMU(MMU* p, MOS6510_PORT* port);
static void MMU_Reset(MMU* p);
static void MMU_InitProcTables(MMU* p);
static unsigned char MMU_ReadZeroPage(MMU* p, unsigned short adresse);
static void MMU_WriteZeroPage(MMU* p, unsigned short adresse, unsigned char wert);
static void MMU_ChangeMemMap(MMU* p);
static unsigned char MMU_ReadRam(MMU* p, unsigned short adresse);
static void MMU_WriteRam(MMU* p, unsigned short adresse, unsigned char wert);
static unsigned char MMU_ReadFarbRam(MMU* p, unsigned short adresse);
static void MMU_WriteFarbRam(MMU* p, unsigned short adresse, unsigned char wert);
static unsigned char MMU_ReadBasicRom(MMU* p, unsigned short adresse);
static unsigned char MMU_ReadKernalRom(MMU* p, unsigned short adresse);
static void MMU_InitRam(
    MMU* p,
    uint8_t init_value,
    uint16_t invert_value_every,
    uint16_t random_pattern_legth,
    uint16_t repeat_random_pattern,
    uint16_t random_chance
);

#endif // MMU_H
