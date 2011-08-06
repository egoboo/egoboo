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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file threads.h
/// @brief A way to handle worker threads using SDL

#include "../egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// opaque struct for wrapping SDL threads
struct s_egolib_thread;
struct s_egolib_thread_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef int (*SDL_thread_callback_t)(void *);

//--------------------------------------------------------------------------------------------
// declaration of egolib_thread_t
//--------------------------------------------------------------------------------------------

typedef struct s_egolib_thread egolib_thread_t;

// constructors
egolib_thread_t * egolib_thread_ctor( egolib_thread_t * );
egolib_thread_t * egolib_thread_dtor( egolib_thread_t * );

// initialization
egolib_thread_t * egolib_thread_start( egolib_thread_t *, SDL_thread_callback_t, void * );

// thread control
bool_t egolib_thread_req_end( egolib_thread_t * );
bool_t egolib_thread_req_quit( egolib_thread_t * );
bool_t egolib_thread_kill( egolib_thread_t * );

// accessors
bool_t egolib_thread_running( const egolib_thread_t * );
bool_t egolib_thread_check_started( const egolib_thread_t * );
bool_t egolib_thread_check_done( const egolib_thread_t * );
bool_t egolib_thread_check_error( const egolib_thread_t * );
const SDL_Thread * egolib_thread_get_thread_ptr( const egolib_thread_t * );

//--------------------------------------------------------------------------------------------
// declaration of egolib_thread_data_t
//--------------------------------------------------------------------------------------------

/// a structure that can be "inherited" by any data segment that can be worked on 
/// via a worker thread
struct s_egolib_thread_data
{
    unsigned initialized;

    // a bitfield to track the state of the data
    unsigned queued  : 1;   ///< the data has been queued in a thread
    unsigned started : 1;   ///< the data processing has been started
    unsigned done    : 1;   ///< the data processing is finished
    unsigned error   : 1;   ///< the data processing ended with an error
};
typedef struct s_egolib_thread_data egolib_thread_data_t;

egolib_thread_data_t * egolib_thread_data_ctor( egolib_thread_data_t * );
egolib_thread_data_t * egolib_thread_data_dtor( egolib_thread_data_t * );