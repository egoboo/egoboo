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

/// @file egolib/clock.c
/// @brief Clock & timer implementation
/// @details This implementation was adapted from Noel Lopis' article in Game Programming Gems 4.

#include <stddef.h>
#include <stdlib.h>

#include "egolib/clock.h"
#include "egolib/system.h"
#include "egolib/log.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The description of a single clock
struct s_ClockState
{
    // Clock data
    char *name;

    double sourceStartTime;  // The first value the clock receives from above function
    double sourceLastTime;  // The last value the clock received from above function
    double currentTime;   // The current time, not necessarily in sync w/ the source time
    double frameTime;   // The time this frame takes
    Uint32 frameNumber; // Which frame the clock is on

    double maximumFrameTime; // The maximum time delta the clock accepts (default .2 seconds)

    // Circular buffer to hold frame histories
    double *frameHistory;
    size_t frameHistorySize;
    size_t frameHistoryWindow;
    size_t frameHistoryHead;
};

static ClockState_t *clk_ctor(ClockState_t *self,const char * name,size_t window_size);
static void clk_dtor(ClockState_t *self);
static int clk_setFrameHistoryWindow(ClockState_t *self,size_t new_window_size);

static void   clk_addToFrameHistory( ClockState_t * cs, double frame );
static double clk_getExactLastFrameDuration( ClockState_t * cs );
static double clk_guessFrameDuration(const ClockState_t *self);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static clock_source_ptr_t _clock_timeSource = NULL;

static clock_source_ptr_t clock_getTimeSource( void )
{
    if ( NULL == _clock_timeSource )
    {
        clk_setTimeSource( sys_getTime );
    }

    return _clock_timeSource;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void clk_setTimeSource( clock_source_ptr_t tsrc )
{
    if ( NULL != tsrc )
    {
        _clock_timeSource = tsrc;
    }
}

//--------------------------------------------------------------------------------------------
void clk_init( void )
{
    log_info( "Initializing clock services...\n" );

    clock_getTimeSource();
}

//--------------------------------------------------------------------------------------------
void clk_shutdown( void )
{
    _clock_timeSource = NULL;
}

//--------------------------------------------------------------------------------------------
ClockState_t *clk_create(const char *name,size_t size)
{
    ClockState_t *self;
    self = EGOBOO_NEW(ClockState_t);
	if (!self)
	{
		return NULL;
	}
	if (!clk_ctor(self,name,size))
	{
		EGOBOO_DELETE(self);
		return NULL;
	}
	return self;
}

//--------------------------------------------------------------------------------------------
void clk_destroy(ClockState_t *self)
{
    clk_dtor(self);
    EGOBOO_DELETE(self);
}

//--------------------------------------------------------------------------------------------
static ClockState_t *clk_ctor(ClockState_t *self,const char * name,size_t window_size)
{
	EGOBOO_ASSERT(NULL != self && NULL != name && window_size > 0);
    clock_source_ptr_t psrc;
    BLANK_STRUCT_PTR(self)

    psrc = clock_getTimeSource();
    self->sourceStartTime = psrc();
    self->sourceLastTime  = self->sourceStartTime;

    self->maximumFrameTime = 0.2;
	self->name = strdup(name);
	if (!self->name)
	{
		/* @todo Do we have to release psrc? */
		return NULL;
	}
	self->frameHistoryHead = 0;
	self->frameHistory = EGOBOO_NEW_ARY(double,window_size);
	if (!self->frameHistory)
	{
		free(self->name);
		self->name = NULL;
		return NULL; /* @todo Do we have to release psrc? */
	}
	self->frameHistoryWindow = window_size;
    return self;
}

//--------------------------------------------------------------------------------------------
static void clk_dtor(ClockState_t *self)
{
	EGOBOO_ASSERT(NULL != self);
    EGOBOO_DELETE_ARY(self->frameHistory);
	free(self->name);
	BLANK_STRUCT_PTR(self);
}

//--------------------------------------------------------------------------------------------
ClockState_t *clk_renew(ClockState_t *self)
{
	clock_source_ptr_t psrc = clock_getTimeSource();
	self->sourceStartTime = psrc();
	self->sourceLastTime = self->sourceStartTime;
	self->maximumFrameTime = 0.2;
	self->frameHistoryHead = 0;
	return self;
}

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	Set the frame history window size.
 * @param self
 *	the clock
 * @param new_window_size
 *	the new frame history window size
 * @return
 *	@a 0 on success, a non-zero value on faiure
 * @pre
 *	new_window_size > 0
 * @post
 *	If this function fails, the state of the clock is not observably modified.
 */
int clk_setFrameHistoryWindow(ClockState_t *self,size_t new_window_size)
{
	EGOBOO_ASSERT(NULL != self && new_window_size > 0);
    double *history;
    size_t old_window_size;

    // Save the old size of the array
    old_window_size = self->frameHistoryWindow;

    // create the new array
    history = EGOBOO_NEW_ARY(double, new_window_size);
	if (!history)
	{
		return 1;
	}
	EGOBOO_ASSERT(NULL != history);
	memset(history, 0, sizeof(double) * new_window_size);

	EGOBOO_ASSERT(NULL != self->frameHistory);
	size_t smaller; /* @todo Use std::min. */

	// Copy over the older history. 
	// Make sure that only the size of the smaller buffer is copied
	smaller = (new_window_size < old_window_size) ? new_window_size : old_window_size;
    memcpy(history, self->frameHistory, smaller);

    EGOBOO_DELETE_ARY(self->frameHistory);

    self->frameHistoryHead   = 0;
    self->frameHistory       = history;
    self->frameHistoryWindow = new_window_size;
	return 0;
}

//--------------------------------------------------------------------------------------------

/**
* @brief
*	Guess the duration of a frame based on data recorded in the frame history window.
* @param self
*	the clock
* @return
*	the guessed duration of a frame.
*/
double clk_guessFrameDuration(const ClockState_t *self)
{
	EGOBOO_ASSERT(NULL != self);
    double time = 0;
    if (1 == self->frameHistorySize)
    {
        time = self->frameHistory[0];
    }
    else
    {
        double totalTime = 0;
        for (size_t c = 0; c < self->frameHistorySize; c++)
        {
            totalTime += self->frameHistory[c];
        }
        time = totalTime / self->frameHistorySize;
    }

    return time;
}

//--------------------------------------------------------------------------------------------
void clk_addToFrameHistory(ClockState_t *self,double frame_duration)
{
    self->frameHistory[self->frameHistoryHead] = frame_duration;

    self->frameHistoryHead++;
    if (self->frameHistoryHead >= self->frameHistoryWindow )
    {
        self->frameHistoryHead = 0;
    }

    self->frameHistorySize++;
    if (self->frameHistorySize > self->frameHistoryWindow )
    {
        self->frameHistorySize = self->frameHistoryWindow;
    }
}

//--------------------------------------------------------------------------------------------
double clk_getExactLastFrameDuration( ClockState_t * cs )
{
    clock_source_ptr_t psrc;
    double sourceTime;
    double timeElapsed;

    psrc = clock_getTimeSource();
    if ( NULL != psrc )
    {
        sourceTime = psrc();
    }
    else
    {
        sourceTime = 0;
    }

    timeElapsed = sourceTime - cs->sourceLastTime;
    // If more time elapsed than the maximum we allow, say that only the maximum occurred
    if ( timeElapsed > cs->maximumFrameTime )
    {
        timeElapsed = cs->maximumFrameTime;
    }

    cs->sourceLastTime = sourceTime;
    return timeElapsed;
}

//--------------------------------------------------------------------------------------------
void clk_frameStep( ClockState_t * cs )
{
	EGOBOO_ASSERT(NULL != cs);
    double lastFrame = clk_getExactLastFrameDuration( cs );
    clk_addToFrameHistory( cs, lastFrame );

    // This feels wrong to me; we're guessing at how long this
    // frame is going to be and accepting that as our time value.
    // I'll trust Mr. Lopis for now, but it may change.
    cs->frameTime = clk_guessFrameDuration( cs );
    cs->currentTime += cs->frameTime;

    cs->frameNumber++;
}

//--------------------------------------------------------------------------------------------
double clk_getTime( ClockState_t * cs )
{
    return cs->currentTime;
}

//--------------------------------------------------------------------------------------------
double clk_getFrameDuration( ClockState_t * cs )
{
    return cs->frameTime;
}

//--------------------------------------------------------------------------------------------
Uint32 clk_getFrameNumber( ClockState_t * cs )
{
    return cs->frameNumber;
}

//--------------------------------------------------------------------------------------------
float clk_getFrameRate( ClockState_t * cs )
{
    return ( float )( 1.0F / cs->frameTime );
}

//--------------------------------------------------------------------------------------------
EGO_TIME getCurrentTime( void )
{
    time_t rawtime = time( NULL );
    EGO_TIME timeinfo = localtime( &rawtime );

    return timeinfo;
}
