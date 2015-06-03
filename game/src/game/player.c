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

/// @file game/player.c
/// @brief Implementation of player functions
/// @details

#include "game/player.h"
#include "game/game.h"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Stack<player_t, MAX_PLAYER> PlaStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void PlaStack_reset_all()
{
    // Reset the initial player data and latches.
    for (PLA_REF cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        pla_reinit( PlaStack.get_ptr( cnt ) );
    }
    PlaStack.count = 0;
    nexttimestamp = std::numeric_limits<Uint32>::max();
    numplatimes   = 0;
}

//--------------------------------------------------------------------------------------------

CHR_REF PlaStack_get_ichr(const PLA_REF iplayer)
{
    if (iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid) return INVALID_CHR_REF;
    player_t *player = PlaStack.get_ptr( iplayer );

    if (!_currentModule->getObjectHandler().exists(player->index)) return INVALID_CHR_REF;

    return player->index;
}

//--------------------------------------------------------------------------------------------
Object *PlaStack_get_pchr(const PLA_REF iplayer)
{
    player_t * pplayer;

    if ( iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid ) return NULL;
    pplayer = PlaStack.get_ptr( iplayer );

    if ( !_currentModule->getObjectHandler().exists( pplayer->index ) ) return NULL;

    return _currentModule->getObjectHandler().get( pplayer->index );
}

//--------------------------------------------------------------------------------------------
void PlaStack_add_tlatch(const PLA_REF iplayer, Uint32 time, latch_t net_latch)
{
    if (!VALID_PLA(iplayer)) return;
    player_t *player = PlaStack.get_ptr(iplayer);

    if (player->tlatch_count >= MAXLAG) return;

    player->tlatch[player->tlatch_count ].button = net_latch.b.to_ulong();
    player->tlatch[player->tlatch_count ].x      = net_latch.x;
    player->tlatch[player->tlatch_count ].y      = net_latch.y;
    player->tlatch[player->tlatch_count ].time   = time;
    player->tlatch_count++;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pla_reinit( player_t * ppla )
{
    if ( nullptr == ppla ) return;

    //clear input pointer
    ppla->pdevice = nullptr;

    //reset data
    BLANK_STRUCT_PTR( ppla )
    ppla->index  = INVALID_CHR_REF;

    // initialize the latches
    ppla->local_latch.clear();
    ppla->net_latch.clear();

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );
}
