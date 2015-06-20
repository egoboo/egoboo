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

/// @file    egolib/clock.h
/// @brief   clock & timer functionality
/// @details This implementation was adapted from Noel Lopis' article in
///          Game Programming Gems 4.

#pragma once

#include "egolib/typedef.h"


    struct ClockState_t;

	typedef struct tm* EGO_TIME;

#define PROFILE_KEEP  0.9F
#define PROFILE_NEW  (1.0F - PROFILE_KEEP)

#define PROFILE_DECLARE_STRUCT      ClockState_t * _clkstate = NULL; double _clkcount; double _clktime
#define PROFILE_INIT_STRUCT(XX,PTR) (PTR)->_clkstate  = clk_create(#XX,1);
#define PROFILE_FREE_STRUCT(PTR)    if (NULL != (PTR)->_clkstate) { clk_destroy(((PTR)->_clkstate)); ((PTR)->_clkstate) = NULL; }

#define PROFILE_DECLARE(XX)         static ClockState_t * clkstate_##XX = NULL; static double clkcount_##XX = 0.0F; static double clktime_##XX = 0.0F;
#define PROFILE_INIT(XX)            clkstate_##XX  = clk_create(#XX,1);
#define PROFILE_RESET(XX)           clkcount_##XX = 0.0; clktime_##XX = 0.0;
#define PROFILE_FREE(XX)            if (NULL != (clkstate_##XX)) { clk_destroy((clkstate_##XX)); (clkstate_##XX) = NULL; }

#if defined(DEBUG_PROFILE) && defined(_DEBUG)

#    define PROFILE_QUERY_STRUCT(PTR)   ( (double)(PTR)->_clktime / (double)(PTR)->_clkcount )
#    define PROFILE_BEGIN_STRUCT(PTR)  clk_frameStep((PTR)->_clkstate);
#    define PROFILE_END_STRUCT(PTR)    clk_frameStep((PTR)->_clkstate);   (PTR)->_clkcount = (PTR)->_clkcount*PROFILE_KEEP + PROFILE_NEW*1.0F; (PTR)->_clktime = (PTR)->_clktime*PROFILE_KEEP + PROFILE_NEW*clk_getFrameDuration((PTR)->_clkstate);
#    define PROFILE_END2_STRUCT(PTR)   clk_frameStep((PTR)->_clkstate);   (PTR)->_clkcount += 1.0F;  (PTR)->_clktime += clk_getFrameDuration((PTR)->_clkstate);

#    define PROFILE_QUERY(XX)   ( (double)clktime_##XX / (double)clkcount_##XX )
#    define PROFILE_BEGIN(XX)  clk_frameStep(clkstate_##XX);
#    define PROFILE_END(XX)    clk_frameStep(clkstate_##XX);   clkcount_##XX = clkcount_##XX*PROFILE_KEEP + PROFILE_NEW*1.0F; clktime_##XX = clktime_##XX*PROFILE_KEEP + PROFILE_NEW*clk_getFrameDuration(clkstate_##XX);
#    define PROFILE_END2(XX)   clk_frameStep(clkstate_##XX);   clkcount_##XX += 1.0F;  clktime_##XX += clk_getFrameDuration(clkstate_##XX);

#else

#    define PROFILE_QUERY_STRUCT(PTR)  1.0F
#    define PROFILE_BEGIN_STRUCT(PTR)
#    define PROFILE_END_STRUCT(PTR)    (PTR)->_clkcount  = 1.0F;
#    define PROFILE_END2_STRUCT(PTR)   (PTR)->_clkcount += 1.0F;

#    define PROFILE_QUERY(XX)  1.0F
#    define PROFILE_BEGIN(XX)
#    define PROFILE_END(XX)    clkcount_##XX  = 1.0F;
#    define PROFILE_END2(XX)   clkcount_##XX += 1.0F;

#endif


/// The description of a single clock
struct ClockState_t
{
	// Clock data
	char *name;

	std::chrono::high_resolution_clock::time_point sourceStartTime;  // The first value the clock receives from above function
	std::chrono::high_resolution_clock::time_point sourceLastTime;  // The last value the clock received from above function
	double currentTime;   // The current time, not necessarily in sync w/ the source time
	double frameTime;   // The time this frame takes
	Uint32 frameNumber; // Which frame the clock is on

	double maximumFrameTime; // The maximum time delta the clock accepts (default .2 seconds)

	// Circular buffer to hold frame histories
	double *frameHistory;
	size_t frameHistorySize;
	size_t frameHistoryWindow;
	size_t frameHistoryHead;

	static ClockState_t *ctor(ClockState_t *self, const char * name, size_t window_size);
	static void dtor(ClockState_t *self);
};

/**
 * @brief
 *	Create a clock.
 * @param name
 *	the name of the clock.
 *	An internal copy of the string is stored.
 * @param window_size
 *	the histogram size of the clock.
 * @return
 *	a pointer to the clock on success, @a NULL on failure
 * @pre
 *	NULL != name && window_size > 0
 * @remark
 *	A histogram of size @a n records the duration of @a n frames.
 *	The average frame duration can be computed from the recorded frame durations.
 */
ClockState_t *clk_create(const char *name,size_t window_size);

/**
 * @brief
 *	Destroy this clock.
 * @param self
 *	this clock
 */
void clk_destroy(ClockState_t *self);

ClockState_t *clk_renew( ClockState_t *self);

/**
 * @brief
 *	Update this clock.
 * @param self
 *	this clock
 */
void clk_frameStep(ClockState_t *self);
/**
 * @brief
 *	Get the current time.
 * @param self
 *	this clock
 * @return
 *	the current time of this clock
 * @remark
 *	The current time of a clock only updates, when clk_frameStep() is called.
 */
double clk_getTime(ClockState_t *self);
/**
 * @brief
 *	Get the length of the current frame of this clock.
 * @param self
 *	this clock
 * @return
 *	the length of the current frame of this clock
 */
double clk_getFrameDuration(ClockState_t *self);
/**
 * @brief
 *	Return which frame we are on.
 * @return
 *	the frame which we are on
 */
Uint32 clk_getFrameNumber(ClockState_t *self);
/**
 * @brief
 *	Return the current instantaneous frames per second.
 * @retrun
 *	the current instantaneous frames per second
 */
float clk_getFrameRate(ClockState_t *self);

    EGO_TIME getCurrentTime( void );                          ///< Returns a structure containing current time and date

    void clk_init( void );                                 ///< Init the clock module
    void clk_shutdown( void );                             ///< Shut down the clock module