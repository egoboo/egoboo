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

#define CHAT_BUFFER_SIZE 2048

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

void net_unbuffer_player_latches();
