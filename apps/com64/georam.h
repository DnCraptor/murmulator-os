//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: georam_class.h                        //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef GEORAM_H
#define GEORAM_H

#include <inttypes.h>

// 512KiB, 1MiB, 2MiB, 4MiB

enum GEO_RAM_MODES { _512KiB, _1024KiB, _2048KiB, _4096KiB };
enum GEO_RAM_TYPE  { _GEORAM, _NEORAM };

#define MAX_GEORAM_SIZE 4194304

const uint32_t geo_ram_size_tbl[] = { 524288, 1048576, 2097152, 4194304 };

typedef struct GEORAM
{
    bool    geo_ram_insert;
    // $DFFE   $DFFF
///    uint8_t ram[MAX_GEORAM_SIZE];   // 64   x  256  x  256 = 4MiB // 4194304
    uint8_t* ram; /// TODO: ^^^

    uint32_t mem_frame;
    uint8_t  _dffe, _dfff;
    uint16_t reg_start_address;
    uint8_t  geo_ram_mode;
} GEORAM;

inline static void GEORAM_GEORAM(GEORAM* p) {
    p->reg_start_address = 0xdf80;  // original GeoRAM
    p->geo_ram_mode = _512KiB;
}

#endif // GEORAM_H
