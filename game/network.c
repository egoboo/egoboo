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

#include "file_common.h"
#include "log.h"
#include "input.h"
#include "char.inl"
#include "module_file.h"
#include "game.h"
#include "menu.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo.h"

#include "enet/enet.h"
#include "file_common.h"

#include <stdarg.h>
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int  numfile = 0;                                // For network copy
static int  numfilesent = 0;                            // For network copy
static int  numfileexpected = 0;                        // For network copy
static int  numplayerrespond = 0;

static net_instance_t gnet = { bfalse, bfalse, bfalse, bfalse, bfalse };

static bool_t net_instance_init( net_instance_t * pnet );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, player_t, PlaStack, MAX_PLAYER );

int         lag  = 3;                       // Lag tolerance
Uint32      numplatimes = 0;

int         local_numlpla;                         // number of players on the local machine

FILE *      globalnetworkerr = NULL;

int     networkservice;
int     numservice  = 0;
char    netservicename[MAXSERVICE][NETNAMESIZE];
int     numsession  = 0;
char    netsessionname[MAXSESSION][NETNAMESIZE];
int     numplayer  = 0;
char    netplayername[MAXNETPLAYER][NETNAMESIZE];

int     local_machine  = 0;        // 0 is host, 1 is 1st remote, 2 is 2nd...

int     playersready  = 0;         // Number of players ready to start
int     playersloaded = 0;

Uint32 sv_last_frame = ( Uint32 )~0;

net_instance_t * PNet = &gnet;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Networking constants
enum NetworkConstant
{
    NET_UNRELIABLE_CHANNEL    = 0,
    NET_GUARANTEED_CHANNEL    = 1,
    NET_EGOBOO_NUM_CHANNELS,
    NET_EGOBOO_PORT           = 34626,
    NET_MAX_FILE_NAME         = 128,
    NET_MAX_FILE_TRANSFERS    = 1024  // Maximum files queued up at once
};

/// All the supported network messages
enum NetworkMessage
{
    NET_TRANSFER_FILE       = 10001,  // Packet contains a file.
    NET_TRANSFER_OK         = 10002,  // Acknowledgement packet for a file send
    NET_CREATE_DIRECTORY    = 10003,  // Tell the peer to create the named directory
    NET_DONE_SENDING_FILES  = 10009,  // Sent when there are no more files to send.
    NET_NUM_FILES_TO_SEND   = 10010  // Let the other person know how many files you're sending
};

/// Network information on connected players
typedef struct NetPlayerInfo
{
    int playerSlot;
} NetPlayerInfo;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint32 nexttimestamp;                          // Expected timestamp

// ENet host & client identifiers
static ENetHost* net_myHost = NULL;
static ENetPeer* net_gameHost = NULL;
static ENetPeer* net_playerPeers[MAX_PLAYER];
static NetPlayerInfo net_playerInfo[MAXNETPLAYER];

static bool_t net_amHost = bfalse;

// Packet reading
static ENetPacket*    net_readPacket = NULL;
static size_t         net_readLocation = 0;

// Packet writing
static Uint32  packethead;                             // The write head
static Uint32  packetsize;                             // The size of the packet
static Uint8   packetbuffer[MAXSENDSIZE];              // The data packet

/// Data for network file transfers
typedef struct NetFileTransfer
{
    char sourceName[NET_MAX_FILE_NAME];
    char destName[NET_MAX_FILE_NAME];
    ENetPeer *target;
} NetFileTransfer;

/// Network file transfer queue
static NetFileTransfer net_transferStates[NET_MAX_FILE_TRANSFERS];
static int net_numFileTransfers = 0;  ///< Queue count
static int net_fileTransferHead = 0;  ///< Queue start indicx
static int net_fileTransferTail = 0;  ///< Queue end index
static int net_waitingForXferAck = 0; ///< Queue state

static Uint8  * transferBuffer = NULL;
static size_t   transferSize = 0;

// Receiving files
static NetFileTransfer net_receiveState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void close_session()
{
    size_t i, numPeers;
    ENetEvent event;

    /// @details ZZ@> This function gets the computer out of a network game

    if ( gnet.on )
    {
        if ( net_amHost )
        {
            // Disconnect the peers
            numPeers = net_myHost->peerCount;

            for ( i = 0; i < numPeers; i++ )
            {
#if defined(ENET11)
                enet_peer_disconnect( &net_myHost->peers[i], 0 );
#else
                enet_peer_disconnect( &net_myHost->peers[i] );
#endif
            }

            // Allow up to 5 seconds for peers to drop
            while ( enet_host_service( net_myHost, &event, 5000 ) )
            {
                switch ( event.type )
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy( event.packet );
                        break;

                    case ENET_EVENT_TYPE_DISCONNECT:
                        log_info( "close_session: Peer id %d disconnected gracefully.\n", event.peer->address.host );
                        numPeers--;
                        break;

                    default:
                        break;
                }
            }

            // Forcefully disconnect any peers leftover
            for ( i = 0; i < net_myHost->peerCount; i++ )
            {
                enet_peer_reset( &net_myHost->peers[i] );
            }
        }

        log_info( "close_session: Disconnecting from network.\n" );
        enet_host_destroy( net_myHost );
        net_myHost = NULL;
        net_gameHost = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void net_startNewPacket()
{
    /// @details ZZ@> This function starts building a network packet

    packethead = 0;
    packetsize = 0;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedByte( Uint8 uc )
{
    /// @details ZZ@> This function appends an Uint8 to the packet

    Uint8* ucp;
    ucp = ( Uint8* )( &packetbuffer[packethead] );
    *ucp = uc;
    packethead += 1;
    packetsize += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedByte( Sint8 sc )
{
    /// @details ZZ@> This function appends a Sint8 to the packet

    signed char* scp;
    scp = ( signed char* )( &packetbuffer[packethead] );
    *scp = sc;
    packethead += 1;
    packetsize += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedShort( Uint16 us )
{
    /// @details ZZ@> This function appends an Uint16 to the packet

    Uint16* usp;
    usp = ( Uint16* )( &packetbuffer[packethead] );

    *usp = ENET_HOST_TO_NET_16( us );
    packethead += 2;
    packetsize += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedShort( Sint16 ss )
{
    /// @details ZZ@> This function appends a Sint16 to the packet

    signed short* ssp;
    ssp = ( signed short* )( &packetbuffer[packethead] );

    *ssp = ENET_HOST_TO_NET_16( ss );

    packethead += 2;
    packetsize += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedInt( Uint32 ui )
{
    /// @details ZZ@> This function appends an Uint32 to the packet

    Uint32* uip;
    uip = ( Uint32* )( &packetbuffer[packethead] );

    *uip = ENET_HOST_TO_NET_32( ui );

    packethead += 4;
    packetsize += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedInt( Sint32 si )
{
    /// @details ZZ@> This function appends a Sint32 to the packet

    signed int* sip;
    sip = ( signed int* )( &packetbuffer[packethead] );

    *sip = ENET_HOST_TO_NET_32( si );

    packethead += 4;
    packetsize += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addString( const char *string )
{
    /// @details ZZ@> This function appends a null terminated string to the packet

    char* cp;
    char cTmp;
    int cnt;

    cnt = 0;
    cTmp = 1;
    cp = ( char* )( &packetbuffer[packethead] );

    while ( CSTR_END != cTmp )
    {
        cTmp = string[cnt];
        *cp = cTmp;
        cp += 1;
        packethead += 1;
        packetsize += 1;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void packet_startReading( ENetPacket *packet )
{
    net_readPacket = packet;
    net_readLocation = 0;
}

//--------------------------------------------------------------------------------------------
void packet_doneReading()
{
    net_readPacket = NULL;
    net_readLocation = 0;
}

//--------------------------------------------------------------------------------------------
void packet_readString( char *buffer, int maxLen )
{
    /// @details ZZ@> This function reads a null terminated string from the packet

    Uint8 uc;
    Uint16 outindex;

    outindex = 0;
    uc = net_readPacket->data[net_readLocation];
    net_readLocation++;

    while ( uc != 0 && outindex < maxLen )
    {
        buffer[outindex] = uc;
        outindex++;
        uc = net_readPacket->data[net_readLocation];
        net_readLocation++;
    }

    buffer[outindex] = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 packet_readUnsignedByte()
{
    /// @details ZZ@> This function reads an Uint8 from the packet

    Uint8 uc;
    uc = ( Uint8 )net_readPacket->data[net_readLocation];
    net_readLocation++;
    return uc;
}

//--------------------------------------------------------------------------------------------
Sint8 packet_readSignedByte()
{
    /// @details ZZ@> This function reads a Sint8 from the packet

    Sint8 sc;
    sc = ( signed char )net_readPacket->data[net_readLocation];
    net_readLocation++;
    return sc;
}

//--------------------------------------------------------------------------------------------
Uint16 packet_readUnsignedShort()
{
    /// @details ZZ@> This function reads an Uint16 from the packet

    Uint16 us;
    Uint16* usp;
    usp = ( Uint16* )( &net_readPacket->data[net_readLocation] );

    us = ENET_NET_TO_HOST_16( *usp );

    net_readLocation += 2;
    return us;
}

//--------------------------------------------------------------------------------------------
Sint16 packet_readSignedShort()
{
    /// @details ZZ@> This function reads a Sint16 from the packet

    Sint16 ss;
    signed short* ssp;
    ssp = ( signed short* )( &net_readPacket->data[net_readLocation] );

    ss = ENET_NET_TO_HOST_16( *ssp );

    net_readLocation += 2;
    return ss;
}

//--------------------------------------------------------------------------------------------
Uint32 packet_readUnsignedInt()
{
    /// @details ZZ@> This function reads an Uint32 from the packet

    Uint32 ui;
    Uint32* uip;
    uip = ( Uint32* )( &net_readPacket->data[net_readLocation] );

    ui = ENET_NET_TO_HOST_32( *uip );

    net_readLocation += 4;
    return ui;
}

//--------------------------------------------------------------------------------------------
Sint32 packet_readSignedInt()
{
    /// @details ZZ@> This function reads a Sint32 from the packet

    Sint32 si;
    signed int* sip;
    sip = ( signed int* )( &net_readPacket->data[net_readLocation] );

    si = ENET_NET_TO_HOST_32( *sip );

    net_readLocation += 4;
    return si;
}

//--------------------------------------------------------------------------------------------
size_t packet_remainingSize()
{
    /// @details ZZ@> This function tells if there's still data left in the packet

    return net_readPacket->dataLength - net_readLocation;
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHost()
{
    /// @details ZZ@> This function sends a packet to the host

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, 0 );
    enet_peer_send( net_gameHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayers()
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, 0 );
    enet_host_broadcast( net_myHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHostGuaranteed()
{
    /// @details ZZ@> This function sends a packet to the host

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE );
    enet_peer_send( net_gameHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayersGuaranteed()
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE );
    enet_host_broadcast( net_myHost, NET_GUARANTEED_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToOnePlayerGuaranteed( int player )
{
    /// @details ZZ@> This function sends a packet to one of the players

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE );
    if ( player < numplayer )
    {
        enet_peer_send( &net_myHost->peers[player], NET_GUARANTEED_CHANNEL, packet );
    }
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeer( ENetPeer *peer )
{
    /// @details JF@> This function sends a packet to a given peer

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE );
    enet_peer_send( peer, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeerGuaranteed( ENetPeer *peer )
{
    /// @details JF@> This funciton sends a packet to a given peer, with guaranteed delivery

    ENetPacket *packet = enet_packet_create( packetbuffer, packetsize, 0 );
    enet_peer_send( peer, NET_GUARANTEED_CHANNEL, packet );
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
        state->target = &net_myHost->peers[0];
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
    int packetsize, packetstart;
    int filesize;
    int fileisdir;
    char cTmp;

    log_info( "net_copyFileToAllPlayers: %s, %s\n", source, dest );
    if ( gnet.on && gnet.hostactive )
    {
        fileisdir = vfs_isDirectory( source );
        if ( fileisdir )
        {
            net_startNewPacket();
            packet_addUnsignedShort( TO_REMOTE_DIR );
            packet_addString( dest );
            net_sendPacketToAllPlayersGuaranteed();
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
                    packetsize = 0;
                    packetstart = 0;
                    numfilesent++;

                    net_startNewPacket();
                    packet_addUnsignedShort( TO_REMOTE_FILE );
                    packet_addString( dest );
                    packet_addUnsignedInt( filesize );
                    packet_addUnsignedInt( packetstart );

                    while ( packetstart < filesize )
                    {
                        // This will probably work...
                        // vfs_read((packetbuffer + packethead), COPYSIZE, 1, fileread);

                        // But I'll leave it alone for now
                        vfs_scanf( fileread, "%c", &cTmp );

                        packet_addUnsignedByte( cTmp );
                        packetsize++;
                        packetstart++;
                        if ( packetsize >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToAllPlayersGuaranteed();
                            enet_host_flush( net_myHost );

                            // Start on the next 4K
                            packetsize = 0;
                            net_startNewPacket();
                            packet_addUnsignedShort( TO_REMOTE_FILE );
                            packet_addString( dest );
                            packet_addUnsignedInt( filesize );
                            packet_addUnsignedInt( packetstart );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToAllPlayersGuaranteed();
                }

                vfs_close( fileread );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToHost( const char *source, const char *dest )
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

        state->target = net_gameHost;
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
            log_warning( "net_copyFileToHost: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToHostOld_vfs( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the remote to the host computer.
    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
    ///    that can be sent is 2 Megs ( TOTALSIZE ).

    vfs_FILE* fileread;
    int packetsize, packetstart;
    int filesize;
    int fileisdir;
    char cTmp;

    log_info( "net_copyFileToHost: " );
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
            net_startNewPacket();
            packet_addUnsignedShort( TO_HOST_DIR );
            packet_addString( dest );
//     net_sendPacketToAllPlayersGuaranteed();
            net_sendPacketToHost();
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
                    packetsize = 0;
                    packetstart = 0;
                    net_startNewPacket();
                    packet_addUnsignedShort( TO_HOST_FILE );
                    packet_addString( dest );
                    packet_addUnsignedInt( filesize );
                    packet_addUnsignedInt( packetstart );

                    while ( packetstart < filesize )
                    {
                        vfs_scanf( fileread, "%c", &cTmp );
                        packet_addUnsignedByte( cTmp );
                        packetsize++;
                        packetstart++;
                        if ( packetsize >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToHostGuaranteed();
                            enet_host_flush( net_myHost );

                            // Start on the next 4K
                            packetsize = 0;
                            net_startNewPacket();
                            packet_addUnsignedShort( TO_HOST_FILE );
                            packet_addString( dest );
                            packet_addUnsignedInt( filesize );
                            packet_addUnsignedInt( packetstart );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToHostGuaranteed();
                }

                vfs_close( fileread );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToHost( const char *dirname, const char *todirname )
{
    /// @details ZZ@> This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "net_copyDirectoryToHost: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        net_copyFileToHost( dirname, todirname );

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

            net_copyFileToHost( fromname, toname );

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

    if ( !gnet.on )
    {
        gnet.waitingforplayers = bfalse;
    }
    else if ( gnet.hostactive )
    {
        log_info( "net_sayHello: Server saying hello.\n" );
        playersloaded++;
        if ( playersloaded >= numplayer )
        {
            gnet.waitingforplayers = bfalse;
        }
    }
    else
    {
        log_info( "net_sayHello: Client saying hello.\n" );
        net_startNewPacket();
        packet_addUnsignedShort( TO_HOST_IM_LOADED );
        net_sendPacketToHostGuaranteed();
    }
}

//--------------------------------------------------------------------------------------------
void cl_talkToHost()
{
    /// @details ZZ@> This function sends the latch packets to the host machine

    PLA_REF player;

    // Let the players respawn
    if ( SDLKEYDOWN( SDLK_SPACE )
         && ( local_stats.allpladead || PMod->respawnanytime )
         && PMod->respawnvalid
         && cfg.difficulty < GAME_HARD
         && !console_mode )
    {
        player = 0;

        while ( player < MAX_PLAYER )
        {
            if ( PlaStack.lst[player].valid && PlaStack.lst[player].device.bits != INPUT_BITS_NONE )
            {
                SET_BIT( PlaStack.lst[player].local_latch.b, LATCHBUTTON_RESPAWN );  // Press the respawn button...
            }

            player++;
        }
    }

    // Start talkin'
    if ( gnet.on && !gnet.hostactive /*&& !PMod->rtscontrol*/ )
    {
        net_startNewPacket();
        packet_addUnsignedShort( TO_HOST_LATCH );        // The message header

        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            // Find the local players
            if ( PlaStack.lst[player].valid && PlaStack.lst[player].device.bits != INPUT_BITS_NONE )
            {
                packet_addUnsignedByte( REF_TO_INT( player ) );                      // The player index
                packet_addUnsignedInt( PlaStack.lst[player].local_latch.b );             // Player button states
                packet_addSignedShort( PlaStack.lst[player].local_latch.x*SHORTLATCH );  // Player motion
                packet_addSignedShort( PlaStack.lst[player].local_latch.y*SHORTLATCH );  // Player motion
            }
        }

        // Send it to the host
        net_sendPacketToHost();
    }
}

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes()
{
    /// @details ZZ@> This function sends the character data to all the remote machines

    PLA_REF player;
    int time;

    // make sure there is only one update per frame;
    if ( update_wld == sv_last_frame ) return;
    sv_last_frame = update_wld;

    if ( gnet.hostactive )
    {
        if ( gnet.on )
        {
            time = true_update + lag;

            // Send a message to all players
            net_startNewPacket();
            packet_addUnsignedShort( TO_REMOTE_LATCH );                       // The message header
            packet_addUnsignedInt( time );                                  // The stamp

            // Send all player latches...
            for ( player = 0; player < MAX_PLAYER; player++ )
            {
                if ( !PlaStack.lst[player].valid ) continue;

                packet_addUnsignedByte( REF_TO_INT( player ) );                      // The player index
                packet_addUnsignedInt( PlaStack.lst[player].local_latch.b );        // Player button states
                packet_addSignedShort( PlaStack.lst[player].local_latch.x*SHORTLATCH );  // Player motion
                packet_addSignedShort( PlaStack.lst[player].local_latch.y*SHORTLATCH );  // Player motion

                player++;
            }

            // Send the packet
            net_sendPacketToAllPlayers();
        }
        else
        {
            time = true_update + 1;
        }

        // update the local timed latches with the same info
        numplatimes = 0;
        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            int index;
            Uint32 cnt;
            player_t * ppla;

            if ( !PlaStack.lst[player].valid ) continue;
            ppla = PlaStack.lst + player;

            index = ppla->tlatch_count;
            if ( index < MAXLAG )
            {
                time_latch_t * ptlatch = ppla->tlatch + index;

                ptlatch->button = ppla->local_latch.b;

                // reduce the resolution of the motion to match the network packets
                ptlatch->x = FLOOR( ppla->local_latch.x * SHORTLATCH ) / SHORTLATCH;
                ptlatch->y = FLOOR( ppla->local_latch.y * SHORTLATCH ) / SHORTLATCH;

                ptlatch->time = true_update;

                ppla->tlatch_count++;
            }

            // determine the max amount of lag
            for ( cnt = 0; cnt < ppla->tlatch_count; cnt++ )
            {
                int loc_lag = update_wld - ppla->tlatch[index].time  + 1;

                if ( loc_lag > numplatimes )
                {
                    numplatimes = loc_lag;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void pla_add_tlatch( const PLA_REF iplayer, Uint32 time, latch_t net_latch )
{
    player_t * ppla;

    if ( !VALID_PLA( iplayer ) ) return;
    ppla = PlaStack.lst + iplayer;

    if ( ppla->tlatch_count >= MAXLAG ) return;

    ppla->tlatch[ ppla->tlatch_count ].button = net_latch.b;
    ppla->tlatch[ ppla->tlatch_count ].x      = net_latch.x;
    ppla->tlatch[ ppla->tlatch_count ].y      = net_latch.y;
    ppla->tlatch[ ppla->tlatch_count ].time   = time;

    ppla->tlatch_count++;
}

//--------------------------------------------------------------------------------------------
void net_handlePacket( ENetEvent *event )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings
    int filesize, newfilesize, fileposition;
    char newfile;
    PLA_REF player;
    Uint32 stamp;
    int time;
    vfs_FILE *file;
    size_t fileSize;

    log_info( "net_handlePacket: Received " );

    packet_startReading( event->packet );
    header = packet_readUnsignedShort();

    switch ( header )
    {
        case TO_ANY_TEXT:
            log_info( "TO_ANY_TEXT\n" );
            packet_readString( filename, 255 );
            debug_printf( filename );
            break;

        case TO_HOST_MODULEOK:
            log_info( "TO_HOSTMODULEOK\n" );
            if ( gnet.hostactive )
            {
                playersready++;
                if ( playersready >= numplayer )
                {
                    gnet.readytostart = btrue;
                }
            }
            break;

        case TO_HOST_LATCH:
            log_info( "TO_HOST_LATCH\n" );
            if ( gnet.hostactive )
            {
                while ( packet_remainingSize() > 0 )
                {
                    latch_t tmp_latch;

                    player = packet_readUnsignedByte();
                    time   = packet_readUnsignedInt();

                    tmp_latch.b = packet_readUnsignedInt();
                    tmp_latch.x = packet_readSignedShort() / SHORTLATCH;
                    tmp_latch.y = packet_readSignedShort() / SHORTLATCH;

                    pla_add_tlatch( player, time, tmp_latch );
                }

            }
            break;

        case TO_HOST_IM_LOADED:
            log_info( "TO_HOST_IMLOADED\n" );
            if ( gnet.hostactive )
            {
                playersloaded++;
                if ( playersloaded == numplayer )
                {
                    // Let the games begin...
                    gnet.waitingforplayers = bfalse;
                    net_startNewPacket();
                    packet_addUnsignedShort( TO_REMOTE_START );
                    net_sendPacketToAllPlayersGuaranteed();
                }
            }
            break;

        case TO_HOST_RTS:
            log_info( "TO_HOST_RTS\n" );
            if ( gnet.hostactive )
            {
                /*whichorder = get_empty_order();
                if(whichorder < MAXORDER)
                  {
                  // Add the order on the host machine
                  cnt = 0;
                  while(cnt < MAXSELECT)
                  {
                    who = packet_readUnsignedByte();
                    orderwho[whichorder][cnt] = who;
                    cnt++;
                  }
                  what = packet_readUnsignedInt();
                  when = update_wld + orderlag;
                  orderwhat[whichorder] = what;
                  orderwhen[whichorder] = when;

                  // Send the order off to everyone else
                  net_startNewPacket();
                  packet_addUnsignedShort(TO_REMOTE_RTS);
                  cnt = 0;
                  while(cnt < MAXSELECT)
                  {
                    packet_addUnsignedByte(orderwho[whichorder][cnt]);
                    cnt++;
                  }
                  packet_addUnsignedInt(what);
                  packet_addUnsignedInt(when);
                  net_sendPacketToAllPlayersGuaranteed();
                  }*/
            }
            break;

        case NET_TRANSFER_FILE:
            packet_readString( filename, 256 );
            fileSize = packet_readUnsignedInt();

            log_info( "NET_TRANSFER_FILE: %s with size %d.\n", filename, fileSize );

            // Try and save the file
            file = vfs_openWriteB( filename );
            if ( NULL != file )
            {
                vfs_write( net_readPacket->data + net_readLocation, 1, fileSize, file );
                vfs_close( file );
            }
            else
            {
                log_warning( "net_handlePacket: Couldn't write new file!\n" );
            }

            // Acknowledge that we got this file
            net_startNewPacket();
            packet_addUnsignedShort( NET_TRANSFER_OK );
            net_sendPacketToPeer( event->peer );

            // And note that we've gotten another one
            numfile++;
            break;

        case NET_TRANSFER_OK:
            log_info( "NET_TRANSFER_OK. The last file sent was successful.\n" );
            net_waitingForXferAck = 0;
            net_numFileTransfers--;

            break;

        case NET_CREATE_DIRECTORY:
            packet_readString( filename, 256 );
            log_info( "NET_CREATE_DIRECTORY: %s\n", filename );

            vfs_mkdir( filename );

            // Acknowledge that we got this file
            net_startNewPacket();
            packet_addUnsignedShort( NET_TRANSFER_OK );
            net_sendPacketToPeer( event->peer );

            numfile++;  // The client considers directories it sends to be files, so ya.
            break;

        case NET_DONE_SENDING_FILES:
            log_info( "NET_DONE_SENDING_FILES\n" );
            numplayerrespond++;
            break;

        case NET_NUM_FILES_TO_SEND:
            log_info( "NET_NUM_FILES_TO_SEND\n" );
            numfileexpected = ( int )packet_readUnsignedShort();
            break;

        case TO_HOST_FILE:
            log_info( "TO_HOST_FILE\n" );
            packet_readString( filename, 255 );
            newfilesize = packet_readUnsignedInt();

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
            fileposition = packet_readUnsignedInt();
            file = vfs_openReadB( filename );
            if ( file )
            {
                if ( 0 == vfs_seek( file, fileposition ) )
                {
                    while ( packet_remainingSize() > 0 )
                    {
                        vfs_putc( packet_readUnsignedByte(), file );
                    }
                }

                vfs_close( file );
            }
            break;

        case TO_HOST_DIR:
            log_info( "TO_HOST_DIR\n" );
            if ( gnet.hostactive )
            {
                packet_readString( filename, 255 );
                vfs_mkdir( filename );
            }
            break;

        case TO_HOST_FILESENT:
            log_info( "TO_HOST_FILESENT\n" );
            if ( gnet.hostactive )
            {
                numfileexpected += packet_readUnsignedInt();
                numplayerrespond++;
            }
            break;

        case TO_REMOTE_FILESENT:
            log_info( "TO_REMOTE_FILESENT\n" );
            if ( !gnet.hostactive )
            {
                numfileexpected += packet_readUnsignedInt();
                numplayerrespond++;
            }
            break;

        case TO_REMOTE_MODULE:
            log_info( "TO_REMOTE_MODULE\n" );
            if ( !gnet.hostactive && !gnet.readytostart )
            {
                PMod->seed = packet_readUnsignedInt();
                packet_readString( filename, 255 );

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
                    gnet.readytostart = btrue;

                    // Tell the host we're ready
                    net_startNewPacket();
                    packet_addUnsignedShort( TO_HOST_MODULEOK );
                    net_sendPacketToHostGuaranteed();
                }
                else
                {
                    // The module doesn't exist locally
                    pickedmodule_ready = bfalse;

                    // Halt the process
                    gnet.readytostart = bfalse;

                    // Tell the host we're not ready
                    net_startNewPacket();
                    packet_addUnsignedShort( TO_HOST_MODULEBAD );
                    net_sendPacketToHostGuaranteed();
                }
            }
            break;

        case TO_REMOTE_START:
            log_info( "TO_REMOTE_START\n" );
            if ( !gnet.hostactive )
            {
                gnet.waitingforplayers = bfalse;
            }
            break;

        case TO_REMOTE_RTS:
            log_info( "TO_REMOTE_RTS\n" );
            if ( !gnet.hostactive )
            {
                /*    whichorder = get_empty_order();
                    if(whichorder < MAXORDER)
                    {
                      // Add the order on the remote machine
                      cnt = 0;
                      while(cnt < MAXSELECT)
                      {
                        who = packet_readUnsignedByte();
                        orderwho[whichorder][cnt] = who;
                        cnt++;
                      }
                      what = packet_readUnsignedInt();
                      when = packet_readUnsignedInt();
                      orderwhat[whichorder] = what;
                      orderwhen[whichorder] = when;
                    }*/
            }
            break;

        case TO_REMOTE_FILE:
            log_info( "TO_REMOTE_FILE\n" );
            if ( !gnet.hostactive )
            {
                packet_readString( filename, 255 );
                newfilesize = packet_readUnsignedInt();

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
                fileposition = packet_readUnsignedInt();
                file = vfs_openReadB( filename );
                if ( file )
                {
                    if ( 0 == vfs_seek( file, fileposition ) )
                    {
                        while ( packet_remainingSize() > 0 )
                        {
                            vfs_putc( packet_readUnsignedByte(), file );
                        }
                    }

                    vfs_close( file );
                }
            }
            break;

        case TO_REMOTE_DIR:
            log_info( "TO_REMOTE_DIR\n" );
            if ( !gnet.hostactive )
            {
                packet_readString( filename, 255 );
                vfs_mkdir( filename );
            }
            break;

        case TO_REMOTE_LATCH:
            log_info( "TO_REMOTE_LATCH\n" );
            if ( !gnet.hostactive )
            {
                stamp = packet_readUnsignedInt();
                time = stamp & LAGAND;
                if ((( Uint32 )( ~0 ) ) == nexttimestamp )
                {
                    nexttimestamp = stamp;
                }
                if ( stamp < nexttimestamp )
                {
                    log_warning( "net_handlePacket: OUT OF ORDER PACKET\n" );
                    outofsync = btrue;
                }
                if ( stamp <= update_wld )
                {
                    log_warning( "net_handlePacket: LATE PACKET\n" );
                    outofsync = btrue;
                }
                if ( stamp > nexttimestamp )
                {
                    log_warning( "net_handlePacket: MISSED PACKET\n" );
                    nexttimestamp = stamp;  // Still use it
                    outofsync = btrue;
                }
                if ( stamp == nexttimestamp )
                {
                    // Remember that we got it
                    numplatimes++;

                    // Read latches for each player sent
                    while ( packet_remainingSize() > 0 )
                    {
                        player = packet_readUnsignedByte();
                        PlaStack.lst[player].tlatch[time].button = packet_readUnsignedInt();
                        PlaStack.lst[player].tlatch[time].x      = packet_readSignedShort() / SHORTLATCH;
                        PlaStack.lst[player].tlatch[time].y      = packet_readSignedShort() / SHORTLATCH;
                    }

                    nexttimestamp = stamp + 1;
                }
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------
void listen_for_packets()
{
    /// @details ZZ@> This function reads any new messages and sets the player latch and matrix needed
    ///    lists...

    ENetEvent event;
    if ( gnet.on )
    {
        // Listen for new messages
        while ( enet_host_service( net_myHost, &event, 0 ) != 0 )
        {
            switch ( event.type )
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    net_handlePacket( &event );
                    enet_packet_destroy( event.packet );
                    break;

                case ENET_EVENT_TYPE_CONNECT:
                    // don't allow anyone to connect during the game session
                    log_warning( "listen_for_packets: Client tried to connect during the game: %x:%u\n",
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
                        NetPlayerInfo *info = ( NetPlayerInfo * )event.peer->data;

                        // uh oh, how do we handle losing a player?
                        log_warning( "listen_for_packets: Player %d disconnected!\n",
                                     info->playerSlot );
                    }
                    break;

                default:
                    break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void unbuffer_player_latches()
{
    /// @details ZZ@> This function sets character latches based on player input to the host

    PLA_REF ipla;
    CHR_REF character;

    // if ( PMod->rtscontrol ) { numplatimes--; return; }

    // get the "network" latch for each valid player
    numplatimes = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        Uint32 latch_count, tnc;
        latch_t tmp_latch;
        player_t * ppla;
        time_latch_t * tlatch_list;

        if ( !PlaStack.lst[ipla].valid ) continue;
        ppla = PlaStack.lst + ipla;
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
        ppla = PlaStack.lst + ipla;

        character = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        pchr->latch = ppla->net_latch;
    }

    // Let players respawn
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        chr_t * pchr;
        player_t * ppla;

        if ( !PlaStack.lst[ipla].valid ) continue;
        ppla = PlaStack.lst + ipla;

        character = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

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

    gnet.serviceon = bfalse;
    numsession = 0;
    numservice = 0;

    net_instance_init( &gnet );

    // Clear all the state variables to 0 to start.
    memset( net_playerPeers, 0, sizeof( ENetPeer* ) * MAX_PLAYER );
    memset( net_playerInfo, 0, sizeof( NetPlayerInfo ) * MAX_PLAYER );
    memset( packetbuffer, 0, MAXSENDSIZE * sizeof( Uint8 ) );
    memset( net_transferStates, 0, sizeof( NetFileTransfer ) * NET_MAX_FILE_TRANSFERS );
    memset( &net_receiveState, 0, sizeof( NetFileTransfer ) );

    sv_last_frame = ( Uint32 )~0;

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
            numservice = 1;
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
    numsession = 0;
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "  Looking for open games...\n");
    ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = NETWORKID;
    hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "    %d sessions found\n", numsession);
      }
    */
}

//--------------------------------------------------------------------------------------------
void sv_letPlayersJoin()
{
    /// @details ZZ@> This function finds all the players in the game

    ENetEvent event;
    STRING hostName;

    // Check all pending events for players joining
    while ( enet_host_service( net_myHost, &event, 0 ) > 0 )
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
                strncpy( netplayername[numplayer], hostName, 16 );

                event.peer->data = &( net_playerInfo[numplayer] );
                numplayer++;

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
}

//--------------------------------------------------------------------------------------------
int cl_joinGame( const char* hostname )
{
    /// @details ZZ@> This function tries to join one of the sessions we found

    ENetAddress address;
    ENetEvent event;
    if ( gnet.on )
    {
        log_info( "cl_joinGame: Creating client network connection... " );
        // Create my host thingamabober
        /// @todo Should I limit client bandwidth here?
        net_myHost = enet_host_create( NULL, 1, 0, 0 );
        if ( NULL == net_myHost )
        {
            // can't create a network connection at all
            log_info( "Failure!\n" );
            return bfalse;
        }
        else
        {
            log_info( "Success!\n" );
        };

        // Now connect to the remote host
        log_info( "cl_joinGame: Attempting to connect to %s:%d\n", hostname, NET_EGOBOO_PORT );
        enet_address_set_host( &address, hostname );
        address.port = NET_EGOBOO_PORT;
        net_gameHost = enet_host_connect( net_myHost, &address, NET_EGOBOO_NUM_CHANNELS );
        if ( NULL == net_gameHost )
        {
            log_info( "cl_joinGame: No available peers to create a connection!\n" );
            return bfalse;
        }

        // Wait for up to 5 seconds for the connection attempt to succeed
        if ( enet_host_service( net_myHost, &event, 5000 ) > 0 &&
             event.type == ENET_EVENT_TYPE_CONNECT )
        {
            log_info( "cl_joinGame: Connected to %s:%d\n", hostname, NET_EGOBOO_PORT );
            return btrue;
            // return create_player(bfalse);
        }
        else
        {
            log_info( "cl_joinGame: Could not connect to %s:%d!\n", hostname, NET_EGOBOO_PORT );
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void stop_players_from_joining()
{
    /// @details ZZ@> This function stops players from joining a game
}

//--------------------------------------------------------------------------------------------
int sv_hostGame()
{
    /// @details ZZ@> This function tries to host a new session

    ENetAddress address;
    if ( gnet.on )
    {
        // Try to create a new session
        address.host = ENET_HOST_ANY;
        address.port = NET_EGOBOO_PORT;

        log_info( "sv_hostGame: Creating game on port %d\n", NET_EGOBOO_PORT );
        net_myHost = enet_host_create( &address, MAX_PLAYER, 0, 0 );
        if ( NULL == net_myHost )
        {
            log_info( "sv_hostGame: Could not create network connection!\n" );
            return bfalse;
        }

        // Try to create a host player
//   return create_player(btrue);
        net_amHost = btrue;

        // Moved from net_sayHello because there they cause a race issue
        gnet.waitingforplayers = btrue;
        playersloaded = 0;
    }

    // Run in solo mode
    return btrue;
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
                net_startNewPacket();
                packet_addUnsignedShort( NET_CREATE_DIRECTORY );
                packet_addString( state->destName );
                net_sendPacketToPeerGuaranteed( state->target );

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
                    *( Uint16* )transferBuffer = ENET_HOST_TO_NET_16( NET_TRANSFER_FILE );

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
            memset( state, 0, sizeof( *state ) );
            net_fileTransferHead++;
            if ( net_fileTransferHead >= NET_MAX_FILE_TRANSFERS )
            {
                net_fileTransferHead = 0;
            }

        } // end if waiting for ack
    } // end if net_numFileTransfers > 0

    // Let the recieve loop run at least once
    listen_for_packets();
}

//--------------------------------------------------------------------------------------------
void net_send_message()
{
    /// @details ZZ@> sends the message in the keyboard buffer to all other players

    if ( console_mode || !console_done ) return;

    // if(gnet.on)
    // {
    //   start_building_packet();
    //   add_packet_us(TO_ANY_TEXT);
    //   add_packet_sz(keyb.buffer);
    //   send_packet_to_all_players();
    // }
}

//--------------------------------------------------------------------------------------------
bool_t net_instance_init( net_instance_t * pnet )
{
    if ( NULL == pnet ) return bfalse;

    memset( pnet, 0, sizeof( *pnet ) );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF pla_get_ichr( const PLA_REF iplayer )
{
    player_t * pplayer;

    if ( iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid ) return ( CHR_REF )MAX_CHR;
    pplayer = PlaStack.lst + iplayer;

    if ( !INGAME_CHR( pplayer->index ) ) return ( CHR_REF )MAX_CHR;

    return pplayer->index;
}

//--------------------------------------------------------------------------------------------
chr_t  * pla_get_pchr( const PLA_REF iplayer )
{
    player_t * pplayer;

    if ( iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid ) return NULL;
    pplayer = PlaStack.lst + iplayer;

    if ( !INGAME_CHR( pplayer->index ) ) return NULL;

    return ChrList.lst + pplayer->index;
}

//--------------------------------------------------------------------------------------------
void net_reset_players()
{
    PLA_REF cnt;

    // Reset the initial player data and latches
    for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        memset( PlaStack.lst + cnt, 0, sizeof( player_t ) );

        // reset the device
        input_device_init( &( PlaStack.lst[cnt].device ) );
    }
    PlaStack.count        = 0;

    nexttimestamp = (( Uint32 )~0 );
    numplatimes   = 0;
}

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
void pla_reinit( player_t * ppla )
{
    if ( NULL == ppla ) return;

    ppla->valid       = bfalse;
    ppla->index       = ( CHR_REF )MAX_CHR;
    ppla->device.bits = INPUT_BITS_NONE;
}

//--------------------------------------------------------------------------------------------
void player_init( player_t * ppla )
{
    if ( NULL == ppla ) return;

    memset( ppla, 0, sizeof( *ppla ) );

    ppla->index       = ( CHR_REF )MAX_CHR;

    // initialize the device
    input_device_init( &( ppla->device ) );

    // initialize the latches
    latch_init( &( ppla->local_latch ) );
    latch_init( &( ppla->net_latch ) );

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );
}

//--------------------------------------------------------------------------------------------
// Sustain old movements to ease mouse play
void input_device_add_latch( input_device_t * pdevice, float newx, float newy )
{
    float dist;

    if ( NULL == pdevice ) return;

    pdevice->latch_old = pdevice->latch;

    pdevice->latch.x = pdevice->latch.x * pdevice->sustain + newx * pdevice->cover;
    pdevice->latch.y = pdevice->latch.y * pdevice->sustain + newy * pdevice->cover;

    // make sure that the latch never overflows
    dist = pdevice->latch.x * pdevice->latch.x + pdevice->latch.y * pdevice->latch.y;
    if ( dist > 1.0f )
    {
        float scale = 1.0f / SQRT( dist );

        pdevice->latch.x *= scale;
        pdevice->latch.y *= scale;
    }
}

//--------------------------------------------------------------------------------------------
player_t* chr_get_ppla( const CHR_REF ichr )
{
    PLA_REF iplayer;

    if ( !INGAME_CHR( ichr ) ) return NULL;
    iplayer = ChrList.lst[ichr].is_which_player;

    if ( !VALID_PLA( iplayer ) ) return NULL;

    return PlaStack.lst + iplayer;
}