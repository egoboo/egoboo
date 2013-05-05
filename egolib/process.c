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

/// @file egolib_process.c
/// @brief Implementation of Egoboo "process" control routines
/// @details

#include "../egolib/process.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
process_t * process_init( process_t * proc )
{
    if ( NULL == proc ) return proc;

    BLANK_STRUCT_PTR( proc )

    proc->terminated = C_TRUE;

    return proc;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_start( process_t * proc )
{
    if ( NULL == proc ) return C_FALSE;

    // choose the correct proc->state
    if ( proc->terminated || proc->state > proc_leaving )
    {
        // must re-initialize the process
        proc->state = proc_begin;
    }

    if ( proc->state > proc_entering )
    {
        // the process is already initialized, just put it back in
        // proc_entering mode
        proc->state = proc_entering;
    }

    // tell it to run
    proc->terminated = C_FALSE;
    proc->valid      = C_TRUE;
    proc->paused     = C_FALSE;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_kill( process_t * proc )
{
    if ( NULL == proc ) return C_FALSE;

    if ( !process_validate( proc ) ) return C_TRUE;

    // turn the process back on with an order to commit suicide
    proc->paused = C_FALSE;
    proc->killme = C_TRUE;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_validate( process_t * proc )
{
    if ( NULL == proc ) return C_FALSE;

    if ( !proc->valid || proc->terminated )
    {
        process_terminate( proc );
    }

    return proc->valid;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_terminate( process_t * proc )
{
    if ( NULL == proc ) return C_FALSE;

    proc->valid      = C_FALSE;
    proc->terminated = C_TRUE;
    proc->state      = proc_begin;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_pause( process_t * proc )
{
    C_BOOLEAN old_value;

    if ( !process_validate( proc ) ) return C_FALSE;

    old_value    = proc->paused;
    proc->paused = C_TRUE;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_resume( process_t * proc )
{
    C_BOOLEAN old_value;

    if ( !process_validate( proc ) ) return C_FALSE;

    old_value    = proc->paused;
    proc->paused = C_FALSE;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN process_running( process_t * proc )
{
    if ( !process_validate( proc ) ) return C_FALSE;

    return !proc->paused;
}
