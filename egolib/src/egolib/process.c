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

process_t *process_t::init(process_t * self)
{
    if (nullptr == self) return nullptr;

	BLANK_STRUCT_PTR(self);

    self->terminated = true;

    return self;
}

//--------------------------------------------------------------------------------------------
bool process_t::start(process_t *self)
{
    if (nullptr == self) return false;

    // choose the correct proc->state
    if (self->terminated || self->state > proc_leaving)
    {
        // must re-initialize the process
        self->state = proc_begin;
    }

    if (self->state > proc_entering)
    {
        // the process is already initialized,
		// just put it back in proc_entering mode
        self->state = proc_entering;
    }

    // tell it to run
    self->terminated = false;
    self->valid      = true;
    self->paused     = false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool process_t::kill(process_t *self)
{
    if (nullptr == self) return false;

    if (!process_t::validate(self)) return true;

    // Turn the process back on with an order to commit suicide.
	self->paused = false;
	self->killme = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool process_t::validate(process_t *self)
{
    if (nullptr == self) return false;

    if (!self->valid || self->terminated)
    {
        process_t::terminate(self);
    }

    return self->valid;
}

bool process_t::terminate(process_t *self)
{
    if (nullptr == self) return false;

    self->valid      = false;
    self->terminated = true;
    self->state      = proc_begin;

    return true;
}

bool process_t::pause(process_t *self)
{
	bool old_value;

    if (!process_t::validate(self)) return false;

    old_value    = self->paused;
    self->paused = true;

    return old_value != self->paused;
}

bool process_t::resume(process_t *self)
{
	bool old_value;

    if (!process_t::validate(self)) return false;

    old_value    = self->paused;
    self->paused = false;

    return old_value != self->paused;
}

bool process_t::running(process_t *self)
{
    if (!process_t::validate(self)) return false;

    return !self->paused;
}
