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

/// @file egolib/process.h

#pragma once

#include "egolib/typedef.h"
#include "egolib/state_machine.h"

//--------------------------------------------------------------------------------------------
// MACROS AND ENUMS
//--------------------------------------------------------------------------------------------

/// grab a pointer to the process_t of any object that "inherits" this type
#define PROC_PBASE(PTR) (&( (PTR)->base ))

// The various states that a process can occupy
enum process_state_t
{
    proc_invalid  = ego_state_invalid,
    proc_begin    = ego_state_begin,
    proc_entering = ego_state_entering,
    proc_running  = ego_state_running,
    proc_leaving  = ego_state_leaving,
    proc_finish   = ego_state_finish
};


/// A rudimantary implementation of "non-preemptive multitasking" in Egoboo.
/// @details All other process types "inherit" from this one
struct process_t
{

    bool valid;
    bool paused;
    bool killme;
    bool terminated;
    process_state_t state;
    double frameDuration;

	static process_t *init(process_t *self);
	static bool start(process_t *self);
	static bool kill(process_t *self);
	static bool validate(process_t *self);
	static bool terminate(process_t *self);
	static bool pause(process_t *self);
	static bool resume(process_t *self);
	static bool running(process_t *self);

};

