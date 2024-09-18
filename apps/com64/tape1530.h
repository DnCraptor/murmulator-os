//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: tape1530_class.h                      //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 29.06.2021                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef TAPE1530_H
#define TAPE1530_H

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include "mos6510.h"

#define REW_SPEED 12 // 15-Fache Geschwindigkeit
#define FFW_SPEED 12 // 15-Fache Geschwindigkeit

#define SAMPLE_TBL_LENGTH 0x10000

typedef struct TAPE1530
{
    MOS6510_PORT    *CPU_PORT;
    bool            CassRead;

    FILE            *image_file;

    float           cycles_per_second;

    unsigned char   *TapeBuffer;
    unsigned int    TapeBufferSize;
    unsigned int    TapeBufferPos;
    unsigned int    TapePosCycles;
    unsigned int    RecCyclesCounter;
    unsigned int    RecTapeSize;

    float           TapeLenTime;
    unsigned int    TapeLenCount;

    bool            IsTapeInsert;
    bool            IsRecTapeInsert;
    bool            WritePotected;

    unsigned int    TapeType;
    unsigned char   TapeVersion;

    bool            TapePosIsStart;
    bool            TapePosIsEnd;

    unsigned char   PressedKeys;
    int             TapeStatus;
    int             LastTapeStatus;

    unsigned int	ZyklenCounter;
    unsigned int	WaitCounter;

    float           Counter;
///    float Time2CounterTbl[1800];    // Platz für 1800 sek (30min)


    float           AddWaveWert;
    float           WaveCounter;
    unsigned short  WAVEFormatTag;
    unsigned short  WAVEChannels;
    unsigned long   WAVESampleRate;
    unsigned long	WAVEBytePerSek;
    unsigned short	WAVEBlockAlign;
    unsigned short	WAVEBitPerSample;
    unsigned long	WAVEDataSize;
    unsigned short	WAVEDataChannel;
    unsigned short	WAVEHighPeek8Bit;
    unsigned short	WAVELowPeek8Bit;
    unsigned short	WAVEHighPeek16Bit;
    unsigned short	WAVELowPeek16Bit;

    // Für Soundausgabe
    float           Samplerate;
    float           FreqConvCounter;
    float           FreqConvAddWert;
    signed short	*SoundBuffer;
    unsigned int	SoundBufferPos;
    unsigned int	SoundBufferSize;
    float           Volume;

    float PlayFrequenzFaktor;
    unsigned long PlayCounter;
    unsigned long PlayAddWert;

///    unsigned short WaveTableSinus[SAMPLE_TBL_LENGTH];
} TAPE1530;

#endif // TAPE1530_H
