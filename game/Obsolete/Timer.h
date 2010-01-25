/* Egoboo - Timer.h
 * This code is not currently in use.
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


#ifndef Egoboo_Timer_h
#define Egoboo_Timer_h

typedef struct Timer
{
    double currentTime;
    double frameTime;
    float timeScale;
    int isPaused;
} Timer;

void timer_init();  // Initialize the timer code
void timer_shutdown(); // Turn off the timer code
void timer_update(); // Update all registered timers

void timer_addTimer( Timer *t );  // Add a timer for the system to watch
void timer_removeTimer( Timer *t ); // Tell the system to stop watching this timer

#endif // include guard
