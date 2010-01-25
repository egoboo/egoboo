/* Egoboo - Timer.c
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

#include "Timer.h"
#include "Clock.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

typedef struct TimerNode
{
    Timer *timer;
    struct TimerNode *previous, *next;
} TimerNode;

static TimerNode *timerList = NULL;
static TimerNode *timerListEnd = NULL;
static int timersInUse = 0;

static TimerNode *findTimer( Timer *t )
{
    TimerNode *node = timerList;

    while ( node != NULL )
    {
        if ( node->timer == t )
        {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

void timer_init()
{
    // Just set some basic stuff
    timerList = NULL;
    timersInUse = 0;
}

void timer_shutdown()
{
    TimerNode *node, *next;

    // Clear out the list o' timers
    node = timerList;
    while ( node != NULL )
    {
        next = node->next;
        free( node );
        node = next;
    }

    timerList = NULL;
    timersInUse = 0;
}

void timer_update()
{
    TimerNode *node;

    node = timerList;
    while ( node != NULL )
    {
        assert( node->timer != NULL );
        if ( !node->timer->isPaused )
        {
            node->timer->frameTime = clock_getFrameDuration() * node->timer->timeScale;
            node->timer->currentTime += node->timer->frameTime;
        }

        node = node->next;
    }
}

void timer_addTimer( Timer *t )
{
    TimerNode *node;

    if ( !t ) return;

    node = malloc( sizeof( TimerNode ) );
    node->timer = t;

    if ( timerList )
    {
        timerListEnd->next = node;
        node->previous = timerListEnd;
        node->next = NULL;
    }
    else
    {
        timerList = node;
        node->previous = NULL;
        node->next = NULL;
        timerListEnd = node;
    }
    timersInUse++;
}

void timer_removeTimer( Timer *t )
{
    TimerNode *node;

    if ( !t ) return;

    node = findTimer( t );
    if ( node )
    {
        if ( node == timerList )
        {
            // Is this the only node in the list?
            if ( timerList->next == NULL )
            {
                // yup
                timerList = NULL;
                timerListEnd = NULL;
            }
            else
            {
                timerList = node->next;
                timerList->previous = NULL;
            }
        }
        else if ( node == timerListEnd )
        {
            timerListEnd = node->previous;
            timerListEnd->next = NULL;
        }
        else
        {
            node->previous->next = node->next;
            node->next->previous = node->previous;
        }
        free( node );
        timersInUse--;
    }
}
