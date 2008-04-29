/* Egoboo - Clock.c
 * Clock & timer functionality
 * This implementation was adapted from Noel Lopis' article in
 * Game Programming Gems 4.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "proto.h"
#include "Clock.h"
#include "System.h"
#include "Log.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h> // memcpy & memset

// Clock data
struct clock_state_t
{
  // Clock data
  double( *timeSource )();     // Function that the clock get it's time values from
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

static void clock_state_init( ClockState * cs )
{
  memset( cs, 0, sizeof( ClockState ) );
  cs->maximumFrameTime = 0.2;
};

ClockState * clock_create_state()
{
  ClockState * cs;

  cs = ( ClockState * ) calloc( 1, sizeof( ClockState ) );
  clock_state_init( cs );

  return cs;
};

ClockState * clock_recreate_state( ClockState * cs )
{
  FREE ( cs->frameHistory );

  clock_state_init( cs );
  return cs;
};


void clock_free_state( ClockState * cs )
{
  FREE ( cs->frameHistory );
  FREE ( cs );
};

void clock_init( ClockState * cs )
{
  log_info( "Initializing clock services...\n" );

  clock_recreate_state( cs );  // Use this to set everything to 0
  clock_setTimeSource( cs, sys_getTime );
  clock_setFrameHistoryWindow( cs, 1 );
}

void clock_shutdown( ClockState * cs )
{
  clock_free_state( cs );
}

void clock_setTimeSource( ClockState * cs, double( *timeSource )() )
{
  cs->timeSource = timeSource;

  if ( cs->timeSource )
  {
    cs->sourceStartTime = cs->timeSource();
    cs->sourceLastTime = cs->sourceStartTime;
  }
}

void clock_setFrameHistoryWindow( ClockState * cs, int size )
{
  double *history;
  int oldSize = cs->frameHistoryWindow;
  int less;

  // The frame history has to be at least 1
  cs->frameHistoryWindow = ( size > 1 ) ? size : 1;
  history = ( double* ) malloc( sizeof( double ) * cs->frameHistoryWindow );

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

    FREE( cs->frameHistory );
  }

  cs->frameHistoryHead = 0;
  cs->frameHistory = history;
}

double clock_guessFrameDuration( ClockState * cs )
{
  int c;
  double totalTime = 0;

  for ( c = 0;c < cs->frameHistorySize;c++ )
  {
    totalTime += cs->frameHistory[c];
  }

  return totalTime / cs->frameHistorySize;
}

void clock_addToFrameHistory( ClockState * cs, double frame )
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

double clock_getExactLastFrameDuration( ClockState * cs )
{
  double sourceTime;
  double timeElapsed;

  if ( cs->timeSource )
  {
    sourceTime = cs->timeSource();
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

void clock_frameStep( ClockState * cs )
{
  double lastFrame = clock_getExactLastFrameDuration( cs );
  clock_addToFrameHistory( cs, lastFrame );

  // This feels wrong to me; we're guessing at how long this
  // frame is going to be and accepting that as our time value.
  // I'll trust Mr. Lopis for now, but it may change.
  cs->frameTime = clock_guessFrameDuration( cs );
  cs->currentTime += cs->frameTime;

  cs->frameNumber++;
}

double clock_getTime( ClockState * cs )
{
  return cs->currentTime;
}

double clock_getFrameDuration( ClockState * cs )
{
  return cs->frameTime;
}

Uint32 clock_getFrameNumber( ClockState * cs )
{
  return cs->frameNumber;
}

float clock_getFrameRate( ClockState * cs )
{
  return ( float )( 1.0 / cs->frameTime );
}
