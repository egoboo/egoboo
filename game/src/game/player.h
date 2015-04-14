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

/// @file game/player.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/network.h"
#include "game/input.h"
#include "egolib/IDSZ_map.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

class Object;
struct player_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of a player
struct player_t
{
    bool                  valid;                    ///< Player used?
    CHR_REF                 index;                    ///< Which character?

    /// the buffered input from the local input devices
    input_device_t          *pdevice;

    // inventory stuff
    Uint8                   inventory_slot;
    bool                  draw_inventory;
    Uint32                  inventory_cooldown;
    int                     inventory_lerp;

    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    latch_t                 local_latch;

    // quest log for this player
    IDSZ_node_t             quest_log[MAX_IDSZ_MAP_SIZE];          ///< lists all the character's quests

    // Timed latches
    Uint32                  tlatch_count;
    time_latch_t            tlatch[MAXLAG];

    /// Network latch, set by net_unbuffer_player_latches(), used to set the local character's latch
    latch_t                 net_latch;
};

//void           player_init( player_t * ppla );
void           pla_reinit( player_t * ppla );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Stack<player_t, MAX_PLAYER> PlaStack; ///< Stack for keeping track of players

#define VALID_PLA_RANGE(IPLA) ( ((IPLA) >= 0) && ((IPLA) < MAX_PLAYER) )
#define VALID_PLA(IPLA)       ( VALID_PLA_RANGE(IPLA) && ((IPLA) < PlaStack.count) && PlaStack.lst[IPLA].valid )
#define INVALID_PLA(IPLA)     ( !VALID_PLA_RANGE(IPLA) || ((IPLA) >= PlaStack.count)|| !PlaStack.lst[IPLA].valid )

void PlaStack_reset_all();
CHR_REF PlaStack_get_ichr( const PLA_REF iplayer );
Object *PlaStack_get_pchr( const PLA_REF iplayer );
void PlaStack_add_tlatch( const PLA_REF iplayer, Uint32 time, latch_t net_latch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

player_t*      chr_get_ppla( const CHR_REF ichr );
