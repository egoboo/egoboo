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

#include "network.h"
#include "server.h"
#include "client.h"

#include "file_common.h"
#include "log.h"
#include "input.h"
#include "game.h"
#include "menu.h"
#include "player.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo.h"

#include "file_formats/module_file.h"

#include "char.inl"

#include <stdarg.h>

// this include must be the absolute last include
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_net_order;
typedef struct s_net_order net_order_t;

struct s_NetFileTransfer;
typedef struct s_NetFileTransfer NetFileTransfer;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define COPYSIZE    0x1000
#define TOTALSIZE   0x00200000L

//#define STARTTALK   10

#define MAXORDER  256
#define MAXSELECT  16

/// All the supported network messages
enum NetworkMessage
{
    NETFILE_TRANSFER       = 10001,  // Packet contains a file.
    NETFILE_TRANSFER_OK         = 10002,  // Acknowledgement packet for a file send
    NETFILE_CREATE_DIRECTORY    = 10003,  // Tell the peer to create the named directory
    NETFILE_DONE_SENDING  = 10009,  // Sent when there are no more files to send.
    NETFILE_NUM_TO_SEND   = 10010  // Let the other person know how many files you're sending
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of the network code used in old-egoboo
struct s_net_instance
{
    bool_t  on;                      ///< Try to connect?
    bool_t  serviceon;               ///< Do I need to free the interface?
    bool_t  hostactive;              ///< Hosting?
    bool_t  readytostart;            ///< Ready to hit the Start Game button?
    bool_t  waitingforplayers;       ///< Has everyone talked to the host?

    int     service;                                  ///< Which service is being used?
    int     service_count;                            ///< How many we found
    char    service_name[MAXSERVICE][NETNAMESIZE];    ///< Names of services

    int     session_count;                            ///< How many we found
    char    session_name[MAXSESSION][NETNAMESIZE];    ///< Names of sessions

    int     local_machine;                            ///< 0 is host, 1 is 1st remote, 2 is 2nd...

    ENetHost* myHost;
};

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
// cracking ENet packets

struct s_enet_packet
{
    ENetPacket*    ptr;
    size_t         read_location;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data for network file transfers
struct s_NetFileTransfer
{
    char sourceName[NET_MAX_FILE_NAME];
    char destName[NET_MAX_FILE_NAME];
    ENetPeer *target;
};

/// Network file transfer queue
static NetFileTransfer net_transferStates[NET_MAX_FILE_TRANSFERS];
static int net_numFileTransfers = 0;  ///< Queue count
static int net_fileTransferHead = 0;  ///< Queue start index
static int net_fileTransferTail = 0;  ///< Queue end index
static int net_waitingForXferAck = 0; ///< Queue state

static Uint8  * transferBuffer = NULL;
static size_t   transferSize = 0;

// Receiving files
static NetFileTransfer net_receiveState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int  numfile = 0;                                // For network copy
static int  numfilesent = 0;                            // For network copy
static int  numfileexpected = 0;                        // For network copy
static int  numplayerrespond = 0;

static net_instance_t gnet = { bfalse, bfalse, bfalse, bfalse, bfalse };

//static net_order_t net_order[MAXORDER];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32      numplatimes = 0;

FILE *      globalnetworkerr = NULL;

int     net_players_ready  = 0;         // Number of players ready to start
int     net_players_loaded = 0;

net_instance_t * PNet = &gnet;

chat_buffer_t net_chat = { 0, EMPTY_CSTR };

Uint32 nexttimestamp;                          // Expected timestamp

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static egoboo_rv net_dispatchEvent( ENetEvent *event );
static egoboo_rv netfile_handleEvent( net_instance_t * pnet, ENetEvent *event );
static egoboo_rv net_handlePacket( net_instance_t * pnet, enet_packet_t * enet_pkt );

static void net_copyFileToAllPlayersOld_vfs( const char *source, const char *dest );
static void net_copyFileToPeerOld_vfs( const char *source, const char *dest, ENetPeer *peer );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void net_close_session()
{
    size_t i, numPeers;
    ENetEvent event;

    /// @details ZZ@> This function gets the computer out of a network game

    if ( gnet.on )
    {
        if ( ServerState.am_host )
        {
            // Disconnect the peers
            numPeers = gnet.myHost->peerCount;

            for ( i = 0; i < numPeers; i++ )
            {
#if defined(ENET11)
                enet_peer_disconnect( &gnet.myHost->peers[i], 0 );
#else
                enet_peer_disconnect( &gnet.myHost->peers[i] );
#endif
            }

            // Allow up to 5 seconds for peers to drop
            while ( enet_host_service( gnet.myHost, &event, 5000 ) )
            {
                switch ( event.type )
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy( event.packet );
                        break;

                    case ENET_EVENT_TYPE_DISCONNECT:
                        log_info( "net_close_session: Peer id %d disconnected gracefully.\n", event.peer->address.host );
                        numPeers--;
                        break;

                    default:
                        break;
                }
            }

            // Forcefully disconnect any peers leftover
            for ( i = 0; i < gnet.myHost->peerCount; i++ )
            {
                enet_peer_reset( &gnet.myHost->peers[i] );
            }
        }

        log_info( "net_close_session: Disconnecting from network.\n" );

        enet_host_destroy( gnet.myHost );
        gnet.myHost = NULL;

        ClientState.gameHost = NULL;
    }
}

//--------------------------------------------------------------------------------------------
bool_t net_sendPacketToAllPlayers( ego_packet_t * ptr )
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return bfalse;

    packet = enet_packet_create( ptr->buffer, ptr->size, 0 );

    if ( NULL != packet )
    {
        enet_host_broadcast( gnet.myHost, NET_UNRELIABLE_CHANNEL, packet );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_sendPacketToAllPlayersGuaranteed( ego_packet_t * ptr )
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return bfalse;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    if ( NULL != packet )
    {
        enet_host_broadcast( gnet.myHost, NET_GUARANTEED_CHANNEL, packet );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_sendPacketToOnePlayerGuaranteed( ego_packet_t * ptr, int player )
{
    /// @details ZZ@> This function sends a packet to one of the players

    ENetPacket *packet;
    int enet_retval;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return bfalse;

    // valid player?
    if ( player >= ServerState.player_count ) return bfalse;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( &gnet.myHost->peers[player], NET_GUARANTEED_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
bool_t net_sendPacketToPeer( ego_packet_t * ptr, ENetPeer *peer )
{
    /// @details JF@> This function sends a packet to a given peer

    int enet_retval;
    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return bfalse;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( peer, NET_UNRELIABLE_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
bool_t net_sendPacketToPeerGuaranteed( ego_packet_t * ptr, ENetPeer *peer )
{
    /// @details JF@> This funciton sends a packet to a given peer, with guaranteed delivery

    int enet_retval;
    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return bfalse;

    packet = enet_packet_create( ptr->buffer, ptr->size, 0 );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( peer, NET_GUARANTEED_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
void net_copyFileToAllPlayers( const char *source, const char *dest )
{
    /// @details JF@> This function queues up files to send to all the hosts.
    ///    @todo Deal with having to send to up to MAX_PLAYER players...

    NetFileTransfer *state;
    if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // net_fileTransferTail should already be pointed at an open
        // slot in the queue.
        state = &( net_transferStates[net_fileTransferTail] );
        EGOBOO_ASSERT( CSTR_END == state->sourceName[0] );

        // Just do the first player for now
        state->target = &gnet.myHost->peers[0];
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        net_numFileTransfers++;
        net_fileTransferTail++;
        if ( net_fileTransferTail >= NET_MAX_FILE_TRANSFERS )
        {
            net_fileTransferTail = 0;
        }
        if ( net_fileTransferTail == net_fileTransferHead )
        {
            log_warning( "net_copyFileToAllPlayers: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToAllPlayersOld_vfs( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the host to every remote computer.
    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
    ///    that can be sent is 2 Megs ( TOTALSIZE ).

    vfs_FILE* fileread;
    int packet_size, packet_start;
    int filesize;
    int fileisdir;
    char cTmp;
    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    log_info( "net_copyFileToAllPlayers: %s, %s\n", source, dest );
    if ( gnet.on && gnet.hostactive )
    {
        fileisdir = vfs_isDirectory( source );
        if ( fileisdir )
        {
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_DIR );
            ego_packet_addString( &ego_pkt, dest );
            net_sendPacketToAllPlayersGuaranteed( &ego_pkt );
        }
        else
        {
            fileread = vfs_openReadB( source );
            if ( fileread )
            {
                filesize = vfs_fileLength( fileread );
                vfs_seek( fileread, 0 );
                if ( filesize > 0 && filesize < TOTALSIZE )
                {
                    packet_size = 0;
                    packet_start = 0;
                    numfilesent++;

                    ego_packet_begin( &ego_pkt );
                    ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_FILE );
                    ego_packet_addString( &ego_pkt, dest );
                    ego_packet_addUint32( &ego_pkt, filesize );
                    ego_packet_addUint32( &ego_pkt, packet_start );

                    while ( packet_start < filesize )
                    {
                        // This will probably work...
                        // vfs_read((net_packet.buffer + net_packet.head), COPYSIZE, 1, fileread);

                        // But I'll leave it alone for now
                        vfs_scanf( fileread, "%c", &cTmp );

                        ego_packet_addUint8( &ego_pkt, cTmp );
                        packet_size++;
                        packet_start++;
                        if ( packet_size >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToAllPlayersGuaranteed( &ego_pkt );
                            enet_host_flush( gnet.myHost );

                            // Start on the next 4K
                            packet_size = 0;
                            ego_packet_begin( &ego_pkt );
                            ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_FILE );
                            ego_packet_addString( &ego_pkt, dest );
                            ego_packet_addUint32( &ego_pkt, filesize );
                            ego_packet_addUint32( &ego_pkt, packet_start );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToAllPlayersGuaranteed( &ego_pkt );
                }

                vfs_close( fileread );
            }
        }
    }

    ego_packet_dtor( &ego_pkt );
}

//--------------------------------------------------------------------------------------------
void net_copyFileToPeer( const char *source, const char *dest, ENetPeer *peer )
{
    NetFileTransfer *state;

    /// @details JF@> New function merely queues up a new file to be sent

    // If this is the host, just copy the file locally
    if ( gnet.hostactive )
    {
        // Simulate a network transfer
        if ( vfs_isDirectory( source ) )
        {
            vfs_mkdir( dest );
        }
        else
        {
            vfs_copyFile( source, dest );
        }

        return;
    }
    if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // net_fileTransferTail should already be pointed at an open
        // slot in the queue.
        state = &( net_transferStates[net_fileTransferTail] );
        EGOBOO_ASSERT( CSTR_END == state->sourceName[0] );

        state->target = peer;
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        net_numFileTransfers++;
        net_fileTransferTail++;
        if ( net_fileTransferTail >= NET_MAX_FILE_TRANSFERS )
        {
            net_fileTransferTail = 0;
        }
        if ( net_fileTransferTail == net_fileTransferHead )
        {
            log_warning( "net_copyFileToPeer: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToPeerOld_vfs( const char *source, const char *dest, ENetPeer *peer )
{
    /// @details ZZ@> This function copies a file on the remote to the host computer.
    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
    ///    that can be sent is 2 Megs ( TOTALSIZE ).

    vfs_FILE* fileread;
    int packet_size, packet_start;
    int filesize;
    int fileisdir;
    char cTmp;
    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    log_info( "net_copyFileToPeer: " );
    fileisdir = vfs_isDirectory( source );
    if ( gnet.hostactive )
    {
        // Simulate a network transfer
        if ( fileisdir )
        {
            log_info( "Creating local directory %s\n", dest );
            vfs_mkdir( dest );
        }
        else
        {
            log_info( "Copying local file %s --> %s\n", source, dest );
            vfs_copyFile( source, dest );
        }
    }
    else
    {
        if ( fileisdir )
        {
            log_info( "Creating directory on host: %s\n", dest );
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_DIR );
            ego_packet_addString( &ego_pkt, dest );
//     net_sendPacketToAllPlayersGuaranteed( &ego_pkt );
            net_sendPacketToPeer( &ego_pkt, ClientState.gameHost );
        }
        else
        {
            log_info( "Copying local file to host file: %s --> %s\n", source, dest );
            fileread = vfs_openReadB( source );
            if ( fileread )
            {
                filesize = vfs_fileLength( fileread );
                vfs_seek( fileread, 0 );
                if ( filesize > 0 && filesize < TOTALSIZE )
                {
                    numfilesent++;
                    packet_size = 0;
                    packet_start = 0;
                    ego_packet_begin( &ego_pkt );
                    ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_FILE );
                    ego_packet_addString( &ego_pkt, dest );
                    ego_packet_addUint32( &ego_pkt, filesize );
                    ego_packet_addUint32( &ego_pkt, packet_start );

                    while ( packet_start < filesize )
                    {
                        vfs_scanf( fileread, "%c", &cTmp );
                        ego_packet_addUint8( &ego_pkt, cTmp );
                        packet_size++;
                        packet_start++;
                        if ( packet_size >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToPeerGuaranteed( &ego_pkt, peer );
                            enet_host_flush( gnet.myHost );

                            // Start on the next 4K
                            packet_size = 0;
                            ego_packet_begin( &ego_pkt );
                            ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_FILE );
                            ego_packet_addString( &ego_pkt, dest );
                            ego_packet_addUint32( &ego_pkt, filesize );
                            ego_packet_addUint32( &ego_pkt, packet_start );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToPeerGuaranteed( &ego_pkt, peer );
                }

                vfs_close( fileread );
            }
        }
    }

    ego_packet_dtor( &ego_pkt );
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToPeer( const char *dirname, const char *todirname, ENetPeer * peer )
{
    /// @details ZZ@> This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "net_copyDirectoryToPeer: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        net_copyFileToPeer( dirname, todirname, peer );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            // Also avoid copying directories in general.
            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            if ( '.' == searchResult[0] || vfs_isDirectory( fromname ) )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );
                continue;
            }

            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );

            net_copyFileToPeer( fromname, toname, peer );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToAllPlayers( const char *dirname, const char *todirname )
{
    /// @details ZZ@> This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "net_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        net_copyFileToAllPlayers( dirname, todirname );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            if ( '.' == searchResult[0] )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );

                continue;
            }

            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );
            net_copyFileToAllPlayers( fromname, toname );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void net_sayHello()
{
    /// @details ZZ@> This function lets everyone know we're here

    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    if ( !gnet.on )
    {
        gnet.waitingforplayers = bfalse;
    }
    else if ( gnet.hostactive )
    {
        log_info( "net_sayHello: Server saying hello.\n" );
        net_players_loaded++;
        if ( net_players_loaded >= ServerState.player_count )
        {
            gnet.waitingforplayers = bfalse;
        }
    }
    else
    {
        log_info( "net_sayHello: Client saying hello.\n" );
        ego_packet_begin( &ego_pkt );
        ego_packet_addUint16( &ego_pkt, TO_HOST_IM_LOADED );
        net_sendPacketToPeerGuaranteed( &ego_pkt, ClientState.gameHost );
    }

    ego_packet_dtor( &ego_pkt );
}

//--------------------------------------------------------------------------------------------
egoboo_rv net_dispatchEvent( ENetEvent *event )
{
    Uint16 header;

    enet_packet_t enet_pkt;
    ego_packet_t  ego_pkt;
    bool_t handled;

    ego_packet_ctor( &ego_pkt );
    enet_packet_ctor( &enet_pkt );

    // assume the best
    handled = btrue;

    if ( !gnet.on || NULL == event )
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
        handled = ( rv_success == net_handlePacket( &gnet, &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == sv_handlePacket( &gnet, &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == cl_handlePacket( &gnet, &enet_pkt ) );
    }

    if ( !handled )
    {
        handled = ( rv_success == netfile_handleEvent( &gnet, event ) );
    }

    // deconstruct the packet(s)
    ego_packet_dtor( &ego_pkt );
    enet_packet_dtor( &enet_pkt );

    return handled ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
void net_listen_for_packets()
{
    /// @details ZZ@> This function reads any new messages and sets the player latch and matrix needed
    ///    lists...

    ENetEvent event;

    if ( gnet.on )
    {
        // Listen for new messages
        while ( 0 != enet_host_service( gnet.myHost, &event, 0 ) )
        {
            switch ( event.type )
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    if ( !net_dispatchEvent( &event ) )
                    {
                        log_warning( " %s(%d) - packet not handled\n", __FUNCTION__, __LINE__ );
                    }
                    enet_packet_destroy( event.packet );
                    break;

                case ENET_EVENT_TYPE_CONNECT:
                    // don't allow anyone to connect during the game session
                    log_warning( "net_listen_for_packets: Client tried to connect during the game: %x:%u\n",
                                 event.peer->address.host, event.peer->address.port );
#if defined(ENET11)
                    enet_peer_disconnect( event.peer, 0 );
#else
                    enet_peer_disconnect( event.peer );
#endif
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:

                    // Is this a player disconnecting, or just a rejected connection
                    // from above?
                    if ( event.peer->data != 0 )
                    {
                        NetPlayerInfo_t *info = ( NetPlayerInfo_t * )event.peer->data;

                        // uh oh, how do we handle losing a player?
                        log_warning( "net_listen_for_packets: Player %d disconnected!\n",
                                     info->playerSlot );
                    }
                    break;

                default:
                    log_warning( "%s(%d) - event %d not handled\n", __FUNCTION__, __LINE__, event.type );
                    break;
            }
        }
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

    net_instance_ctor( &gnet );

    // Clear all the state variables to 0 to start.
    BLANK_ARY( ClientState.player_peers )
    BLANK_ARY( ServerState.player_info )
    BLANK_ARY( net_transferStates )
    BLANK_STRUCT( net_receiveState )

    ServerState.last_frame = ( Uint32 )( ~0 );

    if ( gnet.on )
    {
        // initialize enet
        log_info( "net_initialize: Initializing enet... " );
        if ( enet_initialize() != 0 )
        {
            log_info( "Failure!\n" );
            gnet.on = bfalse;
            gnet.serviceon = bfalse;
        }
        else
        {
            log_info( "Success!\n" );
            gnet.serviceon = btrue;
            gnet.service_count = 1;
        }
    }
    else
    {
        // We're not doing networking this time...
        log_info( "net_initialize: Networking not enabled.\n" );
    }
}

//--------------------------------------------------------------------------------------------
void net_shutDown()
{
    log_info( "net_shutDown: Turning off networking.\n" );

    net_instance_ctor( &gnet );

    enet_deinitialize();
}

//--------------------------------------------------------------------------------------------
void find_open_sessions()
{
    /*PORT
    /// @details ZZ@> This function finds some open games to join

    DPSESSIONDESC2      sessionDesc;
    HRESULT             hr;
    if(gnet.on)
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
/*void turn_on_service( int service )
{
    /// ZZ@> This function turns on a network service ( IPX, TCP, serial, modem )
}*/

//--------------------------------------------------------------------------------------------
int  net_pendingFileTransfers()
{
    return net_numFileTransfers;
}

//--------------------------------------------------------------------------------------------
void net_updateFileTransfers()
{
    NetFileTransfer *state;
    ENetPacket *packet;
    size_t nameLen, fileSize;
    Uint32 networkSize;
    vfs_FILE *file;
    char *p;

    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    // Are there any pending file sends?
    if ( net_numFileTransfers > 0 )
    {
        if ( !net_waitingForXferAck )
        {
            state = &net_transferStates[net_fileTransferHead];

            // Check and see if this is a directory, instead of a file
            if ( vfs_isDirectory( state->sourceName ) )
            {
                // Tell the target to create a directory
                log_info( "net_updateFileTranfers: Creating directory %s on target\n", state->destName );
                ego_packet_begin( &ego_pkt );
                ego_packet_addUint16( &ego_pkt, NETFILE_CREATE_DIRECTORY );
                ego_packet_addString( &ego_pkt, state->destName );
                net_sendPacketToPeerGuaranteed( &ego_pkt, state->target );

                net_waitingForXferAck = 1;
            }
            else
            {
                file = vfs_openReadB( state->sourceName );
                if ( file )
                {
                    log_info( "net_updateFileTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName );

                    fileSize = vfs_fileLength( file );
                    vfs_seek( file, 0 );

                    // Make room for the file's name
                    nameLen = strlen( state->destName ) + 1;
                    transferSize = nameLen;

                    // And for the file's size
                    transferSize += 6;  // Uint32 size, and Uint16 message type
                    transferSize += fileSize;

                    transferBuffer = EGOBOO_NEW_ARY( Uint8, transferSize );
                    *( Uint16* )transferBuffer = ENET_HOST_TO_NET_16( NETFILE_TRANSFER );

                    // Add the string and file length to the buffer
                    p = ( char * )( transferBuffer + 2 );
                    strcpy( p, state->destName );
                    p += nameLen;

                    networkSize = ENET_HOST_TO_NET_32(( Uint32 )fileSize );
                    *( size_t* )p = networkSize;
                    p += 4;

                    vfs_read( p, 1, fileSize, file );
                    vfs_close( file );

                    packet = enet_packet_create( transferBuffer, transferSize, ENET_PACKET_FLAG_RELIABLE );
                    enet_peer_send( state->target, NET_GUARANTEED_CHANNEL, packet );

                    EGOBOO_DELETE_ARY( transferBuffer );
                    transferSize = 0;

                    net_waitingForXferAck = 1;
                }
                else
                {
                    log_warning( "net_updateFileTransfers: Could not open file %s to send it!\n", state->sourceName );
                }
            }

            // update transfer queue state
            BLANK_STRUCT_PTR( state )
            net_fileTransferHead++;
            if ( net_fileTransferHead >= NET_MAX_FILE_TRANSFERS )
            {
                net_fileTransferHead = 0;
            }

        } // end if waiting for ack
    } // end if net_numFileTransfers > 0

    // Let the recieve loop run at least once
    net_listen_for_packets();

    ego_packet_dtor( &ego_pkt );
}

//--------------------------------------------------------------------------------------------
void net_send_message()
{
    /// @details ZZ@> sends the message in the keyboard buffer to all other players

    if ( keyb.chat_mode || !keyb.chat_done ) return;

    // if(gnet.on)
    // {
    //   start_building_packet();
    //   add_packet_us(TO_ANY_TEXT);
    //   add_packet_sz(net_chat.buffer);
    //   send_packet_to_all_players();
    // }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tlatch_ary_init( time_latch_t ary[], size_t len )
{
    size_t cnt;

    if ( NULL == ary || 0 == len ) return;

    for ( cnt = 0; cnt < len; cnt++ )
    {
        ary[cnt].x      = 0;
        ary[cnt].y      = 0;
        ary[cnt].button = 0;
        ary[cnt].time   = ( Uint32 )( ~0 );
    }
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
net_instance_t * net_instance_ctor( net_instance_t * pnet )
{
    if ( NULL == pnet ) return pnet;

    BLANK_STRUCT_PTR( pnet )

    pnet->local_machine  = 0;        // 0 is host, 1 is 1st remote, 2 is 2nd...

    return pnet;
}

//--------------------------------------------------------------------------------------------
bool_t net_on( const net_instance_t * pnet )
{
    if ( NULL == pnet ) return bfalse;

    return pnet->on;
}

//--------------------------------------------------------------------------------------------
bool_t net_serviceon( const net_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->on ) return bfalse;

    return pnet->serviceon;
}

//--------------------------------------------------------------------------------------------
bool_t net_get_hostactive( const net_instance_t * pnet )
{
    if ( NULL == pnet ) return bfalse;

    return pnet->hostactive;
}

//--------------------------------------------------------------------------------------------
bool_t net_set_hostactive( net_instance_t * pnet, bool_t val )
{
    if ( NULL == pnet ) return bfalse;

    pnet->hostactive = val;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_set_waitingforplayers( net_instance_t * pnet, bool_t val )
{
    if ( NULL == pnet ) return bfalse;

    pnet->waitingforplayers = val;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_set_myHost( net_instance_t * pnet, ENetHost* val )
{
    if ( NULL == pnet ) return bfalse;

    pnet->myHost = val;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_get_readytostart( const net_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->on ) return bfalse;

    return pnet->readytostart;
}

//--------------------------------------------------------------------------------------------
bool_t net_set_readytostart( net_instance_t * pnet, bool_t val )
{
    if ( NULL == pnet ) return bfalse;

    pnet->readytostart = val;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t net_waitingforplayers( const net_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->on ) return bfalse;

    return pnet->waitingforplayers;
}

//--------------------------------------------------------------------------------------------
int net_local_machine( const net_instance_t * pnet )
{
    if ( NULL == pnet ) return -1;

    return pnet->local_machine;
}

//--------------------------------------------------------------------------------------------
ENetHost* net_get_myHost( const net_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->on ) return NULL;

    return pnet->myHost;
}

//--------------------------------------------------------------------------------------------
int net_get_player_count( const net_instance_t * pnet )
{
    if ( NULL == pnet ) return 0;

    return ServerState.player_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void net_count_players( net_instance_t * pnet )
{
    PLA_REF ipla;

    if ( NULL == pnet ) return;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        if ( !PlaStack.lst[ipla].valid ) continue;

        // count the total number of players
        ServerState.player_count++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_packet_t * ego_packet_ctor( ego_packet_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_packet_t * ego_packet_dtor( ego_packet_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_begin( ego_packet_t * ptr )
{
    /// @details ZZ@> This function starts building a network packet

    if ( NULL == ptr ) return bfalse;

    ptr->head = 0;
    ptr->size = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addUint8( ego_packet_t * ptr, Uint8 uc )
{
    /// @details ZZ@> This function appends an Uint8 to the packet

    Uint8 * ucp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    ucp = ( Uint8* )( ptr->buffer + ptr->head );
    *ucp = uc;

    ptr->head += sizeof( uc );
    ptr->size += sizeof( uc );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addSint8( ego_packet_t * ptr, Sint8 sc )
{
    /// @details ZZ@> This function appends a Sint8 to the packet

    Sint8 * scp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    scp = ( Sint8* )( ptr->buffer + ptr->head );
    *scp = sc;

    ptr->head += sizeof( sc );
    ptr->size += sizeof( sc );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addUint16( ego_packet_t * ptr, Uint16 us )
{
    /// @details ZZ@> This function appends an Uint16 to the packet

    Uint16 * usp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    usp = ( Uint16* )( ptr->buffer + ptr->head );
    *usp = ENET_HOST_TO_NET_16( us );

    ptr->head += sizeof( us );
    ptr->size += sizeof( us );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addSint16( ego_packet_t * ptr, Sint16 ss )
{
    /// @details ZZ@> This function appends a Sint16 to the packet

    Sint16* ssp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    ssp = ( Sint16* )( ptr->buffer + ptr->head );
    *ssp = ENET_HOST_TO_NET_16( ss );

    ptr->head += sizeof( ss );
    ptr->size += sizeof( ss );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addUint32( ego_packet_t * ptr, Uint32 ui )
{
    /// @details ZZ@> This function appends an Uint32 to the packet

    Uint32* uip;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    uip = ( Uint32* )( ptr->buffer + ptr->head );
    *uip = ENET_HOST_TO_NET_32( ui );

    ptr->head += sizeof( ui );
    ptr->size += sizeof( ui );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addSint32( ego_packet_t * ptr, Sint32 si )
{
    /// @details ZZ@> This function appends a Sint32 to the packet

    Sint32 * sip;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    sip = ( Sint32 * )( ptr->buffer + ptr->head );
    *sip = ENET_HOST_TO_NET_32( si );

    ptr->head += sizeof( si );
    ptr->size += sizeof( si );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_packet_addString( ego_packet_t * ptr, const char *string )
{
    /// @details ZZ@> This function appends a null terminated string to the packet

    char * dst_ptr, * dst_end;
    const char * src_ptr;
    size_t buffer_size;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return bfalse;

    // how big is the packet buffer?
    buffer_size = SDL_arraysize( ptr->buffer );

    // set the source pointers
    src_ptr = string;

    // set the destination pointers
    dst_ptr = ( char* )( ptr->buffer + ptr->head );
    dst_end = ( char* )( ptr->buffer + buffer_size );

    // do not allow an overflow of the buffer
    while ( CSTR_END != *src_ptr && dst_ptr < dst_end && ptr->size < buffer_size )
    {
        *dst_ptr++ = *src_ptr++;

        ptr->head += sizeof( char );
        ptr->size += sizeof( char );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enet_packet_t * enet_packet_ctor( enet_packet_t * pkt )
{
    if ( NULL == pkt ) return NULL;

    BLANK_STRUCT_PTR( pkt );

    return pkt;
}

//--------------------------------------------------------------------------------------------
enet_packet_t * enet_packet_dtor( enet_packet_t * pkt )
{
    // WARNING, the way that the enet_packet_t encapsulates the data, it DOES NOT OWN the
    // ENetPacket pointer, so do not do anything to it

    if ( NULL == pkt ) return NULL;

    BLANK_STRUCT_PTR( pkt );

    return pkt;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_startReading( enet_packet_t * pkt, ENetPacket *packet )
{
    if ( NULL == pkt ) return bfalse;

    pkt->ptr = packet;
    pkt->read_location = 0;

    return ( NULL != pkt->ptr );
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_doneReading( enet_packet_t * pkt )
{
    if ( NULL == pkt ) return bfalse;

    pkt->ptr = NULL;
    pkt->read_location = 0;

    return ( NULL == pkt->ptr );
}

//--------------------------------------------------------------------------------------------
size_t enet_packet_remainingSize( enet_packet_t * pkt )
{
    /// @details ZZ@> This function tells if there's still data left in the packet

    if ( NULL == pkt ) return 0;

    return pkt->ptr->dataLength - pkt->read_location;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readString( enet_packet_t * pkt, char *buffer, size_t maxLen )
{
    /// @details ZZ@> This function reads a null terminated buffer from the packet

    size_t buffer_size;

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    // how big is the packet buffer?
    buffer_size = pkt->ptr->dataLength;

    if ( NULL == buffer || 0 == maxLen )
    {
        // just scan through the data for the end of the buffer

        const char * src_ptr, * src_end;

        // set the source pointers
        src_ptr = ( char* )( pkt->ptr->data + pkt->read_location );
        src_end = ( char* )( pkt->ptr->data + buffer_size );

        // skip to the end of the buffer
        while ( CSTR_END != *src_ptr && src_ptr < src_end )
        {
            pkt->read_location++;
        }
    }
    else
    {
        char * dst_ptr, * dst_end;
        const char * src_ptr, * src_end;

        // set the destination pointers
        dst_ptr = ( char* )( buffer );
        dst_end = ( char* )( buffer + maxLen );

        // set the source pointers
        src_ptr = ( char* )( pkt->ptr->data + pkt->read_location );
        src_end = ( char* )( pkt->ptr->data + buffer_size );

        while ( CSTR_END != *src_ptr && dst_ptr < dst_end && src_ptr < src_end )
        {
            *dst_ptr++ = *src_ptr++;
            pkt->read_location++;
        }

        // terminate the buffer if there is room
        if ( dst_ptr < dst_end )
        {
            *dst_ptr = CSTR_END;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readUint8( enet_packet_t * pkt, Uint8 * pval )
{
    /// @details ZZ@> This function reads an Uint8 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Uint8 * ucp = ( Uint8 * )( pkt->ptr->data + pkt->read_location );

        *pval = *ucp;
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readSint8( enet_packet_t * pkt, Sint8 * pval )
{
    /// @details ZZ@> This function reads a Sint8 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Sint8 * scp = ( Sint8 * )( pkt->ptr->data + pkt->read_location );

        *pval = *scp;
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readUint16( enet_packet_t * pkt, Uint16 * pval )
{
    /// @details ZZ@> This function reads an Uint16 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Uint16 * usp = ( Uint16* )( pkt->ptr->data + pkt->read_location );

        *pval = ENET_NET_TO_HOST_16( *usp );
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readSint16( enet_packet_t * pkt, Sint16 * pval )
{
    /// @details ZZ@> This function reads a Sint16 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Sint16 * ssp = ( Sint16* )( pkt->ptr->data + pkt->read_location );

        *pval = ENET_NET_TO_HOST_16( *ssp );
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readUint32( enet_packet_t * pkt, Uint32 * pval )
{
    /// @details ZZ@> This function reads an Uint32 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Uint32* uip = ( Uint32* )( pkt->ptr->data + pkt->read_location );
        *pval = ENET_NET_TO_HOST_32( *uip );
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t enet_packet_readSint32( enet_packet_t * pkt, Sint32 * pval )
{
    /// @details ZZ@> This function reads a Sint32 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return bfalse;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return bfalse;

    if ( NULL != pval )
    {
        Sint32* sip = ( Sint32* )( pkt->ptr->data + pkt->read_location );
        *pval = ENET_NET_TO_HOST_32( *sip );
    }

    pkt->read_location += sizeof( *pval );

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv netfile_handleEvent( net_instance_t * pnet, ENetEvent * event )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings
    int filesize, newfilesize, fileposition;
    char newfile;

    vfs_FILE *file;
    size_t fileSize;
    enet_packet_t enet_pkt;
    ego_packet_t  ego_pkt;
    bool_t handled;

    Uint8  ub;
    Uint16 us;
    Uint32 ui;

    enet_packet_ctor( &enet_pkt );
    ego_packet_ctor( &ego_pkt );

    if ( !gnet.on || NULL == event )
    {
        return rv_fail;
    }

    // assume the best
    handled = btrue;

    // log the packet
    log_info( "netfile_handleEvent: Received " );

    // initialize the packet container
    if ( !enet_packet_startReading( &enet_pkt, event->packet ) )
    {
        return rv_fail;
    }

    // begin cracking
    enet_packet_readUint16( &enet_pkt, &header );

    switch ( header )
    {

        case NETFILE_TRANSFER:
            enet_packet_readString( &enet_pkt, filename, sizeof( filename ) );
            enet_packet_readUint32( &enet_pkt, &fileSize );

            log_info( "NETFILE_TRANSFER: %s with size %d.\n", filename, fileSize );

            // Try and save the file
            file = vfs_openWriteB( filename );
            if ( NULL != file )
            {
                vfs_write( &enet_pkt.ptr->data + enet_pkt.read_location, 1, fileSize, file );
                vfs_close( file );
            }
            else
            {
                log_warning( "net_dispatchEvent: Couldn't write new file!\n" );
            }

            // Acknowledge that we got this file
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TRANSFER_OK );
            net_sendPacketToPeer( &ego_pkt, event->peer );

            // And note that we've gotten another one
            numfile++;
            break;

        case NETFILE_TRANSFER_OK:
            log_info( "NETFILE_TRANSFER_OK. The last file sent was successful.\n" );
            net_waitingForXferAck = 0;
            net_numFileTransfers--;

            break;

        case NETFILE_CREATE_DIRECTORY:
            enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
            log_info( "NETFILE_CREATE_DIRECTORY: %s\n", filename );

            vfs_mkdir( filename );

            // Acknowledge that we got this file
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TRANSFER_OK );
            net_sendPacketToPeer( &ego_pkt, event->peer );

            numfile++;  // The client considers directories it sends to be files, so ya.
            break;

        case NETFILE_DONE_SENDING:
            log_info( "NETFILE_DONE_SENDING\n" );
            numplayerrespond++;
            break;

        case NETFILE_NUM_TO_SEND:
            log_info( "NETFILE_NUM_TO_SEND\n" );
            if ( enet_packet_readUint16( &enet_pkt, &us ) )
            {
                numfileexpected = ( int ) us;
            }
            else
            {
                numfileexpected = 0;
            }
            break;

        case NETFILE_TO_HOST_FILE:
            log_info( "NETFILE_TO_HOST_FILE\n" );
            enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
            enet_packet_readUint32( &enet_pkt, &ui );
            newfilesize = ( int )ui;

            // Change the size of the file if need be
            newfile = 0;
            file = vfs_openReadB( filename );
            if ( file )
            {
                filesize = vfs_fileLength( file );
                vfs_close( file );
                if ( filesize != newfilesize )
                {
                    // Destroy the old file
                    newfile = 1;
                }
            }
            else
            {
                newfile = 1;
            }
            if ( newfile )
            {
                // file must be created.  Write zeroes to the file to do it
                numfile++;
                file = vfs_openWriteB( filename );
                if ( file )
                {
                    filesize = 0;

                    while ( filesize < newfilesize )
                    {
                        vfs_putc( 0, file );
                        filesize++;
                    }

                    vfs_close( file );
                }
            }

            // Go to the position in the file and copy data
            enet_packet_readUint32( &enet_pkt, &ui );
            fileposition = ( int ) ui;

            file = vfs_openReadB( filename );
            if ( file )
            {
                if ( 0 == vfs_seek( file, fileposition ) )
                {
                    while ( enet_packet_readUint8( &enet_pkt, &ub ) )
                    {
                        vfs_putc( ub, file );
                    }
                }

                vfs_close( file );
            }
            break;

        case NETFILE_TO_HOST_DIR:
            log_info( "NETFILE_TO_HOST_DIR\n" );
            if ( pnet->hostactive )
            {
                if ( enet_packet_readString( &enet_pkt, filename, sizeof( filename ) ) )
                {
                    vfs_mkdir( filename );
                }
            }
            break;

        case NETFILE_TO_HOST_SENT:
            log_info( "NETFILE_TO_HOST_SENT\n" );
            if ( pnet->hostactive )
            {
                if ( enet_packet_readUint32( &enet_pkt, &ui ) )
                {
                    numfileexpected += ui;
                    numplayerrespond++;
                }
            }
            break;

        case NETFILE_TO_REMOTE_SENT:
            log_info( "NETFILE_TO_REMOTE_SENT\n" );
            if ( !pnet->hostactive )
            {
                if ( enet_packet_readUint32( &enet_pkt, &ui ) )
                {
                    numfileexpected += ui;
                    numplayerrespond++;
                }
            }
            break;

        case NETFILE_TO_REMOTE_FILE:
            log_info( "NETFILE_TO_REMOTE_FILE\n" );
            if ( !pnet->hostactive )
            {
                enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
                enet_packet_readUint32( &enet_pkt, &ui );
                newfilesize = ( int )ui;

                // Change the size of the file if need be
                newfile = 0;
                file = vfs_openReadB( filename );
                if ( file )
                {
                    filesize = vfs_fileLength( file );
                    vfs_close( file );
                    if ( filesize != newfilesize )
                    {
                        // Destroy the old file
                        newfile = 1;
                    }
                }
                else
                {
                    newfile = 1;
                }
                if ( newfile )
                {
                    // file must be created.  Write zeroes to the file to do it
                    numfile++;
                    file = vfs_openWriteB( filename );
                    if ( file )
                    {
                        filesize = 0;

                        while ( filesize < newfilesize )
                        {
                            vfs_putc( 0, file );
                            filesize++;
                        }

                        vfs_close( file );
                    }
                }

                // Go to the position in the file and copy data
                enet_packet_readUint32( &enet_pkt, &ui );
                fileposition = ( int )ui;

                file = vfs_openReadB( filename );
                if ( file )
                {
                    if ( 0 == vfs_seek( file, fileposition ) )
                    {
                        while ( enet_packet_readUint8( &enet_pkt, &ub ) )
                        {
                            vfs_putc( ub, file );
                        }
                    }

                    vfs_close( file );
                }
            }
            break;

        case NETFILE_TO_REMOTE_DIR:
            log_info( "NETFILE_TO_REMOTE_DIR\n" );
            if ( !pnet->hostactive )
            {
                enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
                vfs_mkdir( filename );
            }
            break;

        default:
            handled = bfalse;
            break;
    }

    // deconstruct the packet(s)
    enet_packet_dtor( &enet_pkt );
    ego_packet_dtor( &ego_pkt );

    return handled ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv net_handlePacket( net_instance_t * pnet, enet_packet_t * enet_pkt )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings

    ego_packet_t  ego_pkt;
    bool_t handled;

    ego_packet_ctor( &ego_pkt );

    if ( !net_on( pnet ) || NULL == enet_pkt ) return rv_error;

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

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

//bool_t net_sendPacketToHost( ego_packet_t * ptr )
//{
//    /// @details ZZ@> This function sends a packet to the host
//
//    int enet_retval;
//    ENetPacket * packet;
//
//    // valid data?
//    if( NULL == ptr || 0 == ptr->size ) return bfalse;
//
//    packet = enet_packet_create( ptr->buffer, ptr->size, 0 );
//
//    enet_retval = -1;
//    if( NULL != packet )
//    {
//        enet_retval = enet_peer_send( ClientState.gameHost, NET_UNRELIABLE_CHANNEL, packet );
//    }
//
//    return (0 == enet_retval);
//}

//--------------------------------------------------------------------------------------------
//bool_t net_sendPacketToHostGuaranteed( ego_packet_t * ptr )
//{
//    /// @details ZZ@> This function sends a packet to the host
//
//    ENetPacket *packet;
//    int enet_retval;
//
//    // valid data?
//    if( NULL == ptr || 0 == ptr->size ) return bfalse;
//
//    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );
//
//    enet_retval = -1;
//    if( NULL != packet )
//    {
//        enet_peer_send( ClientState.gameHost, NET_UNRELIABLE_CHANNEL, packet );
//    }
//
//    return (0 == enet_retval);
//}
