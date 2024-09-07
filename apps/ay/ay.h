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

#ifndef AY_H_
#define AY_H_

#include <assert.h>

#define AY_TEMP_BUFFER_SIZE 4096

enum
{
    AY_CHNL_A_FINE = 0, 
    AY_CHNL_A_COARSE, 
    AY_CHNL_B_FINE, 
    AY_CHNL_B_COARSE, 
    AY_CHNL_C_FINE, 
    AY_CHNL_C_COARSE, 
    AY_NOISE_PERIOD, 
    AY_MIXER, 
    AY_CHNL_A_VOL, 
    AY_CHNL_B_VOL, 
    AY_CHNL_C_VOL, 
    AY_ENV_FINE, 
    AY_ENV_COARSE, 
    AY_ENV_SHAPE, 
    AY_GPIO_A, 
    AY_GPIO_B
};


typedef struct init_mix_levels
{
    float a_left;
    float a_right;
    float b_left;
    float b_right;
    float c_left;
    float c_right;
} init_mix_levels;

typedef enum _ay_mix_types {
  AY_ABC = 0,
  AY_ACB,
  AY_BAC,
  AY_BCA,
  AY_CAB,
  AY_CBA
} AYMixTypes;

extern const float init_levels_ay[];
extern const float init_levels_ym[];

typedef struct ay {
	unsigned long chip_nr;
    float levels_ay[32];
    float levels_ym[32];
    float *levels;
    unsigned char regs[16];
    long noise_period;
    long chnl_period0, chnl_period1, chnl_period2;
    long tone_period_init0, tone_period_init1, tone_period_init2;
    long noise_period_init;
    long chnl_trigger0, chnl_trigger1, chnl_trigger2;
    unsigned long noise_trigger;
    unsigned long noise_reg;
    long env_period_init;
    long env_period;
    long env_type;
    long env_type_old;
    long env_vol;
    unsigned char env_step;
    bool chnl_mute0, chnl_mute1, chnl_mute2;
    unsigned long ay_tacts;
    double ay_tacts_f;
    unsigned long ay_tacts_counter;
    float volume_divider;
    float volume0, volume1, volume2;
    struct AYSongInfo *songinfo;
    unsigned long int_counter;
    unsigned long int_limit;
    long z80_per_sample;
    long int_per_z80;
    long z80_per_sample_counter;
    long int_per_z80_counter;
    unsigned long frame_size;
    AYMixTypes mix_levels_nr;
    float a_left, a_right, b_left, b_right, c_left, c_right;
    Filter3* pflt;
    unsigned long flt_state;
    unsigned long flt_state_limit;
    unsigned long src_remaining;
///    float ay_temp_buffer_in[AY_TEMP_BUFFER_SIZE];
///    float ay_temp_buffer_out[AY_TEMP_BUFFER_SIZE];
    
    //beeper stuff
    float beeper_volume;
    bool beeper_oldval;
} ay_t;

inline static ay_t* new_ay() {
    ay_t* r = calloc(1, sizeof(ay_t));
    for(unsigned long i = 0; i < 32; ++i) {
        r->levels_ay[i] = (init_levels_ay[i / 2]) / 6.0f;
        r->levels_ym[i] = init_levels_ym[i] / 6.0f;
    }
    r->songinfo = 0;
    r->chip_nr = 0;
    ayReset(r);
    return r;
}

inline static delete_ay(ay_t* a) {
    if (a->pflt) free(a->pflt);
    free(a);
}

static void ayWrite(ay_t*, unsigned char reg, unsigned char val);

inline static unsigned char ayRead(ay_t* a, unsigned char reg) {
    reg &= 0xf;
    return a->regs[reg];
}

static unsigned long ayProcess(ay_t*, unsigned char *stream, unsigned long len);
static void aySetEnvelope(ay_t*);

inline static void ayChnlMute(ay_t* a, unsigned long chnl, bool mute) {
    if (chnl == 0)
        a->chnl_mute0 = !mute;
    else if (chnl == 1)
        a->chnl_mute1 = !mute;
    else if(chnl == 2)
        a->chnl_mute2 = !mute;
}

inline static bool ayChnlMuted(ay_t* a, unsigned long chnl) {
    if (chnl == 0)
                return a->chnl_mute0;
    if (chnl == 1)
                return a->chnl_mute1;
    if (chnl == 2)
                return a->chnl_mute2;
    return false;
}

inline static float ayGetVolume(ay_t* a, unsigned long chnl) {
    if (chnl == 0)
                return a->volume0;
    if (chnl == 1)
                return a->volume1;
    if (chnl == 2)
                return a->volume2;
    return 0;
}

inline static void aySetVolume(ay_t* a, unsigned long chnl, float new_volume) {
    new_volume = new_volume > 1 ? 1 : (new_volume < 0 ? 0 : new_volume);
    if (chnl == 0)
                a->volume0 = new_volume;
    if (chnl == 1)
                a->volume1 = new_volume;
    if (chnl == 2)
                a->volume2 = new_volume;
}

inline static const unsigned char* ayGetRegs(ay_t* a) {
    return a->regs;
}

inline static AYMixTypes ayGetMixType(ay_t* a) {
    return a->mix_levels_nr;
}

static void aySetParameters(ay_t*, struct AYSongInfo* _songinfo);
static void ayBeeper(ay_t*, bool on);

#endif /*AY_H_*/

