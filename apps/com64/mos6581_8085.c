//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6581_8085class.cpp                 //
//                                              //
// Dieser Sourcecode ist Copyright geschutzt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Anderung am 22.06.2023                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include <stdbool.h>
#include "mos6581_8085.h"
#include "mos6581_8085_calc.h"
#include "mos6581_8085_wellenformen.h"

static const unsigned int RateCounterPeriod[]={9,32,63,95,149,220,267,313,392,977,1954,3126,3907,11720,19532,31251};
static const unsigned int SustainLevel[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
static const fc_point f0_points_6581[31]={{0,220},{0,220},{128,230},{256,250},{384,300},{512,420},{640,780},{768,1600},{832,2300},{896,3200},{960,4300},{992,5000},{1008,5400},{1016,5700},{1023,6000},{1023,6000},{1024,4600},{1024,4600},{1032,4800},{1056,5300},{1088,6000},{1120,6600},{1152,7200},{1280,9500},{1408,12000},{1536,14500},{1664,16000},{1792,17100},{1920,17700},{2047,18000},{2047,18000}};
static const fc_point f0_points_8580[19]={{0,0},{0,0},{128,800},{256,1600},{384,2500},{512,3300},{640,4100},{768,4800},{896,5600},{1024,6500},{1152,7500},{1280,8400},{1408,9200},{1536,9800},{1664,10500},{1792,11000},{1920,11700},{2047,12500},{2047,12500}};

static void MOS6581_8085_MOS6581_8085_free(MOS6581_8085* p) {
    free(p->SoundBuffer);
    free(p->SoundBufferV0);
    free(p->SoundBufferV1);
    free(p->SoundBufferV2);
    free(p->Voice[0]);
    free(p->Voice[1]);
    free(p->Voice[2]);
    free(p);
}

static void MOS6581_8085_SetChipType(MOS6581_8085* p, int type)
{
    p->SidModel = type;

    if(p->SidModel==MOS_6581)
    {
        p->Wave0=Wave6581_0;
        p->Wave1=Wave6581_1;
        p->Wave2=Wave6581_2;
        p->Wave3=Wave6581_3;

        p->WaveZero=0x380;
        p->VoiceDC=0x800*0xFF;

        p->MixerDC=-0xfff*0xff/18>>7;
        p->f0=p->f0_6581;
        p->f0_points=f0_points_6581;
        p->f0_count=sizeof(f0_points_6581)/sizeof(*f0_points_6581);
    }
    else
    {
        p->Wave0=Wave8580_0;
        p->Wave1=Wave8580_1;
        p->Wave2=Wave8580_2;
        p->Wave3=Wave8580_3;

        p->WaveZero=0x800;
        p->VoiceDC=0;

        p->MixerDC=0;
        p->f0=p->f0_8580;
        p->f0_points=f0_points_8580;
        p->f0_count=sizeof(f0_points_8580)/sizeof(*f0_points_8580);
    }
    MOS6581_8085_SetW0(p);
    MOS6581_8085_SetQ(p);
}

static void MOS6581_8085_MOS6581_8085(MOS6581_8085* p, int nummer, int samplerate, int puffersize)
{
    p->Voice[0] = (VOICE*)calloc(1, sizeof(VOICE));
    p->Voice[1] = (VOICE*)calloc(1, sizeof(VOICE));
    p->Voice[2] = (VOICE*)calloc(1, sizeof(VOICE));
    
    p->SidNummer = nummer;
    p->FilterFrequenz = 0;
    p->Samplerate=samplerate;
    p->C64ZyklenSek = 985248;
    p->FreqConvAddWert = p->Samplerate / p->C64ZyklenSek;
    p->FreqConvCounter = 0.0;

    p->IODelayEnable = false;
    p->IODelayRPos = 0;
    p->IODelayWPos = 1000;

    p->LastWriteValue = 0;

    for(int i = 0; i < IODelayPuffer_SZ; ++i)
    {
        p->IODelayPuffer[i][0] = 0x20;
        p->IODelayPuffer[i][1] = 0;
    }

    p->Voice[0]->OscEnable = true;
    p->Voice[1]->OscEnable = true;
    p->Voice[2]->OscEnable = true;

    p->FilterOn=true;
    p->Voice[0]->SyncSource=2;
    p->Voice[1]->SyncSource=0;
    p->Voice[2]->SyncSource=1;
    p->Voice[0]->SyncDestination=1;
    p->Voice[1]->SyncDestination=2;
    p->Voice[2]->SyncDestination=0;

    interpolate(f0_points_6581, f0_points_6581+sizeof(f0_points_6581)/sizeof(*f0_points_6581) - 1, p->f0_6581, 1.0);
    interpolate(f0_points_8580, f0_points_8580+sizeof(f0_points_8580)/sizeof(*f0_points_8580) - 1, p->f0_8580, 1.0);
	
    p->SoundOutputEnable=false;

    p->CycleExact=false;

///    p->IoDump = new SIDDumpClass(IO);
///    p->IoDump->WriteReg = &WriteReg;

    p->Zyklencounter = 0;
    p->SoundBufferPos = 0;
    p->SoundBufferSize = puffersize;
    p->SoundBuffer = calloc(sizeof(short), puffersize);
    p->SoundBufferV0 = calloc(sizeof(short), puffersize);
    p->SoundBufferV1 = calloc(sizeof(short), puffersize);
    p->SoundBufferV2 = calloc(sizeof(short), puffersize);

    p->PotX = p->PotY = 0xFF;

///    p->Recording = false;
///    p->RecSampleCounter = 0;

    p->FilterResonanz = 0;

    MOS6581_8085_SetChipType(p, 0);
    MOS6581_8085_Reset(p);
}

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
    }
    switch(adresse & 0x1F)
    {
    case 0: // FrequenzLO fur Stimme 0
        p->Voice[0]->Frequenz = (p->Voice[0]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 1: // FrequenzHI fur Stimme 0
        p->Voice[0]->Frequenz = ((wert<<8) & 0xFF00) | (p->Voice[0]->Frequenz & 0x00FF);
        break;

    case 2: // PulsweiteLO fur Stimme 0
        p->Voice[0]->Pulsweite = (p->Voice[0]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 3: // PulsweiteHI fur Stimme 0
        p->Voice[0]->Pulsweite=((wert<<8) & 0xF00) | (p->Voice[0]->Pulsweite & 0x0FF);
        break;

    case 4: // Kontrol Register fur Stimme 0
        p->Voice[0]->Waveform=(wert>>4) & 0x0F;
        p->Voice[0]->RingBit = !(~wert & 0x04);
        p->Voice[0]->SyncBit = !(~wert & 0x02);
        if(wert & 0x08)
        {
            p->Voice[0]->Accu=0;
            p->Voice[0]->ShiftReg=0;
        }
        else if(p->Voice[0]->TestBit) p->Voice[0]->ShiftReg=0x7FFFF8;
        p->Voice[0]->TestBit = !(~wert & 0x08);
        
        KeyNext = wert & 0x01;
        if (!p->Voice[0]->KeyBit && KeyNext)
        {
            p->Voice[0]->State=ATTACK;
            p->Voice[0]->RatePeriod=RateCounterPeriod[p->Voice[0]->Attack];
            p->Voice[0]->HoldZero=false;
        }
        else if (p->Voice[0]->KeyBit && !KeyNext)
        {
            p->Voice[0]->State=RELEASE;
            p->Voice[0]->RatePeriod=RateCounterPeriod[p->Voice[0]->Release];
        }
        p->Voice[0]->KeyBit=KeyNext;
        break;

    case 5: // Attack-Decay fur Stimme 0
        p->Voice[0]->Attack=(wert>>4)&0x0F;
        p->Voice[0]->Decay=wert&0x0F;
        if(p->Voice[0]->State==ATTACK) p->Voice[0]->RatePeriod=RateCounterPeriod[p->Voice[0]->Attack];
        else if (p->Voice[0]->State==DECAY_SUSTAIN) p->Voice[0]->RatePeriod=RateCounterPeriod[p->Voice[0]->Decay];
        break;

    case 6:		// Sustain-Release fur Stimme 0
        p->Voice[0]->Sustain=(wert>>4)&0x0F;
        p->Voice[0]->Release=wert&0x0F;
        if (p->Voice[0]->State==RELEASE) p->Voice[0]->RatePeriod=RateCounterPeriod[p->Voice[0]->Release];
        break;
        
        /////////////////////////////////////////////////////////////

    case 7: // FrequenzLO fur Stimme 1
        p->Voice[1]->Frequenz=(p->Voice[1]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 8: // FrequenzHI fur Stimme 1
        p->Voice[1]->Frequenz=((wert<<8) & 0xFF00) | (p->Voice[1]->Frequenz & 0x00FF);
        break;

    case 9: // PulsweiteLO fur Stimme 1
        p->Voice[1]->Pulsweite=(p->Voice[1]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 10: // PulsweiteHI fur Stimme 1
        p->Voice[1]->Pulsweite=((wert<<8) & 0xF00) | (p->Voice[1]->Pulsweite & 0x0FF);
        break;

    case 11: // Kontrol Register fur Stimme 1
        p->Voice[1]->Waveform=(wert>>4)&0x0F;
        p->Voice[1]->RingBit = !(~wert&0x04);
        p->Voice[1]->SyncBit = !(~wert&0x02);
        if(wert&0x08)
        {
            p->Voice[1]->Accu=0;
            p->Voice[1]->ShiftReg=0;
        }
        else if(p->Voice[1]->TestBit) p->Voice[1]->ShiftReg=0x7FFFF8;
        p->Voice[1]->TestBit = !(~wert&0x08);
        
        KeyNext=wert&0x01;
        if (!p->Voice[1]->KeyBit && KeyNext)
        {
            p->Voice[1]->State=ATTACK;
            p->Voice[1]->RatePeriod=RateCounterPeriod[p->Voice[1]->Attack];
            p->Voice[1]->HoldZero=false;
        }
        else if (p->Voice[1]->KeyBit && !KeyNext)
        {
            p->Voice[1]->State=RELEASE;
            p->Voice[1]->RatePeriod=RateCounterPeriod[p->Voice[1]->Release];
        }
        p->Voice[1]->KeyBit = KeyNext;
        break;

    case 12: // Attack-Decay fur Stimme 1
        p->Voice[1]->Attack=(wert>>4)&0x0F;
        p->Voice[1]->Decay=wert&0x0F;
        if(p->Voice[1]->State==ATTACK) p->Voice[1]->RatePeriod=RateCounterPeriod[p->Voice[1]->Attack];
        else if (p->Voice[1]->State==DECAY_SUSTAIN) p->Voice[1]->RatePeriod=RateCounterPeriod[p->Voice[1]->Decay];
        break;

    case 13: // Sustain-Release fur Stimme 1
        p->Voice[1]->Sustain=(wert>>4)&0x0F;
        p->Voice[1]->Release=wert&0x0F;
        if (p->Voice[1]->State==RELEASE) p->Voice[1]->RatePeriod=RateCounterPeriod[p->Voice[1]->Release];
        break;
        
        /////////////////////////////////////////////////////////////

    case 14: // FrequenzLO fur Stimme 2
        p->Voice[2]->Frequenz=(p->Voice[2]->Frequenz & 0xFF00) | (wert & 0x00FF);
        break;

    case 15: // FrequenzHI fur Stimme 2
        p->Voice[2]->Frequenz=((wert<<8) & 0xFF00) | (p->Voice[2]->Frequenz & 0x00FF);
        break;

    case 16: // PulsweiteLO fur Stimme 2
        p->Voice[2]->Pulsweite=(p->Voice[2]->Pulsweite & 0xF00) | (wert & 0x0FF);
        break;

    case 17: // PulsweiteHI fur Stimme 2
        p->Voice[2]->Pulsweite=((wert<<8) & 0xF00) | (p->Voice[2]->Pulsweite & 0x0FF);
        break;

    case 18: // Kontrol Register fur Stimme 2
        p->Voice[2]->Waveform=(wert>>4)&0x0F;
        p->Voice[2]->RingBit = !(~wert&0x04);
        p->Voice[2]->SyncBit = !(~wert&0x02);
        if(wert&0x08)
        {
            p->Voice[2]->Accu=0;
            p->Voice[2]->ShiftReg=0;
        }
        else if(p->Voice[2]->TestBit) p->Voice[2]->ShiftReg=0x7FFFF8;
        p->Voice[2]->TestBit = !(~wert&0x08);
        
        KeyNext = wert & 0x01;
        if (!p->Voice[2]->KeyBit && KeyNext)
        {
            p->Voice[2]->State=ATTACK;
            p->Voice[2]->RatePeriod=RateCounterPeriod[p->Voice[2]->Attack];
            p->Voice[2]->HoldZero=false;
        }
        else if (p->Voice[2]->KeyBit && !KeyNext)
        {
            p->Voice[2]->State=RELEASE;
            p->Voice[2]->RatePeriod=RateCounterPeriod[p->Voice[2]->Release];
        }
        p->Voice[2]->KeyBit=KeyNext;
        break;

    case 19: // Attack-Decay fur Stimme 2
        p->Voice[2]->Attack=(wert>>4)&0x0F;
        p->Voice[2]->Decay=wert&0x0F;
        if(p->Voice[2]->State==ATTACK) p->Voice[2]->RatePeriod=RateCounterPeriod[p->Voice[2]->Attack];
        else if (p->Voice[2]->State==DECAY_SUSTAIN) p->Voice[2]->RatePeriod=RateCounterPeriod[p->Voice[2]->Decay];
        break;

    case 20: // Sustain-Release fur Stimme 2
        p->Voice[2]->Sustain=(wert>>4)&0x0F;
        p->Voice[2]->Release=wert&0x0F;
        if (p->Voice[2]->State==RELEASE) p->Voice[2]->RatePeriod=RateCounterPeriod[p->Voice[2]->Release];
        break;

    case 21: // FilterfrequenzLO
        p->FilterFrequenz=(p->FilterFrequenz & 0x7F8) | (wert & 0x007);
        MOS6581_8085_SetW0(p);
        break;

    case 22: // FilterfrequenzHI
        p->FilterFrequenz=((wert<<3) & 0x7F8) | (p->FilterFrequenz & 0x007);
        MOS6581_8085_SetW0(p);
        break;

    case 23: // Filterkontrol_1 und Resonanz
        p->FilterResonanz=(wert>>4)&0x0F;
        MOS6581_8085_SetQ(p);
        p->FilterKey=wert&0x0F;
        break;

    case 24: // Filterkontrol_2 und Lautstarke
        p->Voice3Off=wert&0x80;
        p->HpBpLp=(wert>>4)&0x07;
        p->Volume=wert&0x0F;
        break;
    }
}

static unsigned int MOS6581_8085_OscOutput(MOS6581_8085* p, int voice)
{
    if(!p->Voice[voice]->OscEnable) return 0;

    p->v = p->Voice[voice];
    p->vs = p->Voice[p->v->SyncSource];

    switch (p->v->Waveform)
    {
    case 0x01:
        return VOICE_WaveDreieck(p->v, p->vs);

    case 0x02:
        return VOICE_WaveSaegezahn(p->v);

    case 0x03:
        return p->Wave0[VOICE_WaveSaegezahn(p->v)]<<4;

    case 0x04:
        return (p->v->TestBit||(p->v->Accu>>12)>=p->v->Pulsweite)?0xFFF:0x000;

    case 0x05:
        return (p->Wave1[VOICE_WaveDreieck(p->v, p->vs)>>1]<<4) & VOICE_WaveRechteck(p->v);

    case 0x06:
        return (p->Wave2[VOICE_WaveSaegezahn(p->v)]<<4) & VOICE_WaveRechteck(p->v);

    case 0x07:
        return (p->Wave3[VOICE_WaveSaegezahn(p->v)]<<4) & VOICE_WaveRechteck(p->v);

    case 0x08:
        return VOICE_WaveRauschen(p->v);

    default:
        return 0;
    }
}

static unsigned int MOS6581_8085_EnvOutput(MOS6581_8085* p, int voice)
{
    if(!p->Voice[voice]->OscEnable) return 0;
    return p->Voice[voice]->EnvCounter;
}

static unsigned char MOS6581_8085_ReadIO(MOS6581_8085* p, unsigned short adresse)
{
    switch(adresse&0x1F)
    {
    case 25: // AD Wandler 1
        if(p->SidNummer == 0)
            return p->PotX;
         break;

    case 26: // AD Wandler 2
        if(p->SidNummer == 0)
            return p->PotY;
        break;

    case 27:
        return MOS6581_8085_OscOutput(p, 2) >> 4;

    case 28:
        return MOS6581_8085_EnvOutput(p, 2);

    default:
        return p->LastWriteValue;
    }
    return 0;
}

static void MOS6581_8085_SetW0(MOS6581_8085* p)
{
    const float pi2 = 3.1415926535897932385 * 2;
    const int w0_max_1 = (int)(pi2 * 16000 * 1.048576);
    const int w0_max_dt = (int)(pi2 * 4000 * 1.048576);
    p->w0=(int)(pi2 * p->f0[p->FilterFrequenz] * 1.048576);
    p->w0_ceil_1 = p->w0 <= w0_max_1 ? p->w0 : w0_max_1;
    p->w0_ceil_dt = p->w0 <= w0_max_dt ? p->w0 : w0_max_dt;
}

static void MOS6581_8085_Reset(MOS6581_8085* p)
{
    MOS6581_8085_OscReset(p);
    MOS6581_8085_EnvReset(p);
    MOS6581_8085_FilterReset(p);
}
