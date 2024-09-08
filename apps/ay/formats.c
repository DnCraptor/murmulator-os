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
#include "ayfly.h"

unsigned short ay_sys_getword(unsigned char *p)
{
    return *p | ((*(p + 1)) << 8);
}

unsigned long ay_sys_getdword(unsigned char *p)
{
    return *p | ((*(p + 1)) << 8) | ((*(p + 2)) << 16) | ((*(p + 3)) << 24);
}

void ay_sys_writeword(unsigned char *p, unsigned short val)
{
    *p = (unsigned char)(val & 0xff);
    *(p + 1) = (unsigned char)((val >> 8) & 0xff);
}
string_t* ay_sys_getstr(const unsigned char *str, unsigned long length)
{
    return new_string_cl(str, length);
}

void ay_sys_initz80module(
    AYSongInfo* info,
    unsigned long player_base,
    const unsigned char *player_ptr,
    unsigned long player_length,
    unsigned long player_init_proc,
    unsigned long player_play_proc
);

#include "players/AYPlay.h"
#include "players/ASCPlay.h"
#include "players/PT1Play.h"
#include "players/PT2Play.h"
#include "players/PT3Play.h"
#include "players/STCPlay.h"
#include "players/STPPlay.h"
#include "players/PSCPlay.h"
#include "players/SQTPlay.h"
#include "players/PSGPlay.h"
#include "players/VTXPlay.h"
#include "players/YMPlay.h"

typedef void (*GETINFO_CALLBACK)(AYSongInfo* info);

typedef struct _Players {
    AY_TXT_TYPE ext;
    PLAYER_INIT_PROC soft_init_proc;
    PLAYER_PLAY_PROC soft_play_proc;
    PLAYER_CLEANUP_PROC soft_cleanup_proc;
    GETINFO_CALLBACK getInfo;
    PLAYER_DETECT_PROC detect;
    bool is_z80;
} _Players;
void init_Players(
        _Players* this,
        AY_TXT_TYPE ext,
        PLAYER_INIT_PROC soft_init_proc,
        PLAYER_PLAY_PROC soft_play_proc,
        PLAYER_CLEANUP_PROC soft_cleanup_proc,
        GETINFO_CALLBACK getInfo,
        PLAYER_DETECT_PROC detect,
        bool is_z80
) {
        this->ext = ext;
        this->soft_init_proc = soft_init_proc;
        this->soft_play_proc = soft_play_proc;
        this->soft_cleanup_proc = soft_cleanup_proc;
        this->getInfo = getInfo;
        this->detect = detect;
        this->is_z80 = is_z80;
}

static _Players* Players = 0;
static void PlayersInit(void) {
    Players = (_Players*)calloc(12, sizeof(_Players));
    init_Players(&Players[0], TXT(".ay"), AY_Init, AY_Play, AY_Cleanup, AY_GetInfo, AY_Detect, true);
    init_Players(&Players[1], TXT(".vtx"), VTX_Init, VTX_Play, VTX_Cleanup, VTX_GetInfo, VTX_Detect, false);
    init_Players(&Players[2], TXT(".ym"), YM_Init, YM6_Play, YM_Cleanup, YM_GetInfo, YM_Detect, false);
    init_Players(&Players[3], TXT(".psg"), PSG_Init, PSG_Play, PSG_Cleanup, PSG_GetInfo, PSG_Detect, false);
    init_Players(&Players[4], TXT(".asc"), ASC_Init, ASC_Play, ASC_Cleanup, ASC_GetInfo, ASC_Detect, false);
    init_Players(&Players[5], TXT(".pt2"), PT2_Init, PT2_Play, PT2_Cleanup, PT2_GetInfo, PT2_Detect, false);
    init_Players(&Players[6], TXT(".pt3"), PT3_Init, PT3_Play, PT3_Cleanup, PT3_GetInfo, PT3_Detect, false);
    init_Players(&Players[7], TXT(".stc"), STC_Init, STC_Play, STC_Cleanup, STC_GetInfo, STC_Detect, false);
    init_Players(&Players[8], TXT(".stp"), STP_Init, STP_Play, STP_Cleanup, STP_GetInfo, STP_Detect, false);
    init_Players(&Players[9], TXT(".psc"), PSC_Init, PSC_Play, PSC_Cleanup, PSC_GetInfo, PSC_Detect, false);
    init_Players(&Players[10], TXT(".sqt"), SQT_Init, SQT_Play, SQT_Cleanup, SQT_GetInfo, SQT_Detect, false);
    init_Players(&Players[11], TXT(".pt1"), PT1_Init, PT1_Play, PT1_Cleanup, PT1_GetInfo, PT1_Detect, false);
}

int _init(void) {
    Players = 0;
    return 0;
}

int _fini(void) {
    if (Players) {
        for (int player = 0; player < 12; player++) {
            if (Players[player].ext >= 0x20000000) {
                delete_string( Players[player].ext );
            }
        }
        Players = 0;
        free(Players);
    }
    return 0;
}

bool ay_sys_format_supported(AY_TXT_TYPE filePath)
{
    AY_TXT_TYPE cfp = new_string_v();
    for (size_t i = 0; i < c_strlen(filePath); ++i) {
        string_push_back_c(cfp, tolower(filePath->p[i]));
    }
    if (!Players) PlayersInit();
    for(unsigned long player = 0; player < 12; player++)
    {
        if(strcmp(Players[player].ext, c_str(cfp)) == 0)
        {
            free(cfp);
            return true;
        }
    }
    free(cfp);
    return false;
}

unsigned char *osRead(AY_TXT_TYPE filePath, unsigned long *data_len)
{
    unsigned char *fileData = 0;
    FILINFO* fi = (FILINFO*)malloc(sizeof(FILINFO));
    if ( FR_OK != f_stat(c_str(filePath), fi) ) {
        free(fi);
        return fileData;
    }
    uint32_t sz = (uint32_t)fi->fsize & 0xFFFFFFFF;
    free(fi);
    FIL* f  = (FIL*)malloc(sizeof(FIL));
    if ( FR_OK == f_open(f, c_str(filePath), FA_READ) ) {
        // TODO: control RAM
        fileData = (unsigned char*)malloc(sz);
        UINT br;
        f_read(f, fileData, sz, &br);
        *data_len = br;
        f_close(f);
    }
    free(f);
    return fileData;
}

long ay_sys_detect(AYSongInfo* info)
{
    long player = -1;
    unsigned char *tmp_module = (unsigned char*)malloc(info->file_len);
    if(!tmp_module)
        return -1;
    memcpy(tmp_module, info->file_data, info->file_len);
    if (!Players) PlayersInit();
    for(player = 0; player < 12; player++)
    {
        if(Players[player].detect != 0)
        {
            if(Players[player].detect(tmp_module, info->file_len))
                break;
        }
    }
    free(tmp_module);

    if(player >= 12)
    {
        AY_TXT_TYPE cfp = new_string_v();
        for (size_t i = 0; i < c_strlen(info->FilePath); ++i) {
            string_push_back_c(cfp, tolower(info->FilePath->p[i]));
        }
        for(player = 0; player < 12; player++)
        {
            if(strcmp(Players[player].ext, c_str(cfp)) == 0)
            {
                break;
            }
        }
        free(cfp);
    }
    if(player >= 12)
        player = -1;
    else
    {
        info->is_z80 = Players[player].is_z80;
        for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
        {
            aySetParameters(info->pay8910[i], info);
        }
    }
    return player;
}

bool ay_sys_initsong(AYSongInfo* info)
{
    info->player_num = ay_sys_detect(info);
    if(info->player_num < 0) {
        return false;
    }
    memset(info->module, 0, info->file_len);
    memcpy(info->module, info->file_data, info->file_len);
    info->init_proc = Players[info->player_num].soft_init_proc;
    info->play_proc = Players[info->player_num].soft_play_proc;
    info->cleanup_proc = Players[info->player_num].soft_cleanup_proc;
    return true;
}

void ay_sys_initz80module(AYSongInfo* info, unsigned long player_base, const unsigned char *player_ptr, unsigned long player_length, unsigned long player_init_proc, unsigned long player_play_proc)
{
    //fill z80 memory
    memset(info->module + 0x0000, 0xc9, 0x0100);
    memset(info->module + 0x0100, 0xff, 0x3f00);
    memset(info->module + 0x4000, 0x00, 0xc000);
    info->module[0x38] = 0xfb; /* ei */
    //copy player to 0xc000 of z80 memory
    memcpy(info->module + player_base, player_ptr, player_length);

    //copy module right after the player
    memcpy(info->module + player_base + player_length, info->file_data, info->file_len);
    //copy im1 loop to 0x0 of z80 memory
    memcpy(info->module, intnz, sizeof(intnz));

    info->module[2] = player_init_proc % 256;
    info->module[3] = player_init_proc / 256;
    info->module[9] = player_play_proc % 256;
    info->module[10] = player_play_proc / 256;
    z80ex_set_reg(info->z80ctx, regSP, 0xc000);
}

bool ay_sys_readfromfile(AYSongInfo* info)
{
    unsigned long data_len = 65536;
    info->timeElapsed = 0;
    info->Length = 0;
    info->init_proc = 0;
    info->play_proc = 0;
    if(info->file_data)
    {
        free(info->file_data);
        info->file_data = 0;
    }
    if(info->module)
    {
        free(info->module);
        info->module = 0;
    }
    info->file_data = osRead(info->FilePath, &data_len);
    if(!info->file_data)
        return false;

    info->file_len = data_len;
    info->module_len = data_len;

    unsigned long to_allocate = info->file_len < 65536 ? 65536 : info->file_len;
    info->module = (unsigned char*)malloc(to_allocate);
    if(!info->module)
    {
        free(info->file_data);
        info->file_data = 0;
        return false;
    }
    memset(info->module, 0, to_allocate);

    return true;
}

bool ay_sys_getsonginfoindirect(AYSongInfo* info)
{
    info->Length = 0;
    info->Loop = 0;
    info->Name = TXT("");
    info->Author = TXT("");
    info->player_num = ay_sys_detect(info);
    if(info->player_num < 0)
        return false;
    if(Players[info->player_num].getInfo)
    {
        Players[info->player_num].getInfo(info);
        return true;
    }
    return false;
}
bool ay_sys_getsonginfo(AYSongInfo* info)
{
    if(!ay_sys_readfromfile(info))
        return false;
    return ay_sys_getsonginfoindirect(info);
}

void ay_sys_rewindsong(AYSongInfo* info, long new_position)
{
    bool started = false;
    if(info->player && Started(info->player))
    {
        Stop(info->player);
        started = true;
    }

    if(info->timeElapsed > new_position)
    {
        info->timeElapsed = 0;
        ay_resetsong(info);
    }

    if(info->is_z80)
    {
        int z80_per_sample_counter = 0;
        int int_per_z80_counter = 0;
        float int_per_z80_f = (float)info->z80_freq / (float)info->int_freq;
        unsigned long int_per_z80 = int_per_z80_f;
        if((int_per_z80_f - int_per_z80) >= 0.5)
            int_per_z80++;
        while(info->timeElapsed != new_position)
        {
            int tstates = z80ex_step(info->z80ctx);
            z80_per_sample_counter += tstates;
            int_per_z80_counter += tstates;
            if(int_per_z80_counter > int_per_z80)
            {
                tstates = z80ex_int(info->z80ctx);
                z80_per_sample_counter += tstates;
                int_per_z80_counter = tstates;
                info->timeElapsed++;
            }
        }
    }
    else
    {
        while(info->timeElapsed != new_position)
        {
            info->play_proc(info);
            info->timeElapsed++;
        }
    }
    
    if(started)
        Start(info->player);
        
}

