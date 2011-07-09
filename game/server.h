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

/// @file server.h
/// @Basic skeleton for the server portion of a client-server architecture,
/// this is totally not in use yet.

#include "egoboo_typedef.h"

#include "network.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_NetPlayerInfo;
typedef struct s_NetPlayerInfo NetPlayerInfo_t;

struct s_ServerState;
typedef struct s_ServerState ServerState_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXNETPLAYER         8

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Network information on connected players
struct s_NetPlayerInfo
{
    int playerSlot;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A mockup of the struct that could be used to store the server state
struct s_ServerState
{
    Uint32 last_frame;
    bool_t am_host;

    int     player_count;                             ///< How many we found
    char    player_name[MAXNETPLAYER][NETNAMESIZE];   ///< Names of machines

    NetPlayerInfo_t player_info[MAXNETPLAYER];

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Globally accessible server state
extern ServerState_t ServerState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int  sv_init( void );
void sv_shutDown( void );
void sv_frameStep( void );

egoboo_rv sv_talkToRemotes( net_instance_t * pnet );
egoboo_rv sv_hostGame( net_instance_t * pnet );
egoboo_rv sv_letPlayersJoin( net_instance_t * pnet );
egoboo_rv sv_handlePacket( net_instance_t * pnet, enet_packet_t * enet_pkt );

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define egoboo_Server_h
