/***************************************************************************
 *   Copyright (C) 2008 by Deryabin Andrew                                 *
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
#include <stdbool.h>

AYSongInfo *ay_sys_getnewinfo()
{
//    AYSongInfo *info = (AYSongInfo*)calloc(1, sizeof(AYSongInfo));
    AYSongInfo *info = new_AYSongInfo();
    if(!info)
        return 0;
    info->FilePath = TXT("");
    info->Name = TXT("");
    info->Author = TXT("");
    info->Length = info->Loop = 0;
    info->is_z80 = false;
    info->init_proc = 0;
    info->play_proc = 0;
    info->cleanup_proc = 0;
    info->data = 0;
    info->data1 = 0;
    info->file_len = 0;
    info->module_len = 0;
    info->z80ctx = 0;
    info->timeElapsed = 0;
    info->e_callback = 0;
    info->e_callback_arg = 0;
    info->s_callback = 0;
    info->s_callback_arg = 0;
    info->z80_freq = Z80_FREQ;
    info->ay_freq = info->z80_freq / 2;
    info->int_freq = INT_FREQ;
    info->file_data = info->module = info->module1 = 0;
    info->sr = 44100;
    info->player = 0;
    info->chip_type = 0;
    info->own_player = true;
    info->stopping = false;
    info->player_num = -1;
    info->int_counter = 0;
    info->int_limit = 0;
    info->is_ts = false;
    info->mix_levels_nr = AY_ABC;
    info->ay_oversample = 1;
	info->empty_song = false;
	info->empty_callback = 0;
	info->aywrite_callback = 0;
    gouta("info2\n");
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        info->pay8910[i] = new_ay();
		info->pay8910[i]->chip_nr = i;
        aySetParameters(info->pay8910[i], info);
    }
    gouta("info3\n");
//    memset(info->z80IO, 0, 65536);
    return info;
}

void *ay_initsong(const AY_CHAR *FilePath, unsigned long sr, MOSAudio_t* player) {
    gouta("1\n");
    AYSongInfo *info = ay_sys_getnewinfo();
    gouta("2\n");
    if(!info)
        return 0;
    gouta("3\n");

    info->FilePath = FilePath;
    info->sr = sr;
    if(player)
    {
        info->player = player;
        info->own_player = false;
        SetSongInfo(player, info);
    }
    else
    {
        info->player = new_MOSAudio(info);
    }

    if(!ay_sys_readfromfile(info))
    {
        delete_AYSongInfo(info);
        info = 0;
    }
    else
    {
        if(!ay_sys_initsong(info))
        {
            delete_AYSongInfo(info);
            info = 0;
        }
        else
        {
            if(info->init_proc)
                info->init_proc(info);
            ay_sys_getsonginfoindirect(info);
        }
    }
    return info;
}

AYFLY_API void *ay_initsongindirect(unsigned char *module, unsigned long sr, unsigned long size, MOSAudio_t *player)
{
    AYSongInfo *info = ay_sys_getnewinfo();
    if(!info)
        return 0;
    info->file_len = size;
    info->module_len = size;
    unsigned long to_allocate = size < 65536 ? 65536 : size;
    info->file_data = (unsigned char*)malloc(to_allocate);
    if(!info->file_data)
    {
        delete_AYSongInfo(info);
        return 0;
    }
    memset(info->file_data, 0, to_allocate);
    memcpy(info->file_data, module, size);
    info->module = (unsigned char*)malloc(to_allocate);
    if(!info->module)
    {
        delete_AYSongInfo(info);
        return 0;
    }
    info->sr = sr;
    if(player)
    {
        info->player = player;
        info->own_player = false;
        SetSongInfo(player, info);
    }
    else
    {
        info->player = new_MOSAudio(info);
    }
    if(!ay_sys_initsong(info))
    {
        delete_AYSongInfo(info);
        info = 0;
    }
    else
    {
        if(info->init_proc)
            info->init_proc(info);
        ay_sys_getsonginfoindirect(info);
    }
    return info;
}

AYFLY_API void *ay_getsonginfo(const AY_CHAR *FilePath)
{
    AYSongInfo *info = ay_sys_getnewinfo();
    if(!info)
        return 0;
    info->FilePath = FilePath;
    if(!ay_sys_getsonginfo(info))
    {
        delete_AYSongInfo(info);
        info = 0;
    }

    return info;
}

AYFLY_API void *ay_getsonginfoindirect(unsigned char *module, AY_CHAR *type, unsigned long size)
{
    AYSongInfo *info = ay_sys_getnewinfo();
    if(!info)
        return 0;
    info->FilePath = type;
    unsigned long to_allocate = size < 65536 ? 65536 : size;
    info->file_data = (unsigned char*)malloc(to_allocate);
    if(!info->file_data)
    {
        delete_AYSongInfo(info);
        return 0;
    }
    memset(info->file_data, 0, to_allocate);
    memcpy(info->file_data, module, size);
    info->module = (unsigned char*)malloc(to_allocate);
    if(!info->module)
    {
        delete_AYSongInfo(info);
        return 0;
    }
    memcpy(info->file_data, module, size);
    if(!ay_sys_getsonginfoindirect(info))
    {
        delete_AYSongInfo(info);
        info = 0;
    }
    return info;
}

AYFLY_API const AY_CHAR *ay_getsongname(void *info)
{
    return c_str( ((AYSongInfo *)info)->Name );
}

AYFLY_API const AY_CHAR *ay_getsongauthor(void *info)
{
    return c_str( ((AYSongInfo *)info)->Author );
}

AYFLY_API const AY_CHAR *ay_getsongpath(void *info)
{
    return c_str( ((AYSongInfo *)info)->FilePath );
}

AYFLY_API void ay_seeksong(void *info, long new_position)
{
    ((AYSongInfo *)info)->stopping = false;
    ay_sys_rewindsong((AYSongInfo *)info, new_position);
}

AYFLY_API void ay_resetsong(void *info)
{
    AYSongInfo *song = (AYSongInfo *)info;
    if(!song->player)
        return;
    song->stopping = false;
    bool started = Started(song->player);
    if(started)
        Stop(song->player);
    song->timeElapsed = 0;
    ay_sys_initsong(song);

    if(song->init_proc)
        song->init_proc(song);
}

AYFLY_API void ay_closesong(void **info)
{
    AYSongInfo *song = (AYSongInfo *)*info;
    AYSongInfo **ppsong = (AYSongInfo **)info;
    delete_AYSongInfo(song);
    *ppsong = 0;
}

AYFLY_API void ay_setvolume(void *info, unsigned long chnl, float volume, unsigned char chip_num)
{
    aySetVolume(((AYSongInfo *)info)->pay8910[chip_num], chnl, volume);
}
AYFLY_API float ay_getvolume(void *info, unsigned long chnl, unsigned char chip_num)
{
    return ayGetVolume( ((AYSongInfo *)info)->pay8910[chip_num], chnl);
}

AYFLY_API void ay_chnlmute(void *info, unsigned long chnl, bool mute, unsigned char chip_num)
{
    ayChnlMute(((AYSongInfo *)info)->pay8910[chip_num], chnl, mute);
}

AYFLY_API bool ay_chnlmuted(void *info, unsigned long chnl, unsigned char chip_num)
{
    return ayChnlMuted(((AYSongInfo *)info)->pay8910[chip_num], chnl);
}

AYFLY_API void ay_setmixtype(void *info, AYMixTypes mixType, unsigned char chip_num)
{
    aySetMixType(((AYSongInfo *)info)->pay8910[chip_num], mixType);
}

AYFLY_API AYMixTypes ay_getmixtype(void *info, unsigned char chip_num)
{
    return ayGetMixType(((AYSongInfo *)info)->pay8910[chip_num]);
}

AYFLY_API void ay_setelapsedcallback(void *info, ELAPSED_CALLBACK callback, void *callback_arg)
{
    ((AYSongInfo *)info)->e_callback_arg = callback_arg;
    ((AYSongInfo *)info)->e_callback = callback;
}

AYFLY_API void ay_setstoppedcallback(void *info, STOPPED_CALLBACK callback, void *callback_arg)
{
    ((AYSongInfo *)info)->s_callback_arg = callback_arg;
    ((AYSongInfo *)info)->s_callback = callback;
}

AYFLY_API bool ay_songstarted(void *info)
{
    return ((AYSongInfo *)info)->player ? Started(((AYSongInfo *)info)->player) : 0;
}

AYFLY_API void ay_startsong(void *info)
{
    ((AYSongInfo *)info)->stopping = false;
    if(!ay_songstarted(info))
        Start(((AYSongInfo *)info)->player);
}

AYFLY_API void ay_stopsong(void *info)
{
    ((AYSongInfo *)info)->stopping = false;
    if(ay_songstarted(info))
    {
        Stop(((AYSongInfo *)info)->player);
        while(Started(((AYSongInfo *)info)->player))
            ; //very bad :(
    }
}

AYFLY_API unsigned long ay_getsonglength(void *info)
{
    return ((AYSongInfo *)info)->Length;
}

AYFLY_API unsigned long ay_getelapsedtime(void *info)
{
    return ((AYSongInfo *)info)->timeElapsed;
}

AYFLY_API unsigned long ay_getsongloop(void *info)
{
    return ((AYSongInfo *)info)->Loop;
}

AYFLY_API const unsigned char *ay_getregs(void *info, unsigned char chip_num)
{
    return ayGetRegs(((AYSongInfo *)info)->pay8910[chip_num]);
}

AYFLY_API unsigned long ay_rendersongbuffer(void *info, unsigned char *buffer, unsigned long buffer_length)
{
    return ayProcess(((AYSongInfo *)info)->pay8910[0], buffer, buffer_length);
}

AYFLY_API unsigned long ay_getz80freq(void *info)
{
    return ((AYSongInfo *)info)->z80_freq;
}
AYFLY_API void ay_setz80freq(void *info, unsigned long z80_freq)
{
    ((AYSongInfo *)info)->z80_freq = z80_freq;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}
AYFLY_API unsigned long ay_getayfreq(void *info)
{
    return ((AYSongInfo *)info)->ay_freq;
}
AYFLY_API void ay_setayfreq(void *info, unsigned long ay_freq)
{
    ((AYSongInfo *)info)->ay_freq = ay_freq;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}
AYFLY_API float ay_getintfreq(void *info)
{
    return ((AYSongInfo *)info)->int_freq;
}

AYFLY_API void ay_setintfreq(void *info, float int_freq)
{
    ((AYSongInfo *)info)->int_freq = int_freq;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}

AYFLY_API unsigned long ay_getsamplerate(void *info)
{
    return ((AYSongInfo *)info)->sr;
}

AYFLY_API void ay_setsamplerate(void *info, unsigned long sr)
{
    ((AYSongInfo *)info)->sr = sr;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}

AYFLY_API void ay_setsongplayer(void *info, void * /* MOSAudio_t */player)
{
    if(((AYSongInfo *)info)->player)
    {
        ay_stopsong(info);
        if(((AYSongInfo *)info)->own_player)
        {
            delete_MOSAudio( ((AYSongInfo *)info)->player );
            ((AYSongInfo *)info)->player = 0;
        }
    }
    ((AYSongInfo *)info)->player = (MOSAudio_t *)player;
}

AYFLY_API void *ay_getsongplayer(void *info)
{
    return ((AYSongInfo *)info)->player;
}

AYFLY_API void ay_setchiptype(void *info, unsigned char chip_type)
{
    ((AYSongInfo *)info)->chip_type = chip_type;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}

AYFLY_API unsigned char ay_getchiptype(void *info)
{
    return ((AYSongInfo *)info)->chip_type;
}

AYFLY_API void ay_writeay(void *info, unsigned char reg, unsigned char val, unsigned char chip_num)
{
    ayWrite(((AYSongInfo *)info)->pay8910[chip_num], reg, val);
}

AYFLY_API unsigned char ay_readay(void *info, unsigned char reg, unsigned char chip_num)
{
    return ayRead(((AYSongInfo *)info)->pay8910[chip_num], reg);
}

AYFLY_API void ay_resetay(void *info, unsigned char chip_num)
{
    ayReset(((AYSongInfo *)info)->pay8910[chip_num]);
    aySetParameters(((AYSongInfo *)info)->pay8910[chip_num], (AYSongInfo *)info);

}

AYFLY_API void ay_softexec(void *info)
{
    AYSongInfo *song = (AYSongInfo *)info;
    song->play_proc(song);

    song->int_counter = 0;

    if(++song->timeElapsed >= song->Length)
    {
        song->timeElapsed = song->Loop;
        if(song->e_callback)
            song->stopping = song->e_callback(song->e_callback_arg);
    }
}

AYFLY_API void ay_z80exec(void *info)
{
    AYSongInfo *song = (AYSongInfo *)info;
    song->int_counter += z80ex_step(song->z80ctx);
    //printf("counter = %d\n", song->int_counter);    
    if(song->int_counter > song->int_limit)
    {
        song->int_counter -= song->int_limit;
        //printf("addr = %d\n", z80ex_get_reg(song->z80ctx, regPC));
        song->int_counter += z80ex_int(song->z80ctx);
        if(++song->timeElapsed >= song->Length)
        {
            song->timeElapsed = song->Loop;
            if(song->e_callback)
                song->stopping = song->e_callback(song->e_callback_arg);
        }
    }

}

delete_AYSongInfo(AYSongInfo* a)
{
    if(a->player && Started(a->player))
    {
        Stop(a->player);
    }
    if(a->cleanup_proc)
    {
        cleanup_proc(a);
    }
    if(a->own_player)
    {
        if(a->player)
        {
            delete_MOSAudio(a->player);
            a->player = 0;
        }
    }
    ay_sys_shutdownz80(a);
    if(a->module)
    {
        free(a->module);
        a->module = 0;
    }
    if(a->file_data)
    {
        free(a->file_data);
        a->file_data = 0;
    }
	for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        delete_ay(a->pay8910[i]);
    }
}

AYFLY_API bool ay_format_supported(AY_TXT_TYPE filePath)
{
    return ay_sys_format_supported(filePath);
}

AYFLY_API void ay_setoversample(void *info, unsigned long factor)
{
    ((AYSongInfo *)info)->ay_oversample = factor;
    for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(((AYSongInfo *)info)->pay8910[i], (AYSongInfo *)info);
    }
}

AYFLY_API unsigned long ay_getoversample(void *info)
{
    return ((AYSongInfo *)info)->ay_oversample;
}


AYFLY_API void *ay_initemptysong(unsigned long sr, EMPTY_CALLBACK callback)
{
	AYSongInfo *info = ay_sys_getnewinfo();
	if(!info)
		return 0;
	info->empty_song = true;
	info->sr = sr;
	info->empty_callback = callback;
    info->player = new_MOSAudio(info);
	for(unsigned char i = 0; i < NUMBER_OF_AYS; i++)
    {
        aySetParameters(info->pay8910[i], info);
    }
	return info;
}

AYFLY_API void ay_setaywritecallback(void *info, AYWRITE_CALLBACK callback)
{
	((AYSongInfo *)info)->aywrite_callback = callback;

}

AYFLY_API bool ay_ists(void *info)
{
	return ((AYSongInfo *)info)->is_ts;
}
