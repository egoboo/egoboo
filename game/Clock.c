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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "proto.h"
#include "Clock.h"
#include "Log.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h> // memcpy & memset

// Clock data
static double ( *clk_timeSource )();  // Function that the clock get it's time values from
static double clk_sourceStartTime;    // The first value the clock receives from above function
static double clk_sourceLastTime;    // The last value the clock received from above function
static double clk_currentTime;      // The current time, not necessarily in sync w/ the source time
static double clk_frameTime;      // The time this frame takes
static Uint32 clk_frameNumber;  // Which frame the clock is on

const double clk_maximumFrameTime = 0.2f; // The maximum time delta the clock accepts (default 0.2f seconds)

// Circular buffer to hold frame histories
static double *clk_frameHistory = NULL;
static int clk_frameHistorySize;
static int clk_frameHistoryWindow;
static int clk_frameHistoryHead;

void clock_init()
{
  log_info( "Initializing clock services...\n" );

  clock_shutdown();  // Use this to set everything to 0

  clock_setTimeSource( sys_getTime );
  clock_setFrameHistoryWindow( 1 );
}

void clock_shutdown()
{
  if ( clk_frameHistory != NULL )
  {
    free( clk_frameHistory );
  }

  clk_timeSource = NULL;
  clk_sourceStartTime = 0;
  clk_sourceLastTime = 0;
  clk_currentTime = 0;
  clk_frameTime = 0;
  clk_frameNumber = 0;

  clk_frameHistory = NULL;
  clk_frameHistorySize = 0;
  clk_frameHistoryWindow = 0;
  clk_frameHistoryHead = 0;
}

void clock_setTimeSource( double ( *timeSource )() )
{
  clk_timeSource = timeSource;

  if ( clk_timeSource )
  {
    clk_sourceStartTime = clk_timeSource();
    clk_sourceLastTime = clk_sourceStartTime;
  }
}

void clock_setFrameHistoryWindow( int size )
{
  double *history;
  int oldSize = clk_frameHistoryWindow;
  int less;

  // The frame history has to be at least 1
  clk_frameHistoryWindow = ( size > 1 ) ? size : 1;
  history = ( double* )malloc( sizeof( double ) * clk_frameHistoryWindow );

  if ( clk_frameHistory != NULL )
  {
    // Copy over the older history.  Make sure that only the size of the
    // smaller buffer is copied
    less = ( clk_frameHistoryWindow < oldSize ) ? clk_frameHistoryWindow : oldSize;
    memcpy( history, clk_frameHistory, less );
    free( clk_frameHistory );
  }
  else
  {
    memset( history, 0, sizeof( double ) * clk_frameHistoryWindow );
  }

  clk_frameHistoryHead = 0;
  clk_frameHistory = history;
}

double clock_guessFrameDuration()
{
  int c;
  double totalTime = 0;

  for ( c = 0;c < clk_frameHistorySize;c++ )
  {
    totalTime += clk_frameHistory[c];
  }

  return totalTime / clk_frameHistorySize;
}

void clock_addToFrameHistory( double frame )
{
  clk_frameHistory[clk_frameHistoryHead] = frame;

  clk_frameHistoryHead++;
  if ( clk_frameHistoryHead >= clk_frameHistoryWindow )
  {
    clk_frameHistoryHead = 0;
  }

  clk_frameHistorySize++;
  if ( clk_frameHistorySize > clk_frameHistoryWindow )
  {
    clk_frameHistorySize = clk_frameHistoryWindow;
  }
}

double clock_getExactLastFrameDuration()
{
  double sourceTime;
  double timeElapsed;

  if ( clk_timeSource )
  {
    sourceTime = clk_timeSource();
  }
  else
  {
    sourceTime = 0;
  }

  timeElapsed = sourceTime - clk_sourceLastTime;
  // If more time elapsed than the maximum we allow, say that only the maximum occurred
  if ( timeElapsed > clk_maximumFrameTime )
  {
    timeElapsed = clk_maximumFrameTime;
  }

  clk_sourceLastTime = sourceTime;
  return timeElapsed;
}

void clock_frameStep()
{
  double lastFrame = clock_getExactLastFrameDuration();
  clock_addToFrameHistory( lastFrame );

  // This feels wrong to me; we're guessing at how long this
  // frame is going to be and accepting that as our time value.
  // I'll trust Mr. Lopis for now, but it may change.
  clk_frameTime = clock_guessFrameDuration();
  clk_currentTime += clk_frameTime;

  clk_frameNumber++;
}

double clock_getTime()
{
  return clk_currentTime;
}

double clock_getFrameDuration()
{
  return clk_frameTime;
}

Uint32 clock_getFrameNumber()
{
  return clk_frameNumber;
}

float clock_getFrameRate()
{
  return ( float )( 1.0f / clk_frameTime );
}
