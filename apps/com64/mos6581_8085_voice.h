//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6581_8085_voice.h                  //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 18.05.2014                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS6581_8085_VOICE_H
#define MOS6581_8085_VOICE_H

#define RELEASE		2
static const unsigned int RateCounterPeriod[];

typedef struct VOICE
{
    /// Oscillator Register ///
    bool        OscEnable;
    bool		TestBit;
    bool		RingBit;
    bool		SyncBit;
    unsigned char	Waveform;
    unsigned int	Frequenz;
    unsigned int	Pulsweite;
    unsigned int	Accu;
    unsigned int	ShiftReg;
    unsigned int	MsbRising;
    int			SyncSource;
    int			SyncDestination;

    /// Envelope Register ///
    bool		KeyBit;
    int			State;
    unsigned int	RatePeriod;
    unsigned int	RateCounter;
    unsigned int	ExponentialCounterPeriod;
    unsigned int	ExponentialCounter;
    unsigned int	EnvCounter;
    bool		HoldZero;
    unsigned int	Attack;
    unsigned int	Decay;
    unsigned int	Sustain;
    unsigned int	Release;
} VOICE;
inline static unsigned int VOICE_WaveRauschen(VOICE *v)
{
    return ((v->ShiftReg&0x400000)>>11)|((v->ShiftReg&0x100000)>>10)|((v->ShiftReg&0x010000)>>7)|((v->ShiftReg&0x002000)>>5)|
            ((v->ShiftReg&0x000800)>>4)|((v->ShiftReg&0x000080)>>1)|((v->ShiftReg&0x000010)<<1)|((v->ShiftReg&0x000004)<< 2);
}
inline static unsigned int VOICE_WaveDreieck(VOICE *v, VOICE *vs)
{
    static unsigned int tmp;

    tmp = (v->RingBit ? v->Accu ^ vs->Accu : v->Accu)& 0x800000;
    return ((tmp ? ~v->Accu : v->Accu) >> 11) & 0xfff;
}
inline static unsigned int VOICE_WaveSaegezahn(VOICE *v)
{
    return v->Accu>>12;
}
inline static unsigned int VOICE_WaveRechteck(VOICE *v)
{
    return (v->TestBit||(v->Accu>>12)>=v->Pulsweite)?0xFFF:0x000;
}
inline static void VOICE_EnvReset(VOICE *v)
{
    v->EnvCounter=0;
    v->Attack=0;
    v->Decay=0;
    v->Sustain=0;
    v->Release=0;
    v->KeyBit=false;
    v->RateCounter=0;
    v->State=RELEASE;
    v->RatePeriod=RateCounterPeriod[v->Release];
    v->HoldZero=true;
    v->ExponentialCounter=0;
    v->ExponentialCounterPeriod=1;
}
inline static void VOICE_OscReset(VOICE *v)
{
    v->Accu=0;
    v->ShiftReg=0x7FFFF8;
    v->Frequenz=0;
    v->Pulsweite=0;
    v->TestBit=false;
    v->RingBit=false;
    v->SyncBit=false;
    v->MsbRising=false;
}
#endif // MOS6581_8085_VOICE_H
