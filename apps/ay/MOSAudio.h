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

#ifndef AUDIO_H_
#define AUDIO_H_

typedef struct MOSAudio {
    bool started;
    struct AYSongInfo* songinfo;
} MOSAudio_t;

MOSAudio_t* new_MOSAudio(struct AYSongInfo *info);

void delete_MOSAudio(MOSAudio_t* a);

inline static bool Start(MOSAudio_t* a) {
    printf("Start\n");
}
inline static void Stop(MOSAudio_t* a) {
    printf("Start\n");
}
inline static bool Started(MOSAudio_t* aa) {
    printf("Started?\n");
    return aa->started;
}
inline static void SetSongInfo(MOSAudio_t* aa, struct AYSongInfo *info) {
    aa->songinfo = info;
}


#endif /*AUDIO_H_*/
