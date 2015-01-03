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

/// @file game/server.h
/// @Basic skeleton for the server portion of a client-server architecture,
/// this is totally not in use yet.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/network.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ServerState;
typedef struct s_ServerState ServerState_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXNETPLAYER         8

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A mockup of the struct that could be used to store the server state
struct s_ServerState
{
    BaseServerState_t base;          ///< the base class of the server state
    int               player_count;  ///< the actual number of players
};

#define SERVER_STATE_INIT \
    { \
        BASE_SERVER_STATE_INIT, /* BaseServerState_t base */ \
        0                       /* int player_count       */ \
    }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Globally accessible server state
extern ServerState_t ServerState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int  sv_init();
void sv_shutDown();
void sv_frameStep();

egolib_rv sv_talkToRemotes();
egolib_rv sv_hostGame();
egolib_rv sv_letPlayersJoin();
egolib_rv sv_handlePacket( enet_packet_t * enet_pkt );

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)
