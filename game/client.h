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

/// @file client.h
/// @details Basic skeleton for the client portion of a client-server architecture,
/// this is totally not in use yet.

/// A mockup of an actual client state
typedef struct ClientState_t
{
    int dummy;
} ClientState_t;

// Globally accesible client state
extern ClientState_t ClientState;

int  cl_init();
void cl_shutDown();
void cl_frameStep();

// Much more to come...
// int  cl_connectToServer(...);
// int  cl_loadModule(...);

#define egoboo_Client_h
