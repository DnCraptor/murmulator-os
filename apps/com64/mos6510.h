//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6510_class.h                       //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 15.09.2022                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS_6510_CLASS_H
#define MOS_6510_CLASS_H

#include "func.h"
#include "structs.h"

#define DEBUG_CART_ADRESS 0xD7FF

typedef struct MOS6510
{
    func_read_t  *ReadProcTbl;
    func_write_t *WriteProcTbl;
    // Signale von Außen
    bool *RDY;
    bool *RESET;

    bool EnableExtInterrupts;

    bool *ResetReady;	// Wird bei einem Reset False und beim erreichen einer
						// Adresse wird es True
    unsigned short ResetReadyAdr;
    bool JAMFlag;

    unsigned char  *MCT;		// Zeigt immer an die aktuelle stelle in derMicro Code Tabel
    unsigned char   Pointer;
    unsigned short  Adresse;
    unsigned short  BranchAdresse;
    unsigned short  AktOpcode;
    unsigned short  AktOpcodePC;
    bool            CpuWait;
    unsigned char   TMPByte;

    unsigned short  PC;
    unsigned char   AC;
    unsigned char   XR;
    unsigned char   YR;
    unsigned char   SP;
    unsigned char   SR;
    bool            Interrupts[IntQuellenC64];

    unsigned char   irq_state;          // if Größer 0 ist die Leitung low
    bool            irq_is_low_pegel;
    bool            irq_is_active;

    bool            irq_delay;
    uint8_t         irq_delay_sr_value;

    bool            nmi_state;
    bool            nmi_state_old;
    bool            nmi_fall_edge;
    bool            nmi_is_active;

    bool            EnableDebugCart;
    unsigned char   DebugCartValue;

    bool            shxy_dma;
    uint8_t         axa_byte;

    // ??
    bool WRITE_FF00;
    bool WRITE_DEBUG_CART;
} MOS6510;

static void MOS6510_MOS6510(MOS6510* p);
inline static unsigned char MOS6510_Read(MOS6510* p, unsigned short adresse)
{
    func_read_t* fn = &p->ReadProcTbl[(adresse)>>8];
    if (!fn->p) return 0;
	return fn->fn(fn->p, adresse);
}
static void MOS6510_Write(MOS6510* p, unsigned short adresse, unsigned char wert);
static bool MOS6510_OneZyklus(MOS6510* p);
static void MOS6510_Phi1(MOS6510* p);
inline static void MOS6510_SET_CARRY(MOS6510* p, unsigned char status)
{
	if (status != 0) p->SR |= 1;
	else p->SR &= 0xFE;
}
inline static void MOS6510_SET_SR_NZ(MOS6510* p, unsigned char wert)
{
	p->SR = (p->SR&0x7D)|(wert&0x80);
	if (wert==0) p->SR|=2;
}
inline static void MOS6510_SET_SIGN(MOS6510* p, unsigned char wert)
{
	p->SR = (p->SR&127)|(wert&0x80);
}
inline static void MOS6510_SET_ZERO(MOS6510* p, unsigned char wert)
{
	if(wert==0) p->SR|=2;
	else p->SR&=0xFD;
}
inline static bool MOS6510_IF_CARRY(MOS6510* p)
{
	if(p->SR&1) return true;
	return false;
}
inline static bool MOS6510_IF_DECIMAL(MOS6510* p)
{
	if(p->SR&8) return true;
	return false;
}
inline static void MOS6510_SET_OVERFLOW(MOS6510* p, bool status)
{
	if (status!=0) p->SR|=64;
	else p->SR&=0xBF;
}
static int Disassemble(MOS6510* p, FIL* file, uint16_t pc, bool line_draw);

#endif // MOS_6510_CLASS_H
