//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6581_8085_class.h                  //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 22.06.2023                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS6581_8085_H
#define MOS6581_8085_H

///#include "siddump.h"
#include "mos6581_8085_voice.h"

//// Sid Typen ////
#define MOS_6581 0
#define MOS_8580 1

#define	ATTACK		0
#define	DECAY_SUSTAIN   1
#define RELEASE		2

#define	PUFFER_TIME	1

//// Zyklen die vergehen müssen bis der letzte geschriebene Wert auf Null gesetzt wird
//// Beim lesen eines NoneRead Registers wird immer der zuletzt in den SID geschriebene Wert gelesen.
//// Nach ReSID ca. 2000- 4000 Zyklen
#define LAST_WRITE_COUNTER_START 3000
///#define IODelayPuffer_SZ 1048576
#define IODelayPuffer_SZ 1024

typedef int fc_point[2];

typedef struct MOS6581_8085
{
    /* Variable */
    short               *SoundBuffer;
    short               *SoundBufferV0;
    short               *SoundBufferV1;
    short               *SoundBufferV2;
    int                 SoundBufferPos;
    int                 SoundBufferSize;
    bool                CycleExact;
    bool                FilterOn;
    bool                SoundOutputEnable;
    bool                *RESET;

    unsigned char	WriteReg;

///    SIDDumpClass	*IoDump;

    /// Recording ///
///    bool            Recording;
///    int             RecSampleCounter;
///    short           RecSampleBuffer[19656];

    int                 Zyklencounter;
    bool                ret;

    float               Samplerate;
    VOICE               *Voice[3];
    unsigned char       IO[32];

    unsigned char       LastWriteValue;
    unsigned short      LastWriteCounter;

    bool                IODelayEnable;
    unsigned char       IODelayPuffer[IODelayPuffer_SZ][2];
    int                 IODelayRPos;
    int                 IODelayWPos;

    VOICE               *v;     // Für Temporären zugriff auf die Voices
    VOICE               *vs;    // Für Temporären zugriff auf die Voices (Source Voice)
    VOICE               *vd;    // Für Temporären zugriff auf die Voices (Destination Voice)

    int	SidNummer;
    int SidModel;

    int                 C64ZyklenSek;
    float              FreqConvCounter;	// interner Counter für Fast Fast Fast Resampling ;-)
    float              FreqConvAddWert;

    unsigned char   PotX;
    unsigned char   PotY;

    /// Zeiger auf Sonder Wellen (Mischformen) ///
    unsigned int	*Wave0;
    unsigned int	*Wave1;
    unsigned int	*Wave2;
    unsigned int	*Wave3;

    unsigned int	VoiceDC;
    unsigned int	WaveZero;

    /// Filter Register ///
    unsigned int	FilterKey;
    unsigned int	FilterFrequenz;
    unsigned int	FilterResonanz;
    unsigned int	Voice3Off;
    unsigned int	HpBpLp;
    unsigned int	Volume;

    /// Filter interne Register
    int         MixerDC;
    int         Vhp;
    int         Vbp;
    int         Vlp;
    int         Vnf;
    int         w0,w0_ceil_1,w0_ceil_dt;
    int         _1024_div_Q;
    int         f0_6581[2048];
    int         f0_8580[2048];
    int*		f0;
    fc_point*   f0_points;
    int			f0_count;
} MOS6581_8085;

static void MOS6581_8085_MOS6581_8085_free(MOS6581_8085* p);
static void MOS6581_8085_MOS6581_8085(MOS6581_8085* p, int nummer, int samplerate, int puffersize);
static void MOS6581_8085_WriteIO(MOS6581_8085* p, unsigned short adresse,unsigned char wert);
static unsigned int MOS6581_8085_OscOutput(MOS6581_8085* p, int voice);
static unsigned int MOS6581_8085_EnvOutput(MOS6581_8085* p, int voice);
static unsigned char MOS6581_8085_ReadIO(MOS6581_8085* p, unsigned short adresse);
static void MOS6581_8085_SetChipType(MOS6581_8085* p, int type);
static void MOS6581_8085_SetW0(MOS6581_8085* p);
static void MOS6581_8085_Reset(MOS6581_8085* p);
inline static void MOS6581_8085_SetQ(MOS6581_8085* p)
{
    p->_1024_div_Q = (int)(1024.0/(0.707 + 1.0 * p->FilterResonanz / 0x0F));
}
inline static void MOS6581_8085_OscReset(MOS6581_8085* p)
{
    for(int i = 0; i < 3; ++i)
    {
        VOICE_OscReset(p->Voice[i]);
    }
}
inline static void MOS6581_8085_EnvReset(MOS6581_8085* p)
{
    for(int i = 0; i < 3; ++i)
    {
        VOICE_EnvReset(p->Voice[i]);
    }
}
inline static void MOS6581_8085_FilterReset(MOS6581_8085* p)
{
    p->FilterFrequenz=0;
    p->FilterResonanz=0;
    p->FilterKey=0;
    p->Voice3Off=0;
    p->HpBpLp=0;
    p->Volume=0;

    p->Vhp=0;
    p->Vbp=0;
    p->Vlp=0;
    p->Vnf=0;

    MOS6581_8085_SetW0(p);
    MOS6581_8085_SetQ(p);
}

#endif // MOS6581_8085_H
