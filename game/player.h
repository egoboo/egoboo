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

/// @file player.h

#include "egoboo_typedef.h"

#include "IDSZ_map.h"
#include "network.h"
#include "input.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_player;
typedef struct s_player player_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_PLAYER MAX_PLAYER

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of a player
struct s_player
{
    bool_t                  valid;                    ///< Player used?
    CHR_REF                 index;                    ///< Which character?

    /// the buffered input from the local input devices
    input_device_t          *pdevice;

    // inventory stuff
    CHR_REF                 selected_item;
    bool_t                  draw_inventory;
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

DECLARE_STACK_EXTERN( player_t, PlaStack, MAX_PLAYER );                         ///< Stack for keeping track of players

#define VALID_PLA_RANGE(IPLA) ( ((IPLA) >= 0) && ((IPLA) < MAX_PLAYER) )
#define VALID_PLA(IPLA)       ( VALID_PLA_RANGE(IPLA) && ((IPLA) < PlaStack.count) && PlaStack.lst[IPLA].valid )
#define INVALID_PLA(IPLA)     ( !VALID_PLA_RANGE(IPLA) || ((IPLA) >= PlaStack.count)|| !PlaStack.lst[IPLA].valid )

void           PlaStack_reset_all( void );
CHR_REF        PlaStack_get_ichr( const PLA_REF iplayer );
struct s_chr * PlaStack_get_pchr( const PLA_REF iplayer );
void           PlaStack_add_tlatch( const PLA_REF iplayer, Uint32 time, latch_t net_latch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

player_t*      chr_get_ppla( const CHR_REF ichr );