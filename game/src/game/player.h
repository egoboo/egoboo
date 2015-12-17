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
#include "game/input.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//Forward declarations
class Object;
struct player_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of a player
struct player_t
{
    bool                  valid;                  ///< Player used?
    ObjectRef             index;                  ///< Which character?
    bool                  _unspentLevelUp;        ///< Has gained new experience level?

    //Charge bar
    uint32_t _currentCharge;
    uint32_t _maxCharge;
    uint32_t _chargeBarFrame;
    uint32_t _chargeTick;

    /// the buffered input from the local input devices
    input_device_t          *pdevice;

    // inventory stuff
    bool                    inventoryMode;          //True if input events are consumed by inventory GUI
    uint8_t                 inventory_slot;
    uint32_t                inventory_cooldown;

    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    latch_t                 local_latch;

    // quest log for this player
    std::unordered_map<IDSZ2, int> quest_log;          ///< lists all the character's quests

    // Timed latches
    uint32_t                tlatch_count;
    time_latch_t            tlatch[MAXLAG];

    /// Network latch, set by net_unbuffer_player_latches(), used to set the local character's latch
    latch_t                 net_latch;
};

void           pla_reinit( player_t * ppla );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Stack<player_t, MAX_PLAYER> PlaStack; ///< Stack for keeping track of players

inline bool VALID_PLA_RANGE(const PLA_REF player)
{
    return player < MAX_PLAYER;
}

inline bool VALID_PLA(const PLA_REF iplayer)
{
    return iplayer < MAX_PLAYER && (iplayer < PlaStack.count) && PlaStack.lst[iplayer].valid;
}

#define INVALID_PLA(IPLA)     ( !VALID_PLA_RANGE(IPLA) || ((IPLA) >= PlaStack.count)|| !PlaStack.lst[IPLA].valid )

void PlaStack_reset_all();
ObjectRef PlaStack_get_ichr( const PLA_REF iplayer );
Object *PlaStack_get_pchr( const PLA_REF iplayer );
void PlaStack_add_tlatch( const PLA_REF iplayer, uint32_t time, latch_t net_latch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

player_t*      chr_get_ppla( const ObjectRef ichr );

void net_unbuffer_player_latches();
