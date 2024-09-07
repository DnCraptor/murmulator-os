/***************************************************************************
 *   Copyright (C) 2008 by Deryabin Andrew   				               *
 *   andrew@it-optima.ru                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
//#include <pico/platform.h>
#include "m-os-api.h"
#include "m-os-api-sdtfn.h"
#include "m-os-api-timer.h"
#include "m-os-api-math.h"
#include "m-os-api-c-string.h"
// TODO:
#undef switch

static char tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (c - 'A') + 'a';
    }
    // TODO: cp866 chars
    return c;
}

#include "ayfly.h"

int main(void) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx->argc < 2 || ctx->argc > 3) {
e0:
        fprintf(ctx->std_err,
            "Usage: ay [file] [bytes]\n"
            "  where [bytes] - optional param to reserve some RAM for other applications (512 by default).\n"
        );
        return 1;
    }
    int reserve = 512;
    if (ctx->argc == 3) {
        reserve = atoi(ctx->argv[2]);
        if (!reserve) {
            goto e0;
        }
    }
    int res = 0;
    void* p = ay_initsong(ctx->argv[1], 44100, NULL);
    if (!p) {
        fprintf(ctx->std_err, "ay_initsong returns NULL\n");
        return -1;
    }
    printf("  songpath: [%s]\n", ay_getsongpath(p));
    printf("songauthor: [%s]\n", ay_getsongauthor(p));
    printf("  songname: [%s]\n", ay_getsongname(p));
    printf("TODO: Play!\n");
    return res;
}

#define TACTS_MULT (unsigned long)800
#define VOL_BEEPER (15000)

const float init_levels_ay[] =
{ 0, 836, 1212, 1773, 2619, 3875, 5397, 8823, 10392, 16706, 23339, 29292, 36969, 46421, 55195, 65535 };

const float init_levels_ym[] =
{ 0, 0, 0xF8, 0x1C2, 0x29E, 0x33A, 0x3F2, 0x4D7, 0x610, 0x77F, 0x90A, 0xA42, 0xC3B, 0xEC2, 0x1137, 0x13A7, 0x1750, 0x1BF9, 0x20DF, 0x2596, 0x2C9D, 0x3579, 0x3E55, 0x4768, 0x54FF, 0x6624, 0x773B, 0x883F, 0xA1DA, 0xC0FC, 0xE094, 0xFFFF };

#define TONE_ENABLE(a, ch) ((a->regs [AY_MIXER] >> (ch)) & 1)
#define NOISE_ENABLE(a, ch) ((a->regs [AY_MIXER] >> (3 + (ch))) & 1)
#define TONE_PERIOD(a, ch) (((((a->regs [((ch) << 1) + 1]) & 0xf) << 8)) | (a->regs [(ch) << 1]))
#define NOISE_PERIOD(a) (a->regs [AY_NOISE_PERIOD] & 0x1f)
#define CHNL_VOLUME(a, ch) (a->regs [AY_CHNL_A_VOL + (ch)] & 0xf)
#define CHNL_ENVELOPE(a, ch) (a->regs [AY_CHNL_A_VOL + (ch)] & 0x10)
#define ENVELOPE_PERIOD(a) (((a->regs [AY_ENV_COARSE]) << 8) | a->regs [AY_ENV_FINE])

static const init_mix_levels mix_levels[] = {
    { 1.0, 0.33, 0.67, 0.67, 0.33, 1.0 }, //AY_ABC
    { 1.0, 0.33, 0.33, 1.0, 0.67, 0.67 }, //AY_ACB
    { 0.67, 0.67, 1.0, 0.33, 0.33, 1.0 }, //AY_BAC
    { 0.33, 1.0, 1.0, 0.33, 0.67, 0.67 }, //AY_BCA
    { 0.67, 0.67, 0.33, 1.0, 1.0, 0.33 }, //AY_CAB
    { 0.33, 1.0, 0.67, 0.67, 1.0, 0.33 }  //AY_CBA
};

static void aySetMixType(ay_t* a, AYMixTypes mixType) {
    if (mixType >= 0 && mixType < sizeof(mix_levels) / sizeof(mix_levels[0])) {
        a->a_left = mix_levels[mixType].a_left;
        a->a_right = mix_levels[mixType].a_right;
        a->b_left = mix_levels[mixType].b_left;
        a->b_right = mix_levels[mixType].b_right;
        a->c_left = mix_levels[mixType].c_left;
        a->c_right = mix_levels[mixType].c_right;
        a->mix_levels_nr = mixType;
    }
}

void aySetParameters(ay_t* a, AYSongInfo* _songinfo) {
    if(_songinfo == 0 && a->songinfo == 0)
        return;
    if((_songinfo != a->songinfo) && (_songinfo != 0))
        a->songinfo = _songinfo;
    if(a->songinfo->sr == 0 || a->songinfo->int_freq == 0)
        return;
    a->ay_tacts_f = ((double)a->songinfo->ay_freq * a->songinfo->ay_oversample) / (double)a->songinfo->sr / 8.0;
    a->flt_state_limit = ((double)a->songinfo->ay_freq * TACTS_MULT * a->songinfo->ay_oversample) / (double)a->songinfo->sr / 8.0;
    a->ay_tacts = a->ay_tacts_f;
    if((a->ay_tacts_f - a->ay_tacts) >= 0.5)
        a->ay_tacts++;
    a->levels = a->songinfo->chip_type == 0 ? a->levels_ay : a->levels_ym;
    float int_limit_f = ((float)a->songinfo->sr * TACTS_MULT) / (float)a->songinfo->int_freq * a->ay_tacts_f;
    a->int_limit = int_limit_f;
    if(int_limit_f - a->int_limit >= 0.5)
        a->int_limit++;
    a->frame_size = a->songinfo->sr / a->songinfo->int_freq;
    if(a->songinfo->is_z80) {
        float z80_per_sample_f = ((float)a->songinfo->z80_freq * TACTS_MULT) / a->songinfo->sr / a->ay_tacts_f;
        a->z80_per_sample = z80_per_sample_f;
        if(z80_per_sample_f - a->z80_per_sample)
            a->z80_per_sample++;
        float int_per_z80_f = ((float)a->songinfo->z80_freq * TACTS_MULT) / (float)a->songinfo->int_freq;
        a->int_per_z80 = int_per_z80_f;
        if((int_per_z80_f - a->int_per_z80) >= 0.5)
            a->int_per_z80++;
    }
    a->flt_state = a->flt_state_limit + TACTS_MULT;
    a->src_remaining = 0;
    FilterOpts fopts;
    fopts.Fs = a->songinfo->ay_freq * a->songinfo->ay_oversample / 8;
    fopts.f0 = (float)a->songinfo->sr / (float)4;
    fopts.Q = 1;
    fopts.type = LPF;
    if (a->pflt) delete_Filter3(a->pflt);
    a->pflt = new_Filter3();
    Filter3Init(a->pflt, &fopts);
    aySetMixType(a, a->songinfo->mix_levels_nr);
}

void ayReset(ay_t* a) {
    gouta("ayReset\n");
    //init regs with defaults
    a->int_limit = 0;
    a->int_counter = 0;
    a->z80_per_sample_counter = 0;
    a->int_per_z80_counter = 0;
    gouta("ayReset1\n");
    memset(a->regs, 0, sizeof(a->regs));
    a->regs[AY_GPIO_A] = a->regs[AY_GPIO_B] = 0xff;
    a->chnl_period0 = a->chnl_period1 = a->chnl_period2 = 0;
    a->tone_period_init0 = a->tone_period_init1 = a->tone_period_init2 = 0;
    a->chnl_mute0 = a->chnl_mute1 = a->chnl_mute2 = false;
    a->env_type = 0;
    a->env_vol = 0;
    a->chnl_trigger0 = a->chnl_trigger1 = a->chnl_trigger2 = 0;
    a->noise_reg = 0x1;
    a->noise_trigger = 1;
    a->noise_period = 1;

    a->volume0 = a->volume1 = a->volume2 = 1;
    a->env_type_old = -1;
    a->env_step = 0;
    a->ay_tacts_counter = 0;

    a->beeper_volume = 0;
    a->beeper_oldval = false;

    gouta("ayReset2\n");
    aySetParameters(a, 0);
    gouta("ayReset3\n");
    aySetEnvelope(a);
    gouta("ayReset4\n");
}

static void ayWrite(ay_t* a, unsigned char reg, unsigned char val) {
    a->regs[reg & 0xf] = val;
    if (reg == AY_CHNL_A_COARSE || reg == AY_CHNL_A_FINE)
            a->tone_period_init0 = TONE_PERIOD(a, 0) * a->songinfo->ay_oversample;
    else if (reg == AY_CHNL_B_COARSE || reg == AY_CHNL_B_FINE)
            a->tone_period_init1 = TONE_PERIOD(a, 1) * a->songinfo->ay_oversample;
    else if (reg == AY_CHNL_C_COARSE || reg == AY_CHNL_C_FINE)
            a->tone_period_init2 = TONE_PERIOD(a, 2) * a->songinfo->ay_oversample;
    else if (reg == AY_NOISE_PERIOD)
            a->noise_period_init = NOISE_PERIOD(a) * 2 * a->songinfo->ay_oversample;
    else if (reg == AY_ENV_SHAPE)
            aySetEnvelope(a);
    else if (reg == AY_ENV_COARSE || reg == AY_ENV_FINE)
            a->env_period_init = ENVELOPE_PERIOD(a) * a->songinfo->ay_oversample;
	if(a->songinfo->aywrite_callback)
		a->songinfo->aywrite_callback(a->songinfo, a->chip_nr, reg, val);
}

void aySetEnvelope(ay_t* a) {
    long et = a->regs[AY_ENV_SHAPE];
    a->env_type = et;
    if ( et == 0 || et == 1 || et == 2 || et == 3 || et == 9 ) {
        a->env_step = 0;
        a->env_vol = 31;
    }
    else if ( et == 4 || et == 5 || et == 6 || et == 7 || et == 15 ) {
        a->env_step = 1;
        a->env_vol = 0;
    }
    else if ( et == 8 ) {
        a->env_step = 2;
        a->env_vol = 31;
    }
    else if ( et == 10 ) {
        a->env_step = 3;
        a->env_vol = 31;
    }
    else if ( et == 11 ) {
        a->env_step = 4;
        a->env_vol = 31;
    }
    else if ( et == 12 ) {
        a->env_step = 5;
        a->env_vol = 0;
    }
    else if ( et == 13 ) {
        a->env_step = 6;
        a->env_vol = 0;
    }
    else if ( et == 14 ) {
        a->env_step = 7;
        a->env_vol = 0;
    }
    else {
        a->env_step = 8;
        a->env_vol = 0;
    }
}

static void ayUpdateEnvelope(ay_t* a) {
    a->env_period++;
    if(a->env_period >= a->env_period_init) {
        a->env_period = 0;
        unsigned char c = a->env_step;
        if ( c == 0 ) {
            if(--a->env_vol == 0) {
                a->env_step = 8;
            }
        }
        else if ( c == 1 ) {
                if(++a->env_vol == 32) {
                    a->env_vol = 0;
                    a->env_step = 8;
                }
        }
        else if ( c == 2 ) {
                if(--a->env_vol == -1)
                    a->env_vol = 31;
        }
        else if ( c == 3 ) {
                if(--a->env_vol == -1)
                {
                    a->env_vol = 0;
                    a->env_step = 7;
                }
        }
        else if ( c == 4 ) {
                if(--a->env_vol == -1)
                {
                    a->env_vol = 31;
                    a->env_step = 8;
                }
        }
        else if ( c == 5 ) {
                if(++a->env_vol == 32)
                    a->env_vol = 0;
        }
        else if ( c == 6 ) {
                if(++a->env_vol == 31)
                    a->env_step = 8;
        }
        else if ( c == 7 ) {
                if(++a->env_vol == 32)
                {
                    a->env_vol = 31;
                    a->env_step = 3;
                }
        }
    }
}

inline static void ayStep(ay_t* a, float* ps0, float* ps1, float* ps2) {
    *ps0 = *ps1 = *ps2 = 0;
    if(++a->chnl_period0 >= a->tone_period_init0)
    {
        a->chnl_period0 -= a->tone_period_init0;
        a->chnl_trigger0 ^= 1;
    }
    if(++a->chnl_period1 >= a->tone_period_init1)
    {
        a->chnl_period1 -= a->tone_period_init1;
        a->chnl_trigger1 ^= 1;
    }
    if(++a->chnl_period2 >= a->tone_period_init2)
    {
        a->chnl_period2 -= a->tone_period_init2;
        a->chnl_trigger2 ^= 1;
    }

    if(++a->noise_period >= a->noise_period_init)
    {
        a->noise_period = 0;
        a->noise_reg = (a->noise_reg >> 1) ^ ((a->noise_reg & 1) ? 0x14000 : 0);
        a->noise_trigger = a->noise_reg & 1;
    }
    ayUpdateEnvelope(a);

    if((a->chnl_trigger0 | TONE_ENABLE(a, 0)) & (a->noise_trigger | NOISE_ENABLE(a, 0)) & !a->chnl_mute0)
        *ps0 = (CHNL_ENVELOPE(a, 0) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 0) * 2]) * a->volume0;
    if((a->chnl_trigger1 | TONE_ENABLE(a, 1)) & (a->noise_trigger | NOISE_ENABLE(a, 1)) & !a->chnl_mute1)
        *ps1 = (CHNL_ENVELOPE(a, 1) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 1) * 2]) * a->volume1;
    if((a->chnl_trigger2 | TONE_ENABLE(a, 2)) & (a->noise_trigger | NOISE_ENABLE(a, 2)) & !a->chnl_mute2)
        *ps2 = (CHNL_ENVELOPE(a, 2) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 2) * 2]) * a->volume2;
}

static unsigned long ayProcess(ay_t* a, unsigned char *stream, unsigned long len) {
    unsigned long to_process = (len >> 1);
    float s0, s1, s2;
    unsigned long i = 0;
    short *stream16 = (short *)stream;
    while(i < to_process)
    {
        s0 = s1 = s2 = 0;
        if(!a->songinfo->empty_song)
        {
            if(a->songinfo->is_z80)
            {
                while(a->z80_per_sample_counter < a->z80_per_sample)
                {
                    int tstates = z80ex_step(a->songinfo->z80ctx) * TACTS_MULT;
                    a->z80_per_sample_counter += tstates;
                    a->int_per_z80_counter += tstates;
                    if(a->int_per_z80_counter > a->int_per_z80)
                    {
                        tstates = z80ex_int(a->songinfo->z80ctx) * TACTS_MULT;
                        a->z80_per_sample_counter += tstates;
                        a->int_per_z80_counter = tstates;
                        if(++a->songinfo->timeElapsed >= a->songinfo->Length)
                        {
                            a->songinfo->timeElapsed = a->songinfo->Loop;
                            if(a->songinfo->e_callback)
                                a->songinfo->stopping = a->songinfo->e_callback(a->songinfo->e_callback_arg);
                        }
                    }

                }
                a->z80_per_sample_counter -= a->z80_per_sample;
            }
            else
            {
                a->int_counter += TACTS_MULT;
                if(a->int_counter > a->int_limit)
                {
                    a->int_counter -= a->int_limit;
                    ay_softexec(a->songinfo);
                }
            }
        }
        else
        {
            if(a->songinfo->empty_callback)
            {
                a->int_counter += TACTS_MULT;
                if(a->int_counter > a->int_limit)
                {
                    a->int_counter -= a->int_limit;
                    a->songinfo->empty_callback(a->songinfo);
                }

            }
        }

        if(++a->chnl_period0 >= a->tone_period_init0)
        {
            a->chnl_period0 -= a->tone_period_init0;
            a->chnl_trigger0 ^= 1;
        }
        if(++a->chnl_period1 >= a->tone_period_init1)
        {
            a->chnl_period1 -= a->tone_period_init1;
            a->chnl_trigger1 ^= 1;
        }
        if(++a->chnl_period2 >= a->tone_period_init2)
        {
            a->chnl_period2 -= a->tone_period_init2;
            a->chnl_trigger2 ^= 1;
        }

        if(++a->noise_period >= a->noise_period_init)
        {
            a->noise_period = 0;
            a->noise_reg = (a->noise_reg >> 1) ^ ((a->noise_reg & 1) ? 0x14000 : 0);
            a->noise_trigger = a->noise_reg & 1;
        }
        ayUpdateEnvelope(a);

        if((a->chnl_trigger0 | TONE_ENABLE(a, 0)) & (a->noise_trigger | NOISE_ENABLE(a, 0)) & !a->chnl_mute0)
            s0 = (CHNL_ENVELOPE(a, 0) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 0) * 2]) * a->volume0;
        if((a->chnl_trigger1 | TONE_ENABLE(a, 1)) & (a->noise_trigger | NOISE_ENABLE(a, 1)) & !a->chnl_mute1)
            s1 = (CHNL_ENVELOPE(a, 1) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 1) * 2]) * a->volume1;
        if((a->chnl_trigger2 | TONE_ENABLE(a, 2)) & (a->noise_trigger | NOISE_ENABLE(a, 2)) & !a->chnl_mute2)
            s2 = (CHNL_ENVELOPE(a, 2) ? a->levels[a->env_vol] : a->levels[CHNL_VOLUME(a, 2) * 2]) * a->volume2;

        if(a->songinfo->is_ts)
        {
            float s3, s4, s5;
            ayStep(a->songinfo->pay8910[1], &s3, &s4, &s5);
            s0 = (s0 + s3) / 2;
            s1 = (s1 + s4) / 2;
            s2 = (s2 + s5) / 2;
        }

        if(a->songinfo->stopping)
            break;

        float left = s0 * a->a_left + s1 * a->b_left + s2 * a->c_left + a->beeper_volume;
        float right = s0 * a->a_right + s1 * a->b_right + s2 * a->c_right + a->beeper_volume;

        if (a->pflt) Filter3Process2(a->pflt, &left, &right);
        if(a->flt_state > a->flt_state_limit)
        {
            a->flt_state -= a->flt_state_limit;
            stream16[i] = left;
            stream16[i + 1] = right;
            i += 2;
        }
        a->flt_state += TACTS_MULT;

    }
    return (i << 1);
}

static void ayBeeper(ay_t* a, bool on)
{
    if(a->beeper_oldval == on)
        return;
    a->beeper_volume = on ? -VOL_BEEPER : 0;
    a->beeper_oldval = on;
}

#include "MOSAudio.c"
#include "Filter3.c"
#include "common.c"
#include "z80ex.c"
#include "formats.c"
#include "speccy.c"
#include "lha.c"
