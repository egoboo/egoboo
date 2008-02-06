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

#ifndef Egoboo_Clock_h
#define Egoboo_Clock_h

#include <SDL_types.h>

typedef struct clock_state_t ClockState;

ClockState * clock_create_state();
void clock_free_state( ClockState * cs );

void clock_init( ClockState * cs );    // Init the clock module
void clock_shutdown( ClockState * cs );   // Turn off the clock module

void clock_setTimeSource( ClockState * cs, double( *timeSource )() );     // Specify where the clock gets its time values from
// Defaults to sys_getTime()
void clock_setFrameHistoryWindow( ClockState * cs, int size );    // Set how many frames to keep a length history of
// Defaults to 1

void clock_frameStep( ClockState * cs );   // Update the clock.
double clock_getTime( ClockState * cs );   // Returns the current time.  The clock's time only
// changes when frameStep is called

double clock_getFrameDuration( ClockState * cs );   // Return the length of the current frame. (Sort of.)
Uint32 clock_getFrameNumber( ClockState * cs );  // Return which frame we're on
float clock_getFrameRate( ClockState * cs );  // Return the current instantaneous FPS

#endif // include guard
