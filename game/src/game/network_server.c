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

/// @file game/server.c
/// @brief This code is not currently in use.
/// @details

#include "game/network_server.h"
#include "egolib/egolib.h"
#include "game/game.h"
#include "game/player.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int lag  = 3;       // Lag tolerance

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ServerState_t ServerState = SERVER_STATE_INIT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sv_frameStep()
{
}

//--------------------------------------------------------------------------------------------
egolib_rv sv_talkToRemotes()
{
    /// @author ZZ
    /// @details This function sends the character data to all the remote machines

    PLA_REF player;
    int time;
    int update_counter;
    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    // for games that must be in synch, make sure we use the "true update"
    if ( egonet_on() || !GProc->ups_timer.free_running )
    {
        update_counter = true_update;
    }
    else
    {
        update_counter = update_wld;
    }

    // make sure there is only one update per frame;
    if ( update_wld == ServerState.base.last_frame )
    {
        return rv_fail;
    }
    ServerState.base.last_frame = update_wld;

    if ( egonet_get_hostactive() )
    {
        if ( egonet_on() )
        {
            time = update_counter + lag;

            // Send a message to all players
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, TO_REMOTE_LATCH );                       // The message header
            ego_packet_addUint32( &ego_pkt, time );                                  // The stamp

            // Send all player latches...
            for ( player = 0; player < MAX_PLAYER; player++ )
            {
                if ( !PlaStack.lst[player].valid ) continue;

                ego_packet_addUint8( &ego_pkt, REF_TO_INT( player ) );                      // The player index
                ego_packet_addUint32( &ego_pkt, PlaStack.lst[player].local_latch.b );        // Player button states
                ego_packet_addSint16( &ego_pkt, PlaStack.lst[player].local_latch.x*SHORTLATCH );  // Player motion
                ego_packet_addSint16( &ego_pkt, PlaStack.lst[player].local_latch.y*SHORTLATCH );  // Player motion

                player++;
            }

            // Send the packet
            egonet_broadcastPacket( &ego_pkt );
        }
        else
        {
            time = update_counter + 1;
        }

        // update the local timed latches with the same info
        numplatimes = 0;
        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            int index;
            Uint32 cnt;
            player_t * ppla;

            if ( !PlaStack.lst[player].valid ) continue;
            ppla = PlaStack_get_ptr( player );

            index = ppla->tlatch_count;
            if ( index < MAXLAG )
            {
                time_latch_t * ptlatch = ppla->tlatch + index;

                ptlatch->button = ppla->local_latch.b;

                // reduce the resolution of the motion to match the network packets
                ptlatch->x = FLOOR( ppla->local_latch.x * SHORTLATCH ) / SHORTLATCH;
                ptlatch->y = FLOOR( ppla->local_latch.y * SHORTLATCH ) / SHORTLATCH;

                ptlatch->time = update_counter;

                ppla->tlatch_count++;
            }

            // determine the max amount of lag
            for ( cnt = 0; cnt < ppla->tlatch_count; cnt++ )
            {
                int loc_lag = update_wld - ppla->tlatch[index].time + 1;

                if ( loc_lag > 0 && ( size_t )loc_lag > numplatimes )
                {
                    numplatimes = loc_lag;
                }
            }
        }
    }

    ego_packet_dtor( &ego_pkt );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv sv_letPlayersJoin()
{
    /// @author ZZ
    /// @details This function finds all the players in the game

    ENetEvent event;
    STRING hostName;

    // Check all pending events for players joining
    while ( enet_host_service(( ENetHost * )egonet_get_myHost(), &event, 0 ) > 0 )
    {
        switch ( event.type )
        {
            case ENET_EVENT_TYPE_CONNECT:
                // Look up the hostname the player is connecting from
                enet_address_get_host( &event.peer->address, hostName, 64 );

                log_info( "sv_letPlayersJoin: A new player connected from %s:%u\n",
                          hostName, event.peer->address.port );

                // save the player data here.
                enet_address_get_host( &event.peer->address, hostName, 64 );

                strncpy( ServerState.base.client_name[ServerState.base.client_count], hostName, 16 );
                event.peer->data = &( ServerState.base.client_info[ServerState.base.client_count] );
                ServerState.base.client_count++;

                break;

            case ENET_EVENT_TYPE_RECEIVE:
                log_info( "sv_letPlayersJoin: Recieved a packet when we weren't expecting it...\n" );
                log_info( "\tIt came from %x:%u\n", event.peer->address.host, event.peer->address.port );

                // clean up the packet
                enet_packet_destroy( event.packet );
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                log_info( "sv_letPlayersJoin: A client disconnected!  Address %x:%u\n",
                          event.peer->address.host, event.peer->address.port );

                // Reset that peer's data
                event.peer->data = NULL;
                break;

            default:
                break;
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv sv_hostGame()
{
    /// @author ZZ
    /// @details This function tries to host a new session

    ENetAddress address;

    if ( egonet_on() )
    {
        ENetHost * phost;

        // Try to create a new session
        address.host = ENET_HOST_ANY;
        address.port = NET_EGOBOO_PORT;

        log_info( "sv_hostGame: Creating game on port %d\n", NET_EGOBOO_PORT );

        phost = ENET_HOST_CREATE( &address, MAX_PLAYER, 0, 0 );
        egonet_set_myHost( phost );

        if ( NULL == phost )
        {
            log_info( "sv_hostGame: Could not create network connection!\n" );
            return rv_error;
        }

        // Try to create a host player
        //return create_player(true);
        ServerState.base.am_host = true;

        // Moved from net_sayHello because there they cause a race issue
        egonet_set_waitingforclients( true );
        net_players_loaded = 0;
    }

    // Run in solo mode
    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv sv_handlePacket( enet_packet_t * enet_pkt )
{
    Uint16 header;
    PLA_REF player;
    int time;

    ego_packet_t  ego_pkt;
    bool handled;

    Uint8  ub;
    Uint32 ui;

    ego_packet_ctor( &ego_pkt );

    if ( !egonet_on() || NULL == enet_pkt ) return rv_error;

    // assume the best
    handled = true;

    // log the packet
    log_info( "sv_handlePacket: Received " );

    // begin cracking
    enet_packet_readUint16( enet_pkt, &header );

    switch ( header )
    {
        case TO_HOST_MODULEOK:
            log_info( "TO_HOST_MODULEOK\n" );
            if ( egonet_get_hostactive() )
            {
                net_players_ready++;
                if ( net_players_ready >= ServerState.base.client_count )
                {
                    egonet_set_readytostart( true );
                }
            }
            break;

        case TO_HOST_LATCH:
            log_info( "TO_HOST_LATCH\n" );
            if ( egonet_get_hostactive() )
            {
                while ( enet_packet_remainingSize( enet_pkt ) > 0 )
                {
                    Sint16 ss;
                    latch_t tmp_latch;

                    enet_packet_readUint8( enet_pkt, &ub );
                    player = ( PLA_REF )ub;
                    enet_packet_readUint32( enet_pkt, &ui );
                    time = ( int )ui;
                    enet_packet_readUint32( enet_pkt, &( tmp_latch.b ) );

                    if ( !enet_packet_readSint16( enet_pkt, & ss ) )
                    {
                        tmp_latch.x = 0.0f;
                    }
                    else
                    {
                        tmp_latch.x = ss / SHORTLATCH;
                    }

                    if ( !enet_packet_readSint16( enet_pkt, & ss ) )
                    {
                        tmp_latch.y = 0.0f;
                    }
                    else
                    {
                        tmp_latch.y = ss / SHORTLATCH;
                    }

                    PlaStack_add_tlatch( player, time, tmp_latch );
                }

            }
            break;

        case TO_HOST_IM_LOADED:
            log_info( "TO_HOST_IMLOADED\n" );
            if ( egonet_get_hostactive() )
            {
                net_players_loaded++;
                if ( net_players_loaded == ServerState.base.client_count )
                {
                    // Let the games begin...
                    egonet_set_waitingforclients( false );
                    ego_packet_begin( &ego_pkt );
                    ego_packet_addUint16( &ego_pkt, TO_REMOTE_START );
                    egonet_broadcastPacketGuaranteed( &ego_pkt );
                }
            }
            break;

            //case TO_HOST_RTS:
            //    log_info( "TO_HOST_RTS\n" );
            //    if ( egonet_get_hostactive() )
            //    {
            //        whichorder = get_empty_order();
            //        if(whichorder < MAXORDER)
            //          {
            //          // Add the order on the host machine
            //          cnt = 0;
            //          while(cnt < MAXSELECT)
            //          {
            //            enet_packet_readUint8( enet_pkt, net_order[whichorder].who + cnt );
            //            cnt++;
            //          }
            //          enet_packet_readUint32( enet_pkt, &what );
            //          when = update_wld + orderlag;
            //          net_order[whichorder].what = what;
            //          net_order[whichorder].when = when;

            //          // Send the order off to everyone else
            //          ego_packet_begin( &ego_pkt );
            //          ego_packet_addUint16( &ego_pkt, TO_REMOTE_RTS);
            //          cnt = 0;
            //          while(cnt < MAXSELECT)
            //          {
            //            ego_packet_addUint8( &ego_pkt, net_order[whichorder].who[cnt]);
            //            cnt++;
            //          }
            //          ego_packet_addUint32( &ego_pkt, what);
            //          ego_packet_addUint32( &ego_pkt, when);
            //          egonet_broadcastPacketGuaranteed( &ego_pkt );
            //          }
            //    }
            //    break;

        default:
            handled = false;
            break;
    }

    // deconstruct the packet(s)
    ego_packet_dtor( &ego_pkt );

    return handled ? rv_success : rv_fail;
}

