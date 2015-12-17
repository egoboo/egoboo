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
}

//--------------------------------------------------------------------------------------------

ObjectRef PlaStack_get_ichr(const PLA_REF iplayer)
{
    if (iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid) return ObjectRef::Invalid;
    player_t *player = PlaStack.get_ptr( iplayer );

    if (!_currentModule->getObjectHandler().exists(player->index)) return ObjectRef::Invalid;

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
    *ppla = {};
    ppla->index  = ObjectRef::Invalid;
    ppla->quest_log.clear();

    // initialize the latches
    ppla->local_latch.clear();
    ppla->net_latch.clear();

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );
}

player_t *chr_get_ppla(const ObjectRef ichr)
{
    if (!_currentModule->getObjectHandler().exists(ichr)) return nullptr;
    PLA_REF iplayer = _currentModule->getObjectHandler().get(ichr)->is_which_player;

    if (!VALID_PLA(iplayer)) return nullptr;

    return PlaStack.get_ptr(iplayer);
}

//--------------------------------------------------------------------------------------------
void net_unbuffer_player_latches()
{
    /// @author ZZ
    /// @details This function sets character latches based on player input to the host

    // get the "network" latch for each valid player
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

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, Just one latch for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _currentModule->getObjectHandler().get(ppla->index)->Name );
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

            if ( weight_sum > 0.0f )
            {
                tmp_latch.x /= ( float )weight_sum;
                tmp_latch.y /= ( float )weight_sum;
            }

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, %d, multiple latches for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, latch_count, _currentModule->getObjectHandler().get(ppla->index)->Name );
        }
        else
        {
            // there are no valid latches
            // do nothing. this lets the old value of the latch persist.
            // this might be a decent guess as to what to do if a packet was
            // dropped?
            //log_info( "<<%1.4f, %1.4f>, 0x%x>, latch dead reckoning for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, _currentModule->getObjectHandler().get(ppla->index)->Name );
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

        ObjectRef character = PlaStack.lst[ipla].index;
        if (!_currentModule->getObjectHandler().exists(character)) continue;
        Object *pchr = _currentModule->getObjectHandler().get(character);

        pchr->latch = ppla->net_latch;
    }

    // Let players respawn
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ++ipla)
    {
        if (!PlaStack.lst[ipla].valid) continue;

        ObjectRef character = PlaStack.lst[ipla].index;
        const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];
        if(!pchr) continue;

        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard && pchr->latch.b[LATCHBUTTON_RESPAWN] && _currentModule->isRespawnValid())
        {
            if ( !pchr->isAlive() && 0 == local_stats.revivetimer )
            {
                pchr->respawn();
                _currentModule->getTeamList()[pchr->team].setLeader(pchr);
                SET_BIT( pchr->ai.alert, ALERTIF_CLEANEDUP );

                // cost some experience for doing this...  never lose a level
                pchr->experience *= EXPKEEP;
                if (egoboo_config_t::get().game_difficulty.getValue() > Ego::GameDifficulty::Easy) pchr->money *= EXPKEEP;
            }

            // remove all latches other than latchbutton_respawn
            pchr->latch.b[LATCHBUTTON_RESPAWN] = 0;
        }
    }
}
