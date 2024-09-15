//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6522_class.h                       //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 01.06.2021                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS6522_CLASS_H
#define MOS6522_CLASS_H

#include "structs.h"
#include "functors.h"

class Floppy1541;
class MOS6502;

class MOS6522
{
public:

    /// Funktionen ///

    MOS6522(unsigned char via_nr);
    ~MOS6522();

    void Reset(void);
    unsigned char GetIO_Zero(void);
	float GetIOPB3_RMS(void);
    void OneZyklus(void);
    void WriteIO(unsigned short adresse, unsigned char);
    unsigned char ReadIO(unsigned short adresse);
    BVProcFn<Floppy1541> SyncFound;
    CVProcFn<Floppy1541> ReadGCRByte;
    VCProcFn<Floppy1541> WriteGCRByte;
    VVProcFn<Floppy1541> SpurInc;
    VVProcFn<Floppy1541> SpurDec;
    VIProcFn<MOS6502> TriggerInterrupt;
    VIProcFn<MOS6502> ClearInterrupt;

    /// Variablen ///

    bool            *RESET;
    bool            *WriteProtect;
    bool            *DiskMotorOn;
    unsigned char   *FloppyIEC;
    unsigned char   *C64IEC;
    unsigned char   *Jumper;

private:

    /// Variablen ///

    unsigned char   VIANummer;
    unsigned char   IO[16];
    unsigned char   PA;
    unsigned char   PB;
    unsigned char   DDRA;
    unsigned char   DDRB;
    unsigned short  TimerA;
    unsigned short  TimerALatch;
    unsigned short  TimerB;
    unsigned short  TimerBLatch;
    bool            ATNState;
    bool            IECInterrupt;
    unsigned char   OldIECLines;
    bool            OldATNState;
    unsigned char   TmpByte;

	/// Spezial ///
	/// Annahme das PB3 ein PWM Signal ausgibt bei VIA2 [LED RW]

	uint32_t counter_sample_pb3;
	uint32_t addition_sample_pb3;
	float rms_pb3;
};

#endif // MOS6522_CLASS_H
