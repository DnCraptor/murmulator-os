//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: reu_class.cpp                         //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 13.09.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef REU_H
#define REU_H

#include "func.h"
#include "structs.h"

typedef struct REU
{
    func_read_t *ReadProcTbl;
    func_write_t *WriteProcTbl;
    func_vi_t CpuTriggerInterrupt;
    func_vi_t CpuClearInterrupt;

    // Variablen
    bool *BA;			// Extern
    bool *RESET;		// Extern
    bool *WRITE_FF00;	// Extern

    unsigned char DMAStatus;	// 0=kein Transfer / 1=Lesen / 2=Schreiben / 3=Swap / 4=Verify

    bool			BA_STATUS;

    //unsigned char	RamBaenke[32][0x10000];	// 32 x 64KB = 2MB
///    unsigned char	RamBaenke[256][0x10000]; // 256 x 64KB = 16MB
unsigned char*	RamBaenke; //// TODO: ^^

    bool			REUInsert;
    bool			REUWait_FF00;

    // IO
    unsigned char  IO[0x0B];
    unsigned char  CPUWaitCounter;
    bool           TransferStart;
    unsigned short AdresseC64;
    unsigned short AdresseREU;
    unsigned short Counter;
    unsigned char  Bank;
    unsigned char  TransferTyp;
} REU;

inline static void REU_Reset(REU* p)
{
    p->BA_STATUS = true;
    p->TransferStart = false;
    p->IO[0] = 16;
    p->IO[1] = 16;
    p->IO[2] = 0;
    p->IO[3] = 0;
    p->IO[4] = 0;
    p->IO[5] = 0;
    p->IO[6] = 0;
    p->IO[7] = 255;
    p->IO[8] = 255;
    p->IO[9] = 31;
    p->IO[10] = 63;
}
inline static void REU_ClearRAM(REU* p) { /** TODO: */}
inline static void REU_REU(REU* p)
{
    p->REUInsert = false;
    REU_ClearRAM(p);
    p->DMAStatus = 0;
    REU_Reset(p);
}

#endif // REU_H
