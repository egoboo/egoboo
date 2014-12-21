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

/// @file egolib/timer.c
/// @brief Implementation of a timer "class" using SDL_GetTicks().

#include "egolib/timer.h"
#include "egolib/_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_timer_t * egolib_timer__init( egolib_timer_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->free_running = false;
    ptr->ticks_lst = 0;
    ptr->ticks_now = 0;
    ptr->ticks_next = 0;
    ptr->ticks_diff = 0;

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool egolib_timer__throttle( egolib_timer_t * ptimer, float rate )
{
    /// @author BB
    /// @details return true if the process should proceed, false, otherwise

    int ticks;

    if ( NULL == ptimer ) return false;

    // the current time
    ticks  = SDL_GetTicks();

    // the elapsed time since the last update
    ptimer->ticks_diff = ticks - ptimer->ticks_lst;

    // return if not enough time has elapsed
    if ( !ptimer->free_running && ( ticks <= ptimer->ticks_next ) ) return false;

    // set the next update
    egolib_timer__reset( ptimer, ticks, rate );

    return true;
}

//--------------------------------------------------------------------------------------------
bool egolib_timer__reset( egolib_timer_t * ptimer, int ticks, float rate )
{
    if ( NULL == ptimer ) return false;

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

    return true;
}
