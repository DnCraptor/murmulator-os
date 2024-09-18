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
///#include "mos6581_8085_voice.h"

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

    float              Samplerate;
///    VOICEClass          *Voice[3];
    unsigned char       IO[32];

    unsigned char       LastWriteValue;
    unsigned short      LastWriteCounter;

    bool                IODelayEnable;
///    unsigned char       IODelayPuffer[1048576][2];
    int                 IODelayRPos;
    int                 IODelayWPos;

///    VOICEClass          *v;     // Für Temporären zugriff auf die Voices
///    VOICEClass          *vs;    // Für Temporären zugriff auf die Voices (Source Voice)
///    VOICEClass          *vd;    // Für Temporären zugriff auf die Voices (Destination Voice)

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
    int                 MixerDC;
    int                 Vhp;
    int                 Vbp;
    int                 Vlp;
    int                 Vnf;
    int                 w0,w0_ceil_1,w0_ceil_dt;
    int                 _1024_div_Q;
///    int                 f0_6581[2048];
///    int                 f0_8580[2048];
    int*		f0;
    fc_point*           f0_points;
    int			f0_count;
} MOS6581_8085;

static void MOS6581_8085_WriteIO(MOS6581_8085* p, unsigned short adresse,unsigned char wert) {
    p->LastWriteValue = wert;
    p->LastWriteCounter = LAST_WRITE_COUNTER_START;

    static bool KeyNext;

    if(!p->IODelayEnable)
    {
        p->WriteReg = adresse & 0x1F;
        p->IO[adresse & 0x1F]=wert;
    }
    else
    {
        /**
        if(adresse > 0xD3FF)  // kommt von CPU
        {
            p->IODelayPuffer[p->IODelayWPos][0] = adresse & 0x1F;
            p->IODelayPuffer[p->IODelayWPos][1] = wert;
            return;
        }
        else                // kommt aus Delas
        {
            wert = p->IODelayPuffer[p->IODelayRPos][1];
            p->WriteReg = adresse & 0x1F;
            p->IO[adresse & 0x1F]=wert;
        }
        */
    }
/***
    switch(adresse&0x1F)
    {
    case 0: // FrequenzLO für Stimme 0
        Voice[0]->Frequenz=(Voice[0]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 1: // FrequenzHI für Stimme 0
        Voice[0]->Frequenz=((wert<<8) & 0xFF00) | (Voice[0]->Frequenz & 0x00FF);
        break;

    case 2: // PulsweiteLO für Stimme 0
        Voice[0]->Pulsweite=(Voice[0]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 3: // PulsweiteHI für Stimme 0
        Voice[0]->Pulsweite=((wert<<8) & 0xF00) | (Voice[0]->Pulsweite & 0x0FF);
        break;

    case 4: // Kontrol Register für Stimme 0
        Voice[0]->Waveform=(wert>>4) & 0x0F;
        Voice[0]->RingBit = !(~wert & 0x04);
        Voice[0]->SyncBit = !(~wert & 0x02);
        if(wert & 0x08)
        {
            Voice[0]->Accu=0;
            Voice[0]->ShiftReg=0;
        }
        else if(Voice[0]->TestBit) Voice[0]->ShiftReg=0x7FFFF8;
        Voice[0]->TestBit = !(~wert & 0x08);
        
        KeyNext = wert & 0x01;
        if (!Voice[0]->KeyBit&&KeyNext)
        {
            Voice[0]->State=ATTACK;
            Voice[0]->RatePeriod=RateCounterPeriod[Voice[0]->Attack];
            Voice[0]->HoldZero=false;
        }
        else if (Voice[0]->KeyBit&&!KeyNext)
        {
            Voice[0]->State=RELEASE;
            Voice[0]->RatePeriod=RateCounterPeriod[Voice[0]->Release];
        }
        Voice[0]->KeyBit=KeyNext;
        break;

    case 5: // Attack-Decay für Stimme 0
        Voice[0]->Attack=(wert>>4)&0x0F;
        Voice[0]->Decay=wert&0x0F;
        if(Voice[0]->State==ATTACK) Voice[0]->RatePeriod=RateCounterPeriod[Voice[0]->Attack];
        else if (Voice[0]->State==DECAY_SUSTAIN) Voice[0]->RatePeriod=RateCounterPeriod[Voice[0]->Decay];
        break;

    case 6:		// Sustain-Release für Stimme 0
        Voice[0]->Sustain=(wert>>4)&0x0F;
        Voice[0]->Release=wert&0x0F;
        if (Voice[0]->State==RELEASE) Voice[0]->RatePeriod=RateCounterPeriod[Voice[0]->Release];
        break;
        
        /////////////////////////////////////////////////////////////

    case 7: // FrequenzLO für Stimme 1
        Voice[1]->Frequenz=(Voice[1]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 8: // FrequenzHI für Stimme 1
        Voice[1]->Frequenz=((wert<<8) & 0xFF00) | (Voice[1]->Frequenz & 0x00FF);
        break;

    case 9: // PulsweiteLO für Stimme 1
        Voice[1]->Pulsweite=(Voice[1]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 10: // PulsweiteHI für Stimme 1
        Voice[1]->Pulsweite=((wert<<8) & 0xF00) | (Voice[1]->Pulsweite & 0x0FF);
        break;

    case 11: // Kontrol Register für Stimme 1
        Voice[1]->Waveform=(wert>>4)&0x0F;
        Voice[1]->RingBit = !(~wert&0x04);
        Voice[1]->SyncBit = !(~wert&0x02);
        if(wert&0x08)
        {
            Voice[1]->Accu=0;
            Voice[1]->ShiftReg=0;
        }
        else if(Voice[1]->TestBit) Voice[1]->ShiftReg=0x7FFFF8;
        Voice[1]->TestBit = !(~wert&0x08);
        
        KeyNext=wert&0x01;
        if (!Voice[1]->KeyBit&&KeyNext)
        {
            Voice[1]->State=ATTACK;
            Voice[1]->RatePeriod=RateCounterPeriod[Voice[1]->Attack];
            Voice[1]->HoldZero=false;
        }
        else if (Voice[1]->KeyBit&&!KeyNext)
        {
            Voice[1]->State=RELEASE;
            Voice[1]->RatePeriod=RateCounterPeriod[Voice[1]->Release];
        }
        Voice[1]->KeyBit=KeyNext;
        break;

    case 12: // Attack-Decay für Stimme 1
        Voice[1]->Attack=(wert>>4)&0x0F;
        Voice[1]->Decay=wert&0x0F;
        if(Voice[1]->State==ATTACK) Voice[1]->RatePeriod=RateCounterPeriod[Voice[1]->Attack];
        else if (Voice[1]->State==DECAY_SUSTAIN) Voice[1]->RatePeriod=RateCounterPeriod[Voice[1]->Decay];
        break;

    case 13: // Sustain-Release für Stimme 1
        Voice[1]->Sustain=(wert>>4)&0x0F;
        Voice[1]->Release=wert&0x0F;
        if (Voice[1]->State==RELEASE) Voice[1]->RatePeriod=RateCounterPeriod[Voice[1]->Release];
        break;
        
        /////////////////////////////////////////////////////////////

    case 14: // FrequenzLO für Stimme 2
        Voice[2]->Frequenz=(Voice[2]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 15: // FrequenzHI für Stimme 2
        Voice[2]->Frequenz=((wert<<8) & 0xFF00) | (Voice[2]->Frequenz & 0x00FF);
        break;

    case 16: // PulsweiteLO für Stimme 2
        Voice[2]->Pulsweite=(Voice[2]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 17: // PulsweiteHI für Stimme 2
        Voice[2]->Pulsweite=((wert<<8) & 0xF00) | (Voice[2]->Pulsweite & 0x0FF);
        break;

    case 18: // Kontrol Register für Stimme 2
        Voice[2]->Waveform=(wert>>4)&0x0F;
        Voice[2]->RingBit = !(~wert&0x04);
        Voice[2]->SyncBit = !(~wert&0x02);
        if(wert&0x08)
        {
            Voice[2]->Accu=0;
            Voice[2]->ShiftReg=0;
        }
        else if(Voice[2]->TestBit) Voice[2]->ShiftReg=0x7FFFF8;
        Voice[2]->TestBit = !(~wert&0x08);
        
        KeyNext=wert&0x01;
        if (!Voice[2]->KeyBit&&KeyNext)
        {
            Voice[2]->State=ATTACK;
            Voice[2]->RatePeriod=RateCounterPeriod[Voice[2]->Attack];
            Voice[2]->HoldZero=false;
        }
        else if (Voice[2]->KeyBit&&!KeyNext)
        {
            Voice[2]->State=RELEASE;
            Voice[2]->RatePeriod=RateCounterPeriod[Voice[2]->Release];
        }
        Voice[2]->KeyBit=KeyNext;
        break;

    case 19: // Attack-Decay für Stimme 2
        Voice[2]->Attack=(wert>>4)&0x0F;
        Voice[2]->Decay=wert&0x0F;
        if(Voice[2]->State==ATTACK) Voice[2]->RatePeriod=RateCounterPeriod[Voice[2]->Attack];
        else if (Voice[2]->State==DECAY_SUSTAIN) Voice[2]->RatePeriod=RateCounterPeriod[Voice[2]->Decay];
        break;

    case 20: // Sustain-Release für Stimme 2
        Voice[2]->Sustain=(wert>>4)&0x0F;
        Voice[2]->Release=wert&0x0F;
        if (Voice[2]->State==RELEASE) Voice[2]->RatePeriod=RateCounterPeriod[Voice[2]->Release];
        break;

    case 21: // FilterfrequenzLO
        FilterFrequenz=(FilterFrequenz & 0x7F8) | (wert & 0x007);
        SetW0();
        break;

    case 22: // FilterfrequenzHI
        FilterFrequenz=((wert<<3) & 0x7F8) | (FilterFrequenz & 0x007);
        SetW0();
        break;

    case 23: // Filterkontrol_1 und Resonanz
        FilterResonanz=(wert>>4)&0x0F;
        SetQ();
        FilterKey=wert&0x0F;
        break;

    case 24: // Filterkontrol_2 und Lautstärke
        Voice3Off=wert&0x80;
        HpBpLp=(wert>>4)&0x07;
        Volume=wert&0x0F;
        break;
    }
    */
}

#endif // MOS6581_8085_H
