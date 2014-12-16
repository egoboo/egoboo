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

/// @file egolib/threads.c
/// @brief implementation of worker threads using SDL
/// @details

#include <SDL.h>
#include <SDL_thread.h>

#include "egolib/log.h"

#include "egolib/threads.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a uniqe identifier for this data type
#define EGOLIB_THREAD_DATA_ID 0xE7F76D18

//--------------------------------------------------------------------------------------------
// egolib_thread_t
//--------------------------------------------------------------------------------------------

struct s_egolib_thread
{
    // a bitfield to track the state of the thread
    unsigned started : 1;   ///< the thread has started
    unsigned done    : 1;   ///< the thread is finished processing
    unsigned error   : 1;   ///< an error has occured

    // a bitfield for generic requests to the thread
    unsigned req_end  : 1;  ///< request that the thread terminate in a nice way
    unsigned req_quit : 1;  ///< request that the thread terminate immediately

    unsigned retval;        ///< the return value from the thread

    SDL_Thread            * thread_ptr;
    SDL_thread_callback_t   callback_ptr;
    void                  * child_ptr;
};

//--------------------------------------------------------------------------------------------
// egolib_thread_t constructors
//--------------------------------------------------------------------------------------------
egolib_thread_t * egolib_thread_ctor( egolib_thread_t * pthread )
{
    if ( NULL == pthread ) return pthread;

    BLANK_STRUCT_PTR( pthread );

    return pthread;
}

//--------------------------------------------------------------------------------------------
egolib_thread_t * egolib_thread_dtor( egolib_thread_t * pthread )
{
    if ( NULL == pthread ) return pthread;

    // we cannot leave a running thread going
    egolib_thread_kill( pthread );

    // blank out everything
    BLANK_STRUCT_PTR( pthread );

    return pthread;
}

//--------------------------------------------------------------------------------------------
// egolib_thread_t initialization
//--------------------------------------------------------------------------------------------

egolib_thread_t * egolib_thread_start( egolib_thread_t * pthread, SDL_thread_callback_t pcallback, void * pchild )
{
    if ( NULL == pthread ) return pthread;

    if ( egolib_thread_running( pthread ) )
    {
        log_warning( "%s - tried to start a running thread.\n", __FUNCTION__ );
        return pthread;
    }

    // the thread is not running so it is safe to modify the data

    // set the pointers
    pthread->thread_ptr   = NULL;
    pthread->callback_ptr = pcallback;
    pthread->child_ptr    = pchild;

    // clear the flags
    pthread->started = 0;
    pthread->done    = 0;
    pthread->error   = 0;

    // clear the requests
    pthread->req_end  = 0;
    pthread->req_quit = 0;

    // clear the return value
    pthread->retval = 0;

    // Get SDL to start a thread. From THIS instant on, a new thread could be running.
    pthread->thread_ptr = SDL_CreateThread( pcallback, pthread );

    if ( NULL == pthread->thread_ptr )
    {
        // the thread failed, so clear the useless assignments we made before
        pthread->callback_ptr = NULL;
        pthread->child_ptr    = NULL;
    }

    return pthread;
}

//--------------------------------------------------------------------------------------------
// egolib_thread_t control
//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_req_end( egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    C_BOOLEAN       retval = C_FALSE;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = ( 0 == pthread->req_end );
    pthread->req_quit = 1;

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_req_quit( egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    C_BOOLEAN       retval = C_FALSE;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = ( 0 == pthread->req_quit );
    pthread->req_quit = 1;

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_kill( egolib_thread_t * pthread )
{
    if ( NULL == pthread ) return C_FALSE;

    SDL_KillThread( pthread->thread_ptr );
    pthread->thread_ptr   = NULL;
    pthread->callback_ptr = NULL;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
// egolib_thread_t accessors
//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_check_started( const egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    C_BOOLEAN       retval = C_FALSE;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = ( 1 == pthread->started );

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_check_done( const egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    C_BOOLEAN       retval = C_FALSE;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = ( 1 == pthread->done );

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_check_error( const egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    C_BOOLEAN       retval = C_FALSE;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = ( 1 == pthread->error );

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const SDL_Thread * egolib_thread_get_thread_ptr( const egolib_thread_t * pthread )
{
    C_BOOLEAN       locked = C_FALSE;
    SDL_Thread * retval = NULL;
    SDL_mutex  * mut    = NULL;

    if ( NULL == pthread ) return NULL;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    retval = pthread->thread_ptr;

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egolib_thread_running( const egolib_thread_t * pthread )
{
    C_BOOLEAN      running = C_FALSE;
    C_BOOLEAN      locked = C_FALSE;
    SDL_mutex * mut;

    if ( NULL == pthread ) return C_FALSE;

    mut = SDL_CreateMutex();
    if ( NULL == mut )
    {
        log_warning( "%s - could not create a mutex.\n", __FUNCTION__ );
    }
    else if ( -1 == SDL_mutexP( mut ) )
    {
        log_warning( "%s - could not lock a mutex.\n", __FUNCTION__ );
    }
    else
    {
        locked = C_TRUE;
    }

    if ( NULL == pthread->thread_ptr || NULL == pthread->callback_ptr )
    {
        // the thread data is incomplete, so it can't really be running
        running = C_FALSE;
    }
    else if ( 1 == pthread->started && 0 == pthread->done )
    {
        // the thread is definitely running
        running = C_TRUE;
    }
    else
    {
        // in all other cases, the thread should not be running
        running = C_FALSE;
    }

    if ( NULL != mut && locked )
    {
        if ( -1 == SDL_mutexV( mut ) )
        {
            log_warning( "%s - could not unlock a mutex.\n", __FUNCTION__ );
        }
    }

    return running;
}

//--------------------------------------------------------------------------------------------
// egolib_thread_data_t
//--------------------------------------------------------------------------------------------
egolib_thread_data_t * egolib_thread_data_ctor( egolib_thread_data_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    // is the thread data already initialized?
    if ( EGOLIB_THREAD_DATA_ID == pdata->initialized )
    {
        pdata = egolib_thread_data_dtor( pdata );
    }

    BLANK_STRUCT_PTR( pdata );

    pdata->initialized = EGOLIB_THREAD_DATA_ID;

    return pdata;
}

//--------------------------------------------------------------------------------------------
egolib_thread_data_t * egolib_thread_data_dtor( egolib_thread_data_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    // is the thread data already initialized?
    // if not, do nothing in case of a wild pointer
    if ( EGOLIB_THREAD_DATA_ID != pdata->initialized ) return pdata;
    {
        pdata = egolib_thread_data_dtor( pdata );
    }

    BLANK_STRUCT_PTR( pdata );

    return pdata;
}
