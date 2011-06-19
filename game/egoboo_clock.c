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

/// @file egoboo_clock.c
/// @brief Implementation of a clock "class" using SDL_GetTicks()
/// @details

#include "egoboo_clock.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
bool_t clock_reset( egoboo_clock_t * pclock )
{
    if ( NULL == pclock ) return bfalse;

    pclock->ticks_stt = SDL_GetTicks();
    pclock->ticks_lst = pclock->ticks_stt;
    pclock->ticks_now = pclock->ticks_stt;

    pclock->time_lst = 0;
    pclock->time_now = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t clock_update( egoboo_clock_t * pclock )
{
    if ( NULL == pclock ) return bfalse;

    if ( -1 == pclock->ticks_stt )
    {
        clock_reset( pclock );
        return bfalse;
    }

    pclock->ticks_lst = pclock->ticks_now;
    pclock->ticks_now = SDL_GetTicks();

    pclock->time_lst  = pclock->time_now;
    pclock->time_now += pclock->ticks_now - pclock->ticks_lst;

    return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t clock_update_1( egoboo_clock_t * pclock, int granularity )
{
    int ticks, diff;

    if ( NULL == pclock ) return bfalse;

    if ( -1 == pclock->ticks_stt )
    {
        clock_reset( pclock );
        return bfalse;
    }

    ticks = SDL_GetTicks();

    diff  = ticks - pclock->ticks_lst;
    if ( diff < granularity ) return bfalse;

    pclock->ticks_lst = pclock->ticks_now;
    pclock->ticks_now = ticks;

    pclock->time_lst  = pclock->time_now;
    pclock->time_now += diff;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t clock_update_diff( egoboo_clock_t * pclock, int diff )
{
    if ( NULL == pclock ) return bfalse;

    if ( -1 == pclock->ticks_stt )
    {
        clock_reset( pclock );
        return bfalse;
    }

    // let the ticks proceed as normal
    pclock->ticks_lst = pclock->ticks_now;
    pclock->ticks_now = SDL_GetTicks();

    // advance the clock by diff, no matter what is happening with the ticks
    pclock->time_lst  = pclock->time_now;
    pclock->time_now += diff;

    return btrue;
}
