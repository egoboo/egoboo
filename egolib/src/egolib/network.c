//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file    egolib/network.c
/// @brief   Egoboo networking implementation.
/// @details Shuttles bits across the network, using Enet.
///          Networked play doesn't really work at the moment.

#include "egolib/network.h"

void tlatch_ary_init( time_latch_t ary[], size_t len )
{
    size_t cnt;

    if ( NULL == ary || 0 == len ) return;

    for ( cnt = 0; cnt < len; cnt++ )
    {
        ary[cnt].x      = 0;
        ary[cnt].y      = 0;
        ary[cnt].button = 0;
        ary[cnt].time   = ( Uint32 )( ~0 );
    }
}
