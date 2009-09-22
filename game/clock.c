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
/// @brief Clock & timer implementation
/// @details This implementation was adapted from Noel Lopis' article in
///  Game Programming Gems 4.

#include "Clock.h"

#include "System.h"
#include "Log.h"
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

static clock_source_ptr_t _clock_timeSource = NULL;

static clock_source_ptr_t clock_getTimeSource()
{
  if(NULL == _clock_timeSource)
  {
    _clock_timeSource = sys_getTime;
  }

  return _clock_timeSource;
}

void clk_init()
{
  log_info( "Initializing clock services...\n" );

  clock_getTimeSource();
}

void clk_shutdown()
{
  _clock_timeSource = NULL;
}

// Clock data
struct s_ClockState
{
  // Clock data
  EGO_CONST char * name;

  double sourceStartTime;  // The first value the clock receives from above function
  double sourceLastTime;  // The last value the clock received from above function
  double currentTime;   // The current time, not necessarily in sync w/ the source time
  double frameTime;   // The time this frame takes
  Uint32 frameNumber; // Which frame the clock is on

  double maximumFrameTime; // The maximum time delta the clock accepts (default .2 seconds)

  // Circular buffer to hold frame histories
  double *frameHistory;
  int frameHistorySize;
  int frameHistoryWindow;
  int frameHistoryHead;
};

static ClockState_t * clk_new( ClockState_t * cs, const char * name, int size  );
static bool_t         clk_delete( ClockState_t * cs );

//static void   clk_initTime( ClockState_t * cs );
static void   clk_setFrameHistoryWindow( ClockState_t * cs, int size );
static void   clk_addToFrameHistory( ClockState_t * cs, double frame );
static double clk_getExactLastFrameDuration( ClockState_t * cs );
static double clk_guessFrameDuration( ClockState_t * cs );

ClockState_t * clk_create(const char * name, int size)
{
  ClockState_t * cs;

  cs = EGOBOO_NEW( ClockState_t );

  return clk_new( cs, name, size );
}

bool_t clk_destroy( ClockState_t ** pcs )
{
  bool_t retval;

  if(NULL == pcs || NULL == *pcs) return bfalse;

  retval = clk_delete( *pcs );
  EGOBOO_DELETE( *pcs );

  return retval;
}

ClockState_t * clk_new( ClockState_t * cs, const char * name, int size )
{
  clock_source_ptr_t psrc;
  if(NULL == cs) return cs;

  if(size<0) size = 1;
  //log_info("clk_new() - \n\t\"%s\"\t%d buffer(s)\n", name, size);

  memset( cs, 0, sizeof( ClockState_t ) );

  cs->maximumFrameTime = 0.2;
  cs->name = name;
  clk_setFrameHistoryWindow( cs, size );

  psrc = clock_getTimeSource();
  cs->sourceStartTime = psrc();
  cs->sourceLastTime  = cs->sourceStartTime;

  return cs;
}

bool_t clk_delete( ClockState_t * cs )
{
  if(NULL == cs) return bfalse;

  EGOBOO_DELETE ( cs->frameHistory );

  return btrue;
}

ClockState_t * clk_renew( ClockState_t * cs )
{
  EGO_CONST char * name;
  int size;

  name = cs->name;
  size = cs->frameHistorySize;

  clk_delete(cs);
  return clk_new(cs, name, size);
}

void clk_setFrameHistoryWindow( ClockState_t * cs, int size )
{
  double *history;
  int oldSize = cs->frameHistoryWindow;
  int less;

  // The frame history has to be at least 1
  cs->frameHistoryWindow = ( size > 1 ) ? size : 1;
  history = EGOBOO_NEW_ARY( double, cs->frameHistoryWindow );

  if (NULL == cs->frameHistory)
  {
    memset( history, 0, sizeof( double ) * cs->frameHistoryWindow );
  }
  else
  {
    // Copy over the older history.  Make sure that only the size of the
    // smaller buffer is copied
    less = ( cs->frameHistoryWindow < oldSize ) ? cs->frameHistoryWindow : oldSize;
    memcpy( history, cs->frameHistory, less );

    EGOBOO_DELETE( cs->frameHistory );
  }

  cs->frameHistoryHead = 0;
  cs->frameHistory = history;
}

double clk_guessFrameDuration( ClockState_t * cs )
{
  double time = 0;

  if( cs->frameHistorySize == 1 )
  {
    time = cs->frameHistory[0];
  }
  else
  {
    int c;
    double totalTime = 0;

    for ( c = 0; c < cs->frameHistorySize; c++ )
    {
      totalTime += cs->frameHistory[c];
    }

    time = totalTime / cs->frameHistorySize;
  };

  return time;
}

void clk_addToFrameHistory( ClockState_t * cs, double frame )
{
  cs->frameHistory[cs->frameHistoryHead] = frame;

  cs->frameHistoryHead++;
  if ( cs->frameHistoryHead >= cs->frameHistoryWindow )
  {
    cs->frameHistoryHead = 0;
  }

  cs->frameHistorySize++;
  if ( cs->frameHistorySize > cs->frameHistoryWindow )
  {
    cs->frameHistorySize = cs->frameHistoryWindow;
  }
}

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

void clk_frameStep( ClockState_t * cs )
{
  double lastFrame = clk_getExactLastFrameDuration( cs );
  clk_addToFrameHistory( cs, lastFrame );

  // This feels wrong to me; we're guessing at how long this
  // frame is going to be and accepting that as our time value.
  // I'll trust Mr. Lopis for now, but it may change.
  cs->frameTime = clk_guessFrameDuration( cs );
  cs->currentTime += cs->frameTime;

  cs->frameNumber++;
}

double clk_getTime( ClockState_t * cs )
{
  return cs->currentTime;
}

double clk_getFrameDuration( ClockState_t * cs )
{
  return cs->frameTime;
}

Uint32 clk_getFrameNumber( ClockState_t * cs )
{
  return cs->frameNumber;
}

float clk_getFrameRate( ClockState_t * cs )
{
  return (float)( 1.0 / cs->frameTime );
}
