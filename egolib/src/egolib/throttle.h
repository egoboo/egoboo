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

/// @file  egolib/throttle.h
/// @brief Definitions of a clock "class" using SDL_GetTicks()
/// @details

#pragma once

#include "egolib/typedef.h"

#if 0
#if defined(__cplusplus)
extern "C"
{
#endif
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_clock;
    typedef struct s_clock egolib_throttle_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a method for throttling processes using SDL_GetTicks()
    struct s_clock
    {
        int ticks_stt;
        int ticks_lst;
        int ticks_now;

        int time_lst;
        int time_now;
    };

    bool egolib_throttle_reset( egolib_throttle_t * pclock );

    bool egolib_throttle_update( egolib_throttle_t * pclock );
    bool egolib_throttle_update_1( egolib_throttle_t * pclock, int granularity );
    bool egolib_throttle_update_diff( egolib_throttle_t * pclock, int diff );

#define EGOLIB_THROTTLE_INIT { -1, 0, 0, 0, 0 }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if 0
#if defined(__cplusplus)
}
#endif
#endif