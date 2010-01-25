/* Egoboo - Task.c
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

#include "Task.h"
#include "Clock.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct Task
{
    char *name;
    TaskCallback func;

    double interval;
    double timeLastCalled;
    char paused;

    struct Task *previous, *next;
} Task;

Task *task_list = NULL;

void task_register( const char *taskName, float timeInterval, TaskCallback f )
{
    Task *aTask;
    size_t len;

    if ( taskName == NULL || f == NULL )
    {
        return;
    }

    // If the time interval is negative, treat it as 0
    if ( timeInterval < 0 ) timeInterval = 0;

    len = strlen( taskName ) + 1;
    aTask = malloc( sizeof( Task ) );
    memset( aTask, 0, sizeof( Task ) );

    aTask->name = malloc( len );
    strncpy( aTask->name, taskName, len );
    aTask->func = f;

    aTask->interval = timeInterval;
    aTask->timeLastCalled = ( float )clock_getTime();

    if ( task_list == NULL )
    {
        task_list = aTask;
    }
    else
    {
        // Order on the list doesn't really matter, so just stick this
        // new task on the front so that I don't have to maintain a tail variable
        aTask->next = task_list;
        task_list->previous = aTask;
        task_list = aTask;
    }
}

void task_remove( const char *taskName )
{
    Task *aTask;

    aTask = task_list;
    while ( aTask != NULL )
    {
        if ( strcmp( taskName, aTask->name ) == 0 )
        {
            if ( aTask->previous ) aTask->previous->next = aTask->next;
            if ( aTask->next ) aTask->next->previous = aTask->previous;

            if ( aTask == task_list ) task_list = aTask->next;

            free( aTask->name );
            free( aTask );
            return;
        }
        aTask = aTask->next;
    }
}

void task_pause( const char *taskName )
{
    Task *aTask;

    aTask = task_list;
    while ( aTask != NULL )
    {
        if ( strcmp( taskName, aTask->name ) == 0 )
        {
            aTask->paused = 1;
            return;
        }
        aTask = aTask->next;
    }
}

void task_play( const char *taskName )
{
    Task *aTask;

    aTask = task_list;
    while ( aTask != NULL )
    {
        if ( strcmp( taskName, aTask->name ) == 0 )
        {
            aTask->paused = 0;
            return;
        }
        aTask = aTask->next;
    }
}

void task_updateAllTasks()
{
    double currentTime, deltaTime;
    Task *aTask;

    currentTime = clock_getTime();

    aTask = task_list;
    while ( aTask != NULL )
    {
        if ( aTask->paused )
        {
            // don't call the task, but update it's last called time
            // anyway so that things don't get weird when it is unpaused
            aTask->timeLastCalled = currentTime;
        }
        else
        {
            deltaTime = currentTime - aTask->timeLastCalled;
            if ( deltaTime >= aTask->interval )
            {
                aTask->func(( float )deltaTime );
                aTask->timeLastCalled = currentTime;
            }
        }

        aTask = aTask->next;
    }
}
