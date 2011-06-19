
#pragma once

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
/// @brief Definitions of a clock "class" using SDL_GetTicks()
/// @details

#include "egoboo_typedef.h"

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
typedef struct s_clock egoboo_clock_t;

#define EGOBOO_CLOCK_INIT { -1 }

bool_t clock_reset( egoboo_clock_t * pclock );

bool_t clock_update( egoboo_clock_t * pclock );
bool_t clock_update_1( egoboo_clock_t * pclock, int granularity );
bool_t clock_update_diff( egoboo_clock_t * pclock, int diff );

