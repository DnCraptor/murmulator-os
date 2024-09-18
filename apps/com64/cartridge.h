//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: cartridge_class.h                     //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "structs.h"
#include "am29f040.h"
#include "mk7pla.h"

typedef struct Cartridge
{
    bool        *exrom;
    bool        *game;

    uint8_t     *lo_rom;                      // 0x8000
    uint8_t     *hi_rom;                      // 0xA000 oder 0xE000

    func_vv_t ChangeMemMapProc;
    func_vi_t CpuTriggerInterrupt;
    func_vi_t CpuClearInterrupt;
    func_vib_t ChangeLED;


///    uint8_t     rom_bank1[64*0x2000];         // Alle ChipRoms für 0x8000	(max 64 x 0x2000)
///    uint8_t     rom_bank2[64*0x2000];         // Alle ChipRoms für 0xA000 oder 0xE000 (max 64 x 0x2000)
uint8_t* rom_bank1;
uint8_t* rom_bank2; /// TODO: ^^ 

///    uint8_t     rom_bank1_tmp[64*0x2000];     // Alle ChipRoms für 0x8000	(max 64 x 0x2000)
///    uint8_t     rom_bank2_tmp[64*0x2000];     // Alle ChipRoms für 0xA000 oder 0xE000 (max 64 x 0x2000)

    uint8_t     *c64_ram;                     // Kompletter C64 RAM

    bool        led_00;                 // LED FCIII
    bool        led_00_old;
    bool        led_01;                 // LED EF
    bool        led_01_old;

    bool        cartridge_exrom;
    bool        cartridge_game;

    bool        cartridge_is_insert;
    uint16_t    cartridge_type;

    //// EASY FLASH (32)
    bool            easyflash_jumper;
    uint8_t         easyflash_ram[256];		// Speziller Speicher für das EasyFlash Cartridge
    uint8_t         easyflash_bank_reg;		// Bank Register
    AM29F040*       am29f040Hi;
    AM29F040*       am29f040Lo;

    uint32_t    rom_lo_bank;

    //// Action Replay 4/5/6 ////
    uint8_t     ar_reg;                     // Inhalt des zuletzt geschriebenen $DExx Wert
    bool        ar_freez;
    bool        ar_active;
    bool        ar_enable_ram;
    uint8_t     ar_ram[0x2000];             // 8KB
    uint8_t     pla_address;
} Cartridge;

inline static void Cartridge_ResetAllLEDS(Cartridge* p) {
    p->led_00 = p->led_00_old = p->led_01 = p->led_01_old = false;
    if(p->ChangeLED.p != nullptr) p->ChangeLED.fn(p->ChangeLED.p, 0, p->led_00);
    if(p->ChangeLED.p != nullptr) p->ChangeLED.fn(p->ChangeLED.p, 1, p->led_01);
}
inline static void Cartridge_Cartridge(Cartridge* p)
{
    p->easyflash_jumper = false;	// false = Boot
    p->am29f040Lo = calloc(1, sizeof(AM29F040));
    AM29F040_AM29F040(p->am29f040Lo, p->rom_bank1, 1);
    p->am29f040Hi = calloc(1, sizeof(AM29F040));
    AM29F040_AM29F040(p->am29f040Hi, p->rom_bank1, 2);
    Cartridge_ResetAllLEDS(p);
    p->ar_freez = false;
}

#endif // CARTRIDGE_H
