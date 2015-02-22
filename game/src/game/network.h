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

/// @file  game/network.h
/// @brief Skeleton for Egoboo networking

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// Network constants
//--------------------------------------------------------------------------------------------

#define CHAT_BUFFER_SIZE 2048

#define NET_EGOBOO_PORT  0x8742

/// packet headers
enum e_net_commands
{
    TO_ANY_TEXT         = 0x654F,

    TO_HOST_MODULEOK    = 0x3A67,
    TO_HOST_MODULEBAD   = 0x3A68,
    TO_HOST_LATCH       = 0x8477,
    TO_HOST_IM_LOADED   = 0x9D00,

    TO_REMOTE_MODULE    = 0xDAD9,
    TO_REMOTE_LATCH     = 0x31AB,
    TO_REMOTE_START     = 0xC8BE,
};

//--------------------------------------------------------------------------------------------
// CHAT BUFFER

struct chat_buffer_t
{
    int     buffer_count;
    char    buffer[CHAT_BUFFER_SIZE];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Uint32        nexttimestamp;                ///< Expected timestamp

extern chat_buffer_t net_chat;

extern Uint32        numplatimes;

extern int     net_players_ready;         // Number of players ready to start
extern int     net_players_loaded;

//--------------------------------------------------------------------------------------------
// Networking functions
//--------------------------------------------------------------------------------------------
void net_initialize();
void net_shutDown();
bool net_begin();
bool net_end();

void net_unbuffer_player_latches();

void net_sayHello();
void net_send_message();

void find_open_sessions();
void stop_players_from_joining();
void turn_on_service( int service );

void net_count_players();

int create_player( int host );
