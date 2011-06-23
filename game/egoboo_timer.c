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

/// @file egoboo_timer.c
/// @brief Implementation of a timer "class" using SDL_GetTicks()
/// @details

#include "egoboo_timer.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
bool_t ego_timer_throttle( ego_timer_t * ptimer, float rate )
{
    /// BB@> return btrue if the process should proceed, bfalse, otherwise

    int ticks;

    if ( NULL == ptimer ) return bfalse;

    // the current time
    ticks  = SDL_GetTicks();

    // the elapsed time since the last update
    ptimer->ticks_diff = ticks - ptimer->ticks_lst;

    // return if not enough time has elapsed
    if ( !ptimer->free_running && ( ticks <= ptimer->ticks_next ) ) return bfalse;

    // set the next update
    timer_reset( ptimer, ticks, rate );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t timer_reset( ego_timer_t * ptimer, int ticks, float rate )
{
    if ( NULL == ptimer ) return bfalse;

    if ( ticks < 0 )
    {
        ticks = SDL_GetTicks();
    }

    // update the timers
    ptimer->ticks_lst  = ptimer->ticks_now;
    ptimer->ticks_now  = ticks;
    ptimer->ticks_next = ticks + CEIL( TICKS_PER_SEC / rate );

    // determine the elapsed time
    ptimer->ticks_diff = ptimer->ticks_now - ptimer->ticks_lst;

    return btrue;
}
