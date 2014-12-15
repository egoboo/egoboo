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

/// @file egolib/throttle.c
/// @brief Implementation of a clock "class" using SDL_GetTicks()
/// @details

#include <SDL.h>

#include "egolib/throttle.h"
#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_throttle_reset( egolib_throttle_t * pthrottle )
{
    if ( NULL == pthrottle ) return C_FALSE;

    pthrottle->ticks_stt = SDL_GetTicks();
    pthrottle->ticks_lst = pthrottle->ticks_stt;
    pthrottle->ticks_now = pthrottle->ticks_stt;

    pthrottle->time_lst = 0;
    pthrottle->time_now = 0;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_throttle_update( egolib_throttle_t * pthrottle )
{
    if ( NULL == pthrottle ) return C_FALSE;

    if ( -1 == pthrottle->ticks_stt )
    {
        egolib_throttle_reset( pthrottle );
        return C_FALSE;
    }

    pthrottle->ticks_lst = pthrottle->ticks_now;
    pthrottle->ticks_now = SDL_GetTicks();

    pthrottle->time_lst  = pthrottle->time_now;
    pthrottle->time_now += pthrottle->ticks_now - pthrottle->ticks_lst;

    return C_TRUE;

}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_throttle_update_1( egolib_throttle_t * pthrottle, int granularity )
{
    int ticks, diff;

    if ( NULL == pthrottle ) return C_FALSE;

    if ( -1 == pthrottle->ticks_stt )
    {
        egolib_throttle_reset( pthrottle );
        return C_FALSE;
    }

    ticks = SDL_GetTicks();

    diff  = ticks - pthrottle->ticks_lst;
    if ( diff < granularity ) return C_FALSE;

    pthrottle->ticks_lst = pthrottle->ticks_now;
    pthrottle->ticks_now = ticks;

    pthrottle->time_lst  = pthrottle->time_now;
    pthrottle->time_now += diff;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_throttle_update_diff( egolib_throttle_t * pthrottle, int diff )
{
    if ( NULL == pthrottle ) return C_FALSE;

    if ( -1 == pthrottle->ticks_stt )
    {
        egolib_throttle_reset( pthrottle );
        return C_FALSE;
    }

    // let the ticks proceed as normal
    pthrottle->ticks_lst = pthrottle->ticks_now;
    pthrottle->ticks_now = SDL_GetTicks();

    // advance the clock by diff, no matter what is happening with the ticks
    pthrottle->time_lst  = pthrottle->time_now;
    pthrottle->time_now += diff;

    return C_TRUE;
}
