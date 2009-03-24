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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - Clock.c
 * Clock & timer functionality
 * This implementation was adapted from Noel Lopis' article in
 * Game Programming Gems 4.
 */

typedef double( *clk_timeSourcePtr_t )(void);

void   clk_init();               // Init the clock module
void   clk_shutdown();           // Turn off the clock module

void   clk_setTimeSource( clk_timeSourcePtr_t timeSource );  // Specify where the clock gets its time values from
                                                               // Defaults to sys_getTime()
void   clk_setFrameHistoryWindow( int size );                 // Set how many frames to keep a length history of
                                                              // Defaults to 1
void   clk_frameStep();           // Update the clock.
double clk_getTime();             // Returns the current time.  The clock's time only
                                  // changes when frameStep is called

double clk_getFrameDuration();    // Return the length of the current frame. (Sort of.)
Uint32 clk_getFrameNumber();      // Return which frame we're on
float  clk_getFrameRate();        // Return the current instantaneous FPS

#define Egoboo_Clock_h
