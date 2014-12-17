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

/// @file egolib/process.c
/// @brief Implementation of Egoboo "process" control routines
/// @details

#include "egolib/process.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
process_t * process_init( process_t * proc )
{
    if ( NULL == proc ) return proc;

    BLANK_STRUCT_PTR( proc )

    proc->terminated = true;

    return proc;
}

//--------------------------------------------------------------------------------------------
bool process_start(process_t * proc)
{
    if ( NULL == proc ) return false;

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
    proc->terminated = false;
    proc->valid      = true;
    proc->paused     = false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool process_kill(process_t * proc)
{
    if ( NULL == proc ) return false;

    if ( !process_validate( proc ) ) return true;

    // turn the process back on with an order to commit suicide
    proc->paused = false;
    proc->killme = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool process_validate(process_t * proc)
{
    if ( NULL == proc ) return false;

    if ( !proc->valid || proc->terminated )
    {
        process_terminate( proc );
    }

    return proc->valid;
}

//--------------------------------------------------------------------------------------------
bool process_terminate(process_t * proc)
{
    if ( NULL == proc ) return false;

    proc->valid      = false;
    proc->terminated = true;
    proc->state      = proc_begin;

    return true;
}

//--------------------------------------------------------------------------------------------
bool process_pause(process_t * proc)
{
	bool old_value;

    if ( !process_validate( proc ) ) return false;

    old_value    = proc->paused;
    proc->paused = true;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
bool process_resume(process_t * proc)
{
	bool old_value;

    if ( !process_validate( proc ) ) return false;

    old_value    = proc->paused;
    proc->paused = false;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
bool process_running(process_t * proc)
{
    if ( !process_validate( proc ) ) return false;

    return !proc->paused;
}
