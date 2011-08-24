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

#include "egoboo_typedef.h"

#include "network.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ClientState;
typedef struct s_ClientState ClientState_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A mockup of an actual client state
struct s_ClientState
{
    BaseClientState_t base;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Globally accesible client state
extern ClientState_t ClientState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int  cl_init( void );
void cl_shutDown( void );
void cl_frameStep( void );

// Much more to come...
// int  cl_connectToServer(...);
// int  cl_loadModule(...);

egolib_rv cl_talkToHost( void );
egolib_rv cl_joinGame( const char *hostname );
egolib_rv cl_handlePacket( enet_packet_t * enet_pkt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define egoboo_Client_h
