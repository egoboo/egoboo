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

/// @file client.c
/// @brief This code is not currently in use.

#include <egolib/log.h>
#include <egolib/egoboo_setup.h>
#include <egolib/strutil.h>

#include "network_client.h"

#include "game.h"
#include "player.h"
#include "menu.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Global ClientState instance
ClientState_t ClientState = { BASE_CLIENT_STATE_INIT };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void cl_frameStep()
{
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv cl_talkToHost()
{
    /// @author ZZ
    /// @details This function sends the latch packets to the host machine

    PLA_REF player;
    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    // Let the players respawn
    if ( SDLKEYDOWN( SDLK_SPACE )
         && ( local_stats.allpladead || PMod->respawnanytime )
         && PMod->respawnvalid
         && cfg.difficulty < GAME_HARD
         && !keyb.chat_mode )
    {
        player = 0;

        while ( player < MAX_PLAYER )
        {
            if ( PlaStack.lst[player].valid && PlaStack.lst[player].pdevice != NULL )
            {
                SET_BIT( PlaStack.lst[player].local_latch.b, LATCHBUTTON_RESPAWN );  // Press the respawn button...
            }

            player++;
        }
    }

    // Start talkin'
    if ( egonet_on() && !egonet_get_hostactive() )
    {
        ego_packet_begin( &ego_pkt );
        ego_packet_addUint16( &ego_pkt, TO_HOST_LATCH );        // The message header

        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            // Find the local players
            if ( PlaStack.lst[player].valid && PlaStack.lst[player].pdevice != NULL )
            {
                ego_packet_addUint8( &ego_pkt, REF_TO_INT( player ) );                         // The player index
                ego_packet_addUint32( &ego_pkt, PlaStack.lst[player].local_latch.b );             // Player button states
                ego_packet_addSint16( &ego_pkt, PlaStack.lst[player].local_latch.x*SHORTLATCH );  // Player motion
                ego_packet_addSint16( &ego_pkt, PlaStack.lst[player].local_latch.y*SHORTLATCH );  // Player motion
            }
        }

        // Send it to the host
        egonet_sendPacketToPeer( &ego_pkt, ClientState.base.gameHost );
    }

    ego_packet_dtor( &ego_pkt );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv cl_joinGame( const char* hostname )
{
    /// @author ZZ
    /// @details This function tries to join one of the sessions we found

    ENetAddress address;
    ENetEvent event;

    if ( egonet_on() )
    {
        ENetHost * phost;
        ENetPeer * pgame;

        log_info( "cl_joinGame: Creating client network connection... " );
        // Create my host thingamabober
        /// @todo Should I limit client bandwidth here?
        phost = ENET_HOST_CREATE( NULL, 1, 0, 0 );

        egonet_set_myHost( phost );
        if ( NULL == phost )
        {
            // can't create a network connection at all
            log_info( "Failure!\n" );
            return rv_fail;
        }
        else
        {
            log_info( "Success!\n" );
        };

        // Now connect to the remote host
        log_info( "cl_joinGame: Attempting to connect to %s:%d\n", hostname, NET_EGOBOO_PORT );
        enet_address_set_host( &address, hostname );
        address.port = NET_EGOBOO_PORT;

        pgame = ENET_HOST_CONNECT(( ENetHost * )egonet_get_myHost(), &address, NET_EGOBOO_NUM_CHANNELS );
        ClientState.base.gameHost = pgame;

        if ( NULL == ClientState.base.gameHost )
        {
            log_info( "cl_joinGame: No available peers to create a connection!\n" );
            return rv_fail;
        }

        // Wait for up to 5 seconds for the connection attempt to succeed
        if ( enet_host_service(( ENetHost * )egonet_get_myHost(), &event, 5000 ) > 0 &&
             event.type == ENET_EVENT_TYPE_CONNECT )
        {
            log_info( "cl_joinGame: Connected to %s:%d\n", hostname, NET_EGOBOO_PORT );
            return rv_success;
            // return create_player(bfalse);
        }
        else
        {
            log_info( "cl_joinGame: Could not connect to %s:%d!\n", hostname, NET_EGOBOO_PORT );
        }
    }

    return rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv cl_handlePacket( enet_packet_t * enet_pkt )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings

    PLA_REF player;
    Uint32 stamp;
    int time;

    ego_packet_t  ego_pkt;
    bool_t handled;

    Uint8  ub;
    Sint16 ss;

    ego_packet_ctor( &ego_pkt );

    if ( !egonet_on() || NULL == enet_pkt ) return rv_error;

    // assume the best
    handled = btrue;

    // log the packet
    log_info( "sv_handlePacket: Received " );

    // begin cracking
    enet_packet_readUint16( enet_pkt, &header );

    switch ( header )
    {
        case TO_REMOTE_MODULE:
            log_info( "TO_REMOTE_MODULE\n" );
            if ( !egonet_get_hostactive() && !egonet_get_readytostart() )
            {
                enet_packet_readUint32( enet_pkt, &PMod->seed );
                enet_packet_readString( enet_pkt,  filename, sizeof( filename ) );

                pickedmodule_index         = -1;
                pickedmodule_path[0]       = CSTR_END;
                pickedmodule_name[0]       = CSTR_END;
                pickedmodule_write_path[0] = CSTR_END;

                pickedmodule_index = mnu_get_mod_number( filename );

                // Check to see if the module exists
                if ( -1 != pickedmodule_index )
                {
                    strncpy( pickedmodule_path,       mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pickedmodule_path ) );
                    strncpy( pickedmodule_name,       mnu_ModList_get_name( pickedmodule_index ), SDL_arraysize( pickedmodule_name ) );
                    strncpy( pickedmodule_write_path, mnu_ModList_get_dest_path( pickedmodule_index ), SDL_arraysize( pickedmodule_write_path ) );

                    pickedmodule_ready = btrue;

                    // Make ourselves ready
                    egonet_set_readytostart( btrue );

                    // Tell the host we're ready
                    ego_packet_begin( &ego_pkt );
                    ego_packet_addUint16( &ego_pkt, TO_HOST_MODULEOK );
                    egonet_sendPacketToPeerGuaranteed( &ego_pkt, ClientState.base.gameHost );
                }
                else
                {
                    // The module doesn't exist locally
                    pickedmodule_ready = bfalse;

                    // Halt the process
                    egonet_set_readytostart( bfalse );

                    // Tell the host we're not ready
                    ego_packet_begin( &ego_pkt );
                    ego_packet_addUint16( &ego_pkt, TO_HOST_MODULEBAD );
                    egonet_sendPacketToPeerGuaranteed( &ego_pkt, ClientState.base.gameHost );
                }
            }
            break;

        case TO_REMOTE_START:
            log_info( "TO_REMOTE_START\n" );
            if ( !egonet_get_hostactive() )
            {
                egonet_set_waitingforclients( bfalse );
            }
            break;

            //case TO_REMOTE_RTS:
            //    log_info( "TO_REMOTE_RTS\n" );
            //    if ( !pnet->hostactive )
            //    {
            //            whichorder = get_empty_order();
            //            if(whichorder < MAXORDER)
            //            {
            //              // Add the order on the remote machine
            //              cnt = 0;
            //              while(cnt < MAXSELECT)
            //              {
            //                who = enet_packet_readUint8( enet_pkt, net_order[whichorder].who + cnt );
            //                cnt++;
            //              }
            //              enet_packet_readUint32( enet_pkt, &(net_order[whichorder].what) );
            //              enet_packet_readUint32( enet_pkt, &(net_order[whichorder].when) );
            //            }
            //    }
            //    break;

        case TO_REMOTE_LATCH:
            log_info( "TO_REMOTE_LATCH\n" );
            if ( !egonet_get_hostactive() )
            {
                enet_packet_readUint32( enet_pkt, &stamp );
                time = stamp & LAGAND;
                if ((( Uint32 )( ~0 ) ) == nexttimestamp )
                {
                    nexttimestamp = stamp;
                }
                if ( stamp < nexttimestamp )
                {
                    log_warning( "net_dispatchEvent: OUT OF ORDER PACKET\n" );
                    outofsync = btrue;
                }
                if ( stamp <= update_wld )
                {
                    log_warning( "net_dispatchEvent: LATE PACKET\n" );
                    outofsync = btrue;
                }
                if ( stamp > nexttimestamp )
                {
                    log_warning( "net_dispatchEvent: MISSED PACKET\n" );
                    nexttimestamp = stamp;  // Still use it
                    outofsync = btrue;
                }
                if ( stamp == nexttimestamp )
                {
                    // Remember that we got it
                    numplatimes++;

                    // Read latches for each player sent
                    while ( enet_packet_remainingSize( enet_pkt ) > 0 )
                    {
                        enet_packet_readUint8( enet_pkt, &ub );
                        player = ( PLA_REF )ub;

                        enet_packet_readUint32( enet_pkt, &( PlaStack.lst[player].tlatch[time].button ) );

                        if ( enet_packet_readSint16( enet_pkt, &ss ) )
                        {
                            PlaStack.lst[player].tlatch[time].x =  ss / SHORTLATCH;
                        }
                        else
                        {
                            PlaStack.lst[player].tlatch[time].x =  0.0f;
                        }

                        if ( enet_packet_readSint16( enet_pkt, &ss ) )
                        {
                            PlaStack.lst[player].tlatch[time].y =  ss / SHORTLATCH;
                        }
                        else
                        {
                            PlaStack.lst[player].tlatch[time].y =  0.0f;
                        }
                    }

                    nexttimestamp = stamp + 1;
                }
            }
            break;

        default:
            handled = bfalse;
            break;
    }

    // deconstruct the packet(s)
    ego_packet_dtor( &ego_pkt );

    return handled ? rv_success : rv_fail;
}
