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

#include "egolib/clock.h"
#include "egolib/log.h"
#include "game/Core/GameEngine.hpp"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------



static void   clk_addToFrameHistory( ClockState_t * cs, double frame );
static double clk_getExactLastFrameDuration( ClockState_t * cs );
static double clk_guessFrameDuration(const ClockState_t *self);

//--------------------------------------------------------------------------------------------
void clk_init( void )
{
    log_info( "Initializing clock services...\n" );
}

//--------------------------------------------------------------------------------------------
void clk_shutdown( void )
{

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
	if (!ClockState_t::ctor(self,name,size))
	{
		EGOBOO_DELETE(self);
		return NULL;
	}
	return self;
}

//--------------------------------------------------------------------------------------------
void clk_destroy(ClockState_t *self)
{
    ClockState_t::dtor(self);
    EGOBOO_DELETE(self);
}

//--------------------------------------------------------------------------------------------
ClockState_t *ClockState_t::ctor(ClockState_t *self,const char * name,size_t window_size)
{
	EGOBOO_ASSERT(NULL != self && NULL != name && window_size > 0);
    BLANK_STRUCT_PTR(self)

    self->sourceStartTime = std::chrono::high_resolution_clock::now();
    self->sourceLastTime  = self->sourceStartTime;

    self->maximumFrameTime = 0.2;
	self->name = strdup(name);
	if (!self->name)
	{
		return NULL;
	}
	self->frameHistoryHead = 0;
	self->frameHistory = EGOBOO_NEW_ARY(double,window_size);
	if (!self->frameHistory)
	{
		free(self->name);
		self->name = NULL;
		return NULL;
	}
	self->frameHistoryWindow = window_size;
    return self;
}

//--------------------------------------------------------------------------------------------
void ClockState_t::dtor(ClockState_t *self)
{
	EGOBOO_ASSERT(NULL != self);
    EGOBOO_DELETE_ARY(self->frameHistory);
	free(self->name);
	BLANK_STRUCT_PTR(self);
}

//--------------------------------------------------------------------------------------------
ClockState_t *clk_renew(ClockState_t *self)
{
	self->sourceStartTime = std::chrono::high_resolution_clock::now();
	self->sourceLastTime = self->sourceStartTime;
	self->maximumFrameTime = 0.2;
	self->frameHistoryHead = 0;
	return self;
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
    auto sourceTime = std::chrono::high_resolution_clock::now();

    double timeElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(sourceTime - cs->sourceLastTime).count();

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
