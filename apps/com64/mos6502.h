//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6502_class.h                       //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 17.09.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS6502_H
#define MOS6502_H

#include "structs.h"
#include "func.h"

typedef struct MOS6502
{
    func_read_t  *ReadProcTbl;
    func_write_t *WriteProcTbl;
    bool *RESET;
    bool *ResetReady;               // Wird bei einem Reset False und beim erreichen einer
                                    // Adresse wird es True
    unsigned short  ResetReadyAdr;
    unsigned char  *MCT;	// Zeigt immer an die aktuelle stelle in derMicro Code Tabel
    unsigned char   Pointer;
    unsigned short  Adresse;
    unsigned short  BranchAdresse;
    unsigned short  AktOpcode;
    unsigned short  AktOpcodePC;
    bool            JAMFlag;
    unsigned char   TMPByte;

    unsigned short  PC;
    unsigned char   AC;
    unsigned char   XR;
    unsigned char   YR;
    unsigned char   SP;
    unsigned char   SR;

    unsigned char   irq_state;          // if Größer 0 ist die Leitung low
    bool            irq_is_low_pegel;
    bool            irq_is_active;

    bool            irq_delay;
    uint8_t         irq_delay_sr_value;

    /// Für OneZyklus ///

    unsigned int tmp,tmp1;
    unsigned short tmp2;
    unsigned short src;
    bool carry_tmp;
    unsigned char idxReg;
} MOS6502;

#define SetAdresseLo(wert) Adresse = ((Adresse&0xFF00)|wert)
#define SetAdresseHi(wert) Adresse = ((Adresse&0x00FF)|(wert<<8))

#define SetPCLo(wert) PC=((PC&0xFF00)|wert)
#define SetPCHi(wert) PC=((PC&0x00FF)|(wert<<8))

#define GetPCLo ((unsigned char)p->PC)
#define GetPCHi ((unsigned char)(p->PC>>8))

static void MOS6502_MOS6502(MOS6502* p, MMU* mmu);
static void MOS6502_Reset(MOS6502* p);
static void MOS6502_TriggerInterrupt(MOS6502* p, int typ);
static void MOS6502_ClearInterrupt(MOS6502* p, int typ);
inline static void MOS6502_ClearJAMFlag(MOS6502* p)
{
    p->JAMFlag = false;
}
static void MOS6502_SetRegister(MOS6502* p, REG_STRUCT *reg);
static void MOS6502_GetRegister(MOS6502* p, REG_STRUCT *reg);
inline unsigned char MOS6502_Read(MOS6502* p, unsigned short adresse)
{
    func_read_t* fn = &p->ReadProcTbl[adresse >> 8];
    unsigned char wert = fn->fn == 0 ? 0 : fn->fn(fn->p, adresse);
}

#endif // MOS6502_H
