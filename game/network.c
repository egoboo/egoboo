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

/// @file network.c
/// @brief Egoboo networking implementation
/// @details Shuttles bits across the network, using Enet.  Networked play doesn't
/// really work at the moment.

#include <stdarg.h>

#include <egolib/file_common.h>
#include <egolib/log.h>
#include <egolib/strutil.h>
#include <egolib/vfs.h>
#include <egolib/egoboo_setup.h>
#include <egolib/file_formats/module_file.h>

#include "network.h"
#include "network_server.h"
#include "network_client.h"

#include "input.h"
#include "game.h"
#include "menu.h"
#include "player.h"
#include "egoboo.h"

#include "char.inl"

// this include must be the absolute last include
#include <egolib/mem.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_net_order;
typedef struct s_net_order net_order_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define COPYSIZE    0x1000
#define TOTALSIZE   0x00200000L

//#define STARTTALK   10

#define MAXORDER  256
#define MAXSELECT  16

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Network orders
struct s_net_order
{
    unsigned char           valid;
    unsigned char           who[MAXSELECT];
    unsigned int            what;
    unsigned int            when;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static net_order_t net_order[MAXORDER];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32      numplatimes = 0;

int     net_players_ready  = 0;         // Number of players ready to start
int     net_players_loaded = 0;

chat_buffer_t net_chat = { 0, EMPTY_CSTR };

Uint32 nexttimestamp;                          // Expected timestamp

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void net_sayHello()
{
    /// @details ZZ@> This function lets everyone know we're here

    if ( !egonet_on() )
    {
        egonet_set_waitingforclients( bfalse );
    }
    else if ( egonet_get_hostactive() )
    {
        log_info( "egonet_sayHello: Server saying hello.\n" );

        net_players_loaded++;
        if ( net_players_loaded >= ServerState.base.client_count )
        {
            egonet_set_waitingforclients( bfalse );
        }
    }
    else
    {
        ego_packet_t ego_pkt;

        log_info( "net_sayHello: Client saying hello.\n" );

        ego_packet_ctor( &ego_pkt );
        ego_packet_addUint16( &ego_pkt, TO_HOST_IM_LOADED );
        egonet_sendPacketToPeerGuaranteed( &ego_pkt, ClientState.base.gameHost );

        ego_packet_dtor( &ego_pkt );
    }
}

//--------------------------------------------------------------------------------------------
void net_unbuffer_player_latches()
{
    /// @details ZZ@> This function sets character latches based on player input to the host

    PLA_REF ipla;
    CHR_REF character;

    // get the "network" latch for each valid player
    numplatimes = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        Uint32 latch_count, tnc;
        latch_t tmp_latch;
        player_t * ppla;
        time_latch_t * tlatch_list;

        if ( !PlaStack.lst[ipla].valid ) continue;
        ppla = PlaStack_get_ptr( ipla );
        tlatch_list = ppla->tlatch;

        // copy the latch from last time
        tmp_latch = ppla->net_latch;

        // what are the minimum and maximum indices that can be applies this update?
        for ( tnc = 0; tnc < ppla->tlatch_count; tnc++ )
        {
            int dt;

            dt = update_wld - tlatch_list[tnc].time;

            if ( dt < 0 )
                break;
        }
        latch_count = tnc;

        if ( 1 == latch_count )
        {
            // there is just one valid latch
            tmp_latch.x = tlatch_list[0].x;
            tmp_latch.y = tlatch_list[0].y;
            tmp_latch.b = tlatch_list[0].button;

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, Just one latch for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, ChrList.lst[ppla->index].Name );
        }
        else if ( latch_count > 1 )
        {
            int weight, weight_sum;
            int dt;

            // estimate the best latch value by weighting latches that are back in time
            // by dt*dt. This estimates the effect of actually integrating the position over
            // that much time without the hastle of actually integrating the trajectory.

            // blank the current latch so that we can sum the latch values
            latch_init( &( tmp_latch ) );

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

            numplatimes = MAX( numplatimes, latch_count );
            if ( weight_sum > 0.0f )
            {
                tmp_latch.x /= ( float )weight_sum;
                tmp_latch.y /= ( float )weight_sum;
            }

            //log_info( "<<%1.4f, %1.4f>, 0x%x>, %d, multiple latches for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, latch_count, ChrList.lst[ppla->index].Name );
        }
        else
        {
            // there are no valid latches
            // do nothing. this lets the old value of the latch persist.
            // this might be a decent guess as to what to do if a packet was
            // dropped?
            //log_info( "<<%1.4f, %1.4f>, 0x%x>, latch dead reckoning for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, ChrList.lst[ppla->index].Name );
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
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        chr_t * pchr;
        player_t * ppla;

        if ( !PlaStack.lst[ipla].valid ) continue;
        ppla = PlaStack_get_ptr( ipla );

        character = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList_get_ptr( character );

        pchr->latch = ppla->net_latch;
    }

    // Let players respawn
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        chr_t * pchr;
        player_t * ppla;

        if ( !PlaStack.lst[ipla].valid ) continue;
        ppla = PlaStack_get_ptr( ipla );

        character = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList_get_ptr( character );

        if ( cfg.difficulty < GAME_HARD && HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_RESPAWN ) && PMod->respawnvalid )
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
            UNSET_BIT( pchr->latch.b, LATCHBUTTON_RESPAWN );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_initialize()
{
    /// @details ZZ@> This starts up the network and logs whatever goes on

    egonet_initialize( &( ServerState.base ), &( ClientState.base ), MAX_PLAYER, cfg.network_allowed );
}

//--------------------------------------------------------------------------------------------
bool_t net_begin()
{
    return egonet_begin( cfg.network_allowed );
}

//--------------------------------------------------------------------------------------------
bool_t net_end()
{
    return egonet_end();
}

//--------------------------------------------------------------------------------------------
void net_shutDown()
{
    log_info( "net_shutDown: Turning off networking.\n" );

    egonet_shutDown();
}

//--------------------------------------------------------------------------------------------
void find_open_sessions()
{
    /*PORT
    /// @details ZZ@> This function finds some open games to join

    DPSESSIONDESC2      sessionDesc;
    HRESULT             hr;
    if(egonet_on())
      {
    gnet.session_count = 0;
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "  Looking for open games...\n");
    ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = NETWORKID;
    hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "    %d sessions found\n", gnet.session_count);
      }
    */
}

//--------------------------------------------------------------------------------------------
void stop_players_from_joining()
{
    /// @details ZZ@> This function stops players from joining a game
}

//--------------------------------------------------------------------------------------------
void net_send_message()
{
    /// @details ZZ@> sends the message in the keyboard buffer to all other players

    if ( keyb.chat_mode || !keyb.chat_done ) return;

    // if(egonet_on())
    // {
    //   start_building_packet();
    //   add_packet_us(TO_ANY_TEXT);
    //   add_packet_sz(net_chat.buffer);
    //   send_packet_to_all_players();
    // }
}

//--------------------------------------------------------------------------------------------
player_t* chr_get_ppla( const CHR_REF ichr )
{
    PLA_REF iplayer;

    if ( !INGAME_CHR( ichr ) ) return NULL;
    iplayer = ChrList.lst[ichr].is_which_player;

    if ( !VALID_PLA( iplayer ) ) return NULL;

    return PlaStack_get_ptr( iplayer );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void net_count_players()
{
    //int cnt;
    PLA_REF ipla;

    // count the valid connections
    //BaseServerState_count_clients( &(ServerState.base) );

    // count the players
    ServerState.player_count = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        if ( !PlaStack.lst[ipla].valid ) continue;

        // count the total number of players
        ServerState.player_count++;
    }
}

//--------------------------------------------------------------------------------------------
// functions required for egolib implementation
//--------------------------------------------------------------------------------------------

egolib_rv egonet_dispatchEvent( ENetEvent *event )
{
    Uint16 header;

    enet_packet_t enet_pkt;
    ego_packet_t  ego_pkt;
    bool_t handled;

    ego_packet_ctor( &ego_pkt );
    enet_packet_ctor( &enet_pkt );

    // assume the best
    handled = btrue;

    if ( !egonet_on() || NULL == event )
    {
        return rv_fail;
    }

    // empty event?
    if ( NULL == event->packet )
    {
        return rv_error;
    }

    // log the packet
    log_info( "net_dispatchEvent: Received " );

    // initialize the packet container
    if ( !enet_packet_startReading( &enet_pkt, event->packet ) )
    {
        return rv_fail;
    }

    // begin cracking
    enet_packet_readUint16( &enet_pkt, &header );

    if ( !handled )
    {
        handled = ( rv_success == egonet_handlePacket( &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == sv_handlePacket( &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == cl_handlePacket( &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == netfile_handleEvent( event ) );
    }

    // deconstruct the packet(s)
    ego_packet_dtor( &ego_pkt );
    enet_packet_dtor( &enet_pkt );

    return handled ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv egonet_handlePacket( enet_packet_t * enet_pkt )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings

    ego_packet_t  ego_pkt;
    bool_t handled;

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
        case TO_ANY_TEXT:
            log_info( "TO_ANY_TEXT\n" );
            enet_packet_readString( enet_pkt, filename, sizeof( filename ) );
            debug_printf( filename );
            break;

        default:
            handled = bfalse;
            break;
    }

    // deconstruct the packet(s)
    ego_packet_dtor( &ego_pkt );

    return handled ? rv_success : rv_fail;
}

