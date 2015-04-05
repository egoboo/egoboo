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

/// @file    game/network.c
/// @brief   Egoboo networking implementation.
/// @details Shuttles bits across the network, using Enet.
///          Networked play doesn't really work at the moment.

#include "game/network.h"
#include "game/network_server.h"
#include "game/network_client.h"
#include "game/input.h"
#include "game/game.h"
#include "game/player.h"
#include "game/renderer_2d.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 numplatimes = 0;
chat_buffer_t net_chat = { 0, EMPTY_CSTR };
Uint32 nexttimestamp;                          // Expected timestamp

//--------------------------------------------------------------------------------------------
void net_unbuffer_player_latches()
{
    /// @author ZZ
    /// @details This function sets character latches based on player input to the host

    // get the "network" latch for each valid player
    numplatimes = 0;
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ++ipla)
    {
        if ( !PlaStack.lst[ipla].valid ) continue;
        player_t *ppla = PlaStack.get_ptr(ipla);
        time_latch_t *tlatch_list = ppla->tlatch;

        // copy the latch from last time
        latch_t tmp_latch = ppla->net_latch;

        // what are the minimum and maximum indices that can be applies this update?
        Uint32 tnc;
        for (tnc = 0; tnc < ppla->tlatch_count; ++tnc)
        {
            int dt;

            dt = update_wld - tlatch_list[tnc].time;

            if (dt < 0)
                break;
        }
        Uint32 latch_count = tnc;

        if ( 1 == latch_count )
        {
            // there is just one valid latch
            tmp_latch.x = tlatch_list[0].x;
            tmp_latch.y = tlatch_list[0].y;
            tmp_latch.b = tlatch_list[0].button;

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, Just one latch for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _gameObjects.get(ppla->index)->Name );
        }
        else if ( latch_count > 1 )
        {
            int weight, weight_sum;
            int dt;

            // estimate the best latch value by weighting latches that are back in time
            // by dt*dt. This estimates the effect of actually integrating the position over
            // that much time without the hastle of actually integrating the trajectory.

            // blank the current latch so that we can sum the latch values
            tmp_latch.clear();

            // apply the latch
            weight_sum = 0;
            for ( tnc = 0; tnc < latch_count; tnc++ )
            {
                dt = update_wld - tlatch_list[tnc].time;

                weight      = ( dt + 1 ) * ( dt + 1 );

                weight_sum  += weight;
                tmp_latch.x += tlatch_list[tnc].x * weight;
                tmp_latch.y += tlatch_list[tnc].y * weight;
                SET_BIT( tmp_latch.b, tlatch_list[tnc].button );
            }

            numplatimes = std::max( numplatimes, latch_count );
            if ( weight_sum > 0.0f )
            {
                tmp_latch.x /= ( float )weight_sum;
                tmp_latch.y /= ( float )weight_sum;
            }

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, %d, multiple latches for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, latch_count, _gameObjects.get(ppla->index)->Name );
        }
        else
        {
            // there are no valid latches
            // do nothing. this lets the old value of the latch persist.
            // this might be a decent guess as to what to do if a packet was
            // dropped?
            //log_info( "<<%1.4f, %1.4f>, 0x%x>, latch dead reckoning for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _gameObjects.get(ppla->index)->Name );
        }

        if ( latch_count >= ppla->tlatch_count )
        {
            // we have emptied all of the latches
            ppla->tlatch_count = 0;
        }
        else if ( latch_count > 0 )
        {
            int index;

            // concatenate the list
            for ( tnc = latch_count, index = 0; tnc < ppla->tlatch_count; tnc++, index++ )
            {
                tlatch_list[index].x      = tlatch_list[tnc].x;
                tlatch_list[index].y      = tlatch_list[tnc].y;
                tlatch_list[index].button = tlatch_list[tnc].button;
                tlatch_list[index].time   = tlatch_list[tnc].time;
            }
            ppla->tlatch_count = index;
        }

        // fix the network latch
        ppla->net_latch = tmp_latch;
    }

    // set the player latch
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ++ipla)
    {
        if (!PlaStack.lst[ipla].valid) continue;
        player_t *ppla = PlaStack.get_ptr(ipla);

        CHR_REF character = PlaStack.lst[ipla].index;
        if (!_gameObjects.exists(character)) continue;
        Object *pchr = _gameObjects.get(character);

        pchr->latch = ppla->net_latch;
    }

    // Let players respawn
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ++ipla)
    {
        if (!PlaStack.lst[ipla].valid) continue;

        CHR_REF character = PlaStack.lst[ipla].index;
        if ( !_gameObjects.exists( character ) ) continue;
        Object *pchr = _gameObjects.get( character );

        if ( cfg.difficulty < GAME_HARD && pchr->latch.b[LATCHBUTTON_RESPAWN] && PMod->isRespawnValid() )
        {
            if ( !pchr->alive && 0 == local_stats.revivetimer )
            {
                respawn_character( character );
                TeamStack.lst[pchr->team].leader = character;
                SET_BIT( pchr->ai.alert, ALERTIF_CLEANEDUP );

                // cost some experience for doing this...  never lose a level
                pchr->experience *= EXPKEEP;
                if ( cfg.difficulty > GAME_EASY ) pchr->money *= EXPKEEP;
            }

            // remove all latches other than latchbutton_respawn
            pchr->latch.b[LATCHBUTTON_RESPAWN] = 0;
        }
    }
}

player_t *chr_get_ppla(const CHR_REF ichr)
{
    if (!_gameObjects.exists(ichr)) return nullptr;
    PLA_REF iplayer = _gameObjects.get(ichr)->is_which_player;

    if (!VALID_PLA(iplayer)) return nullptr;

    return PlaStack.get_ptr(iplayer);
}
