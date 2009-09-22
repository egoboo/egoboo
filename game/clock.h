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

///
/// @file
/// @brief Clock & timer functionality.
/// @details This implementation was adapted from Noel Lopis' article in
///   Game Programming Gems 4.

#include <SDL_types.h>
#include "egoboo_typedef.h"

struct s_ClockState;

typedef double( *clock_source_ptr_t )( void );
typedef struct s_ClockState ClockState_t;

void clk_init( void );                                 ///< Init the clock module
void clk_shutdown( void );                             ///< Shut down the clock module
//void clk_setTimeSource( clock_source_ptr_t tsrc );     ///< Specify where the clock gets its time values from

ClockState_t * clk_create( const char * name, int size );
bool_t         clk_destroy( ClockState_t ** cs );
ClockState_t * clk_renew( ClockState_t * cs );

void   clk_frameStep( ClockState_t * cs );          ///< Update the clock.
double clk_getTime( ClockState_t * cs );            ///< Returns the current time.  The clock's time only updates when clk_frameStep() is called
double clk_getFrameDuration( ClockState_t * cs );   ///< Return the length of the current frame. (Sort of.)
Uint32 clk_getFrameNumber( ClockState_t * cs );     ///< Return which frame we're on
float  clk_getFrameRate( ClockState_t * cs );       ///< Return the current instantaneous FPS

//-----------------------------------------------------------------
// macros to use the high resolution timer for profiling

#define PROFILE_KEEP  0.9
#define PROFILE_NEW  (1.0 - PROFILE_KEEP)

#ifdef DEBUG_PROFILE

#    define PROFILE_DECLARE(XX) ClockState_t * clkstate_##XX = NULL; double clkcount_##XX = 0.0; double clktime_##XX = 0.0;
#    define PROFILE_INIT(XX)    { clkstate_##XX  = clk_create(#XX, -1); }
#    define PROFILE_FREE(XX)    { clk_destroy(&(clkstate_##XX)); }
#    define PROFILE_QUERY(XX)   ( (double)clktime_##XX / (double)clkcount_##XX )

#    define PROFILE_BEGIN(XX)  clk_frameStep(clkstate_##XX);
#    define PROFILE_END(XX)    clk_frameStep(clkstate_##XX);   clkcount_##XX = clkcount_##XX*PROFILE_KEEP + PROFILE_NEW*1.0; clktime_##XX = clktime_##XX*PROFILE_KEEP + PROFILE_NEW*clk_getFrameDuration(clkstate_##XX);
#    define PROFILE_END2(XX)   clk_frameStep(clkstate_##XX);   clkcount_##XX += 1.0;  clktime_##XX += clk_getFrameDuration(clkstate_##XX);

#else

#    define PROFILE_DECLARE(XX) ClockState_t * clkstate_##XX = NULL; double clkcount_##XX = 0.0; double clktime_##XX = 0.0;
#    define PROFILE_INIT(XX)    { clkstate_##XX  = clk_create(#XX, -1); }
#    define PROFILE_FREE(XX)    { clk_destroy(&(clkstate_##XX)); }
#    define PROFILE_QUERY(XX)   1.0

#    define PROFILE_BEGIN(XX)
#    define PROFILE_END(XX)    clkcount_##XX  = 1.0;
#    define PROFILE_END2(XX)   clkcount_##XX += 1.0;

#endif
