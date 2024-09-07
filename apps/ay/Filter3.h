/***************************************************************************
 *   Copyright (C) 2007 by Deryabin Andrew                                 *
 *   andrewderyabin@gmail.com                                              *
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
#ifndef FILTER3
#define FILTER3

typedef enum FilterType {
    LPF = 0,
    HPF,
    BPF
} FilterType;

typedef struct FilterOpts
{
    float Fs; //sample rate
    float f0; //center freq
    float Q; //Q
    FilterType type; //filter type
} FilterOpts;

typedef struct Filter3 {
    float _s0, _s1, _s2;
    float a0, a1, a2;
    float b0, b1, b2;
    float b0_a0, b1_a0, b2_a0;
    float a1_a0, a2_a0;
    float in_delay00, in_delay01, in_delay10, in_delay11, in_delay20, in_delay21;
    float out_delay00, out_delay01, out_delay10, out_delay11, out_delay20, out_delay21;
    float Fs;;
    float f0;
    float Q;
} Filter3;

inline static Filter3* new_Filter3(void) {
    return (Filter3*)calloc(1, sizeof(Filter3));
}
void delete_Filter3(Filter3* f) {
    free(f);
}
void Filter3Init(Filter3*, const FilterOpts *opts);

inline void Filter3Process2(Filter3* f, float* ps0, float* ps1) {
    f->_s0 = *ps0;
    f->_s1 = *ps1;

    *ps0 = f->b0_a0 * *ps0 + f->b1_a0 * f->in_delay01 + f->b2_a0 * f->in_delay00 - f->a1_a0 * f->out_delay01 - f->a2_a0 * f->out_delay00;
    *ps1 = f->b0_a0 * *ps1 + f->b1_a0 * f->in_delay11 + f->b2_a0 * f->in_delay10 - f->a1_a0 * f->out_delay11 - f->a2_a0 * f->out_delay10;

    f->in_delay00 = f->in_delay01;
    f->in_delay10 = f->in_delay11;
    f->out_delay00 = f->out_delay01;
    f->out_delay10 = f->out_delay11;

    f->in_delay01 = f->_s0;
    f->in_delay11 = f->_s1;
    f->out_delay01 = *ps0;
    f->out_delay11 = *ps1;
}

inline void Filter3Process3(Filter3* f, float* ps0, float* ps1, float* ps2) {
    f->_s0 = *ps0;
    f->_s1 = *ps1;
    f->_s2 = *ps2;

    *ps0 = f->b0_a0 * *ps0 + f->b1_a0 * f->in_delay01 + f->b2_a0 * f->in_delay00 - f->a1_a0 * f->out_delay01 - f->a2_a0 * f->out_delay00;
    *ps1 = f->b0_a0 * *ps1 + f->b1_a0 * f->in_delay11 + f->b2_a0 * f->in_delay10 - f->a1_a0 * f->out_delay11 - f->a2_a0 * f->out_delay10;
    *ps2 = f->b0_a0 * *ps2 + f->b1_a0 * f->in_delay21 + f->b2_a0 * f->in_delay20 - f->a1_a0 * f->out_delay21 - f->a2_a0 * f->out_delay20;

    f->in_delay00 = f->in_delay01;
    f->in_delay10 = f->in_delay11;
    f->in_delay20 = f->in_delay21;
    f->out_delay00 = f->out_delay01;
    f->out_delay10 = f->out_delay11;
    f->out_delay20 = f->out_delay21;

    f->in_delay01 = f->_s0;
    f->in_delay11 = f->_s1;
    f->in_delay21 = f->_s2;
    f->out_delay01 = *ps0;
    f->out_delay11 = *ps1;
    f->out_delay21 = *ps2;
}

inline void Filter3Process(Filter3* f, float* ps0) {
    f->_s0 = *ps0;
    *ps0 = f->b0_a0 * *ps0 + f->b1_a0 * f->in_delay01 + f->b2_a0 * f->in_delay00 - f->a1_a0 * f->out_delay01 - f->a2_a0 * f->out_delay00;
    f->in_delay00 = f->in_delay01;
    f->out_delay00 = f->out_delay01;
    f->in_delay01 = f->_s0;
    f->out_delay01 = *ps0;
}

#endif
