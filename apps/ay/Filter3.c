/***************************************************************************
 *   Copyright (C) 2007 by Deryabin Andrew   				   *
 *   andrewderyabin@gmail.com   					   *
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
#ifdef M_API_VERSION

#else

#include "ayfly.h"
#include <math.h>

#endif

#ifndef PI
#define PI 3.1415926535
#endif

#include "Filter3.h"

void Filter3Init(Filter3* f, const FilterOpts *opts)
{
    if(f->Fs == opts->Fs && f->f0 == opts->f0 && f->Q == opts->Q)
        return;
    f->Fs = opts->Fs;
    f->f0 = opts->f0;
    f->Q = opts->Q;
    float w0 = 2 * PI * f->f0 / f->Fs;

    f->in_delay00 = f->in_delay01 = f->in_delay10 = f->in_delay11 = f->in_delay20 = f->in_delay21 = 0;
    f->out_delay00 = f->out_delay01 = f->out_delay10 = f->out_delay11 = f->out_delay20 = f->out_delay21 = 0;

    float alpha = sin(w0) / (2 * f->Q);

    if ( opts->type == LPF ) {
            f->b0 = (1 - cos(w0)) / 2;
            f->b1 = 1 - cos(w0);
            f->b2 = (1 - cos(w0)) / 2;
            f->a0 = 1 + alpha;
            f->a1 = -2 * cos(w0);
            f->a2 = 1 - alpha;
    }
    else if ( opts->type == HPF ) {
            f->b0 = (1 + cos(w0)) / 2;
            f->b1 = -(1 + cos(w0));
            f->b2 = (1 + cos(w0)) / 2;
            f->a0 = 1 + alpha;
            f->a1 = -2 * cos(w0);
            f->a2 = 1 - alpha;
    }
    else if ( opts->type == BPF ) {
            f->b0 = sin(w0) / 2;
            f->b1 = 0;
            f->b2 = -sin(w0) / 2;
            f->a0 = 1 + alpha;
            f->a1 = -2 * cos(w0);
            f->a2 = 1 - alpha;
    }
    else {
            f->b0 = f->b1 = f->b2 = f->a0 = f->a1 = f->a2 = 0;
            f->b0_a0 = f->b1_a0 = f->b2_a0 = f->a1_a0 = f->a2_a0 = 0;
            return;
    }

    f->b0_a0 = f->b0 / f->a0;
    f->b1_a0 = f->b1 / f->a0;
    f->b2_a0 = f->b2 / f->a0;
    f->a1_a0 = f->a1 / f->a0;
    f->a2_a0 = f->a2 / f->a0;
}
