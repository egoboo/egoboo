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

/// @file egolib/network.c
/// @brief Egoboo networking implementation
/// @details Shuttles bits across the network, using Enet.  Networked play doesn't
/// really work at the moment.

#include <stdarg.h>

#include "egolib/network.h"

#include "egolib/network_file.h"
#include "egolib/file_common.h"
#include "egolib/log.h"
#include "egolib/strutil.h"
#include "egolib/vfs.h"
#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"

#include "egolib/file_formats/module_file.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of the network code used in old-egoboo
struct s_egonet_instance
{
    C_BOOLEAN  initialized;             ///< the singleton has been initialized
    C_BOOLEAN  req_enet;                  ///< Try to connect?
    C_BOOLEAN  enet_on;                 ///< enet is active
    C_BOOLEAN  service_on;               ///< Do I need to free the interface?
    C_BOOLEAN  hostactive;              ///< Hosting?
    C_BOOLEAN  readytostart;            ///< Ready to hit the Start Game button?
    C_BOOLEAN  waitingforclients;       ///< Has everyone talked to the host?

    int     service;                                  ///< Which service is being used?
    int     service_count;                            ///< How many we found
    char    service_name[MAXSERVICE][NETNAMESIZE];    ///< Names of services

    int     session_count;                            ///< How many we found
    char    session_name[MAXSESSION][NETNAMESIZE];    ///< Names of sessions

    int     local_machine;                            ///< 0 is host, 1 is 1st remote, 2 is 2nd...

    ENetHost          * myHost;
    BaseServerState_t * pserver;
    BaseClientState_t * pclient;
};

#define EGONET_INSTANCE_INIT \
    { \
        C_FALSE,  /* C_BOOLEAN initialized   */ \
        C_FALSE,  /* C_BOOLEAN  req_enet       */ \
        C_FALSE,  /* C_BOOLEAN  enet_on      */ \
        C_FALSE,  /* C_BOOLEAN  service_on    */ \
        C_FALSE,  /* C_BOOLEAN  hostactive   */ \
        C_FALSE,  /* C_BOOLEAN  readytostart */ \
        C_FALSE,  /* C_BOOLEAN  waitingforclients */ \
        -1,      /* int     service */ \
        0,       /* int     service_count */ \
        { '\0' }, /* char    service_name[MAXSERVICE][NETNAMESIZE] */ \
        -1,      /* int     session_count */ \
        { '\0' }, /* char    session_name[MAXSESSION][NETNAMESIZE] */ \
        -1,      /* int     local_machine */ \
        NULL,    /* ENetHost          * myHost */ \
        NULL,    /* BaseServerState_t * pserver */ \
        NULL     /* BaseClientState_t * pclient */ \
    }

egonet_instance_t * egonet_instance_ctor( egonet_instance_t * pnet );
egonet_instance_t * egonet_instance_dtor( egonet_instance_t * pnet );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void egonet_connect( void );
static void egonet_disconnect( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static egonet_instance_t gnet = EGONET_INSTANCE_INIT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

FILE * globalnetworkerr = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

egonet_instance_t * egonet_get_instance( void )
{
    if ( !gnet.initialized ) return NULL;

    return &gnet;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_on( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    return gnet.enet_on;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_get_serviceon( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_get_serviceon( &gnet );
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN     egonet_get_hostactive( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_get_hostactive( &gnet );
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN     egonet_get_readytostart( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_get_readytostart( &gnet );
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN     egonet_get_waitingforclients( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_get_waitingforclients( &gnet );
}

//--------------------------------------------------------------------------------------------
const int        egonet_get_local_machine( void )
{
    if ( !gnet.initialized ) return -1;

    return egonet_instance_get_local_machine( &gnet );
}

//--------------------------------------------------------------------------------------------
const int        egonet_get_client_count( void )
{
    if ( !gnet.initialized ) return 0;

    return egonet_instance_get_client_count( &gnet );
}

//--------------------------------------------------------------------------------------------
const ENetHost * egonet_get_myHost( void )
{
    if ( !gnet.initialized ) return NULL;

    return egonet_instance_get_myHost( &gnet );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_set_hostactive( const C_BOOLEAN val )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_set_hostactive( &gnet, val );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_set_waitingforclients( const C_BOOLEAN val )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_set_waitingforclients( &gnet, val );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_set_readytostart( const C_BOOLEAN val )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_set_readytostart( &gnet, val );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_set_myHost( const ENetHost* phost )
{
    if ( !gnet.initialized ) return C_FALSE;

    return egonet_instance_set_myHost( &gnet, phost );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void egonet_connect( void )
{
    if ( !gnet.initialized ) return;

    if ( !gnet.req_enet )
    {
        // We're not doing networking this time...
        log_info( "egonet_connect: Networking not enabled.\n" );
    }
    else if ( !gnet.enet_on )
    {
        // initialize enet
        log_info( "egonet_connect: Initializing enet... " );
        if ( 0 != enet_initialize() )
        {
            log_info( "Failure!\n" );
            gnet.enet_on       = C_FALSE;
            gnet.service_on    = C_FALSE;
            gnet.service_count = 0;
        }
        else
        {
            log_info( "Success!\n" );
            gnet.enet_on       = C_TRUE;
            gnet.service_on    = C_TRUE;
            gnet.service_count = 1;
        }
    }
}

//--------------------------------------------------------------------------------------------
void egonet_disconnect( void )
{
    if ( !gnet.initialized ) return;

    if ( !gnet.req_enet )
    {
        // We're not doing networking this time...
        log_info( "egonet_disconnect: Networking not enabled.\n" );
    }
    else if ( gnet.enet_on )
    {
        log_info( "egonet_disconnect: deinitializing enet... " );

        enet_deinitialize();

        gnet.enet_on = C_FALSE;
        gnet.service_on    = C_FALSE;
        gnet.service_count = 0;

        log_info( "Finished!\n" );
    }
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_begin( C_BOOLEAN req_enet )
{
    if ( !gnet.initialized ) return C_FALSE;

    // initialize the file transfer code
    netfile_initialize();

    gnet.req_enet = req_enet;

    egonet_connect();

    return req_enet == gnet.enet_on;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_end( void )
{
    if ( !gnet.initialized ) return C_FALSE;

    egonet_disconnect();

    gnet.req_enet    = C_FALSE;

    // clear the file transfer code
    netfile_initialize();

    return !gnet.enet_on;
}

//--------------------------------------------------------------------------------------------
void egonet_initialize( BaseServerState_t * ps, BaseClientState_t * pc, size_t size, C_BOOLEAN req_enet )
{
    /// @author ZZ
    /// @details This starts up the network and logs whatever goes on

    if ( gnet.initialized ) return;

    // register the shutdown command
    atexit( egonet_shutDown );

    // initialize the "singleton"
    egonet_instance_ctor( &gnet );

    // initialize all state variables
    gnet.pclient = BaseClientState_ctor( pc, size );
    gnet.pserver = BaseServerState_ctor( ps, size );

    gnet.initialized = C_TRUE;

    egonet_begin( req_enet );
}

//--------------------------------------------------------------------------------------------
void egonet_shutDown( void )
{
    if ( !gnet.initialized ) return;

    log_info( "egonet_shutDown: Turning off networking.\n" );

    // kill enet
    if ( gnet.enet_on )
    {
        enet_deinitialize();
        gnet.enet_on = C_FALSE;
    }

    // dealloc all allocated data
    BaseClientState_dtor( gnet.pclient );
    BaseServerState_dtor( gnet.pserver );

    // reinitialize the network state
    egonet_instance_dtor( &gnet );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void egonet_close_session( void )
{
    size_t i, numPeers;
    ENetEvent event;

    /// @author ZZ
    /// @details This function gets the computer out of a network game

    if ( !gnet.initialized ) return;

    if ( gnet.enet_on )
    {
        if ( gnet.pserver->am_host )
        {
            // Disconnect the peers
            numPeers = gnet.myHost->peerCount;

            for ( i = 0; i < numPeers; i++ )
            {
#if defined(ENET11)
                enet_peer_disconnect( &gnet.myHost->peers[i], 0 );
#else
                enet_peer_disconnect( &gnet.myHost->peers[i], 0 );
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
                        log_info( "egonet_close_session: Peer id %d disconnected gracefully.\n", event.peer->address.host );
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

        log_info( "egonet_close_session: Disconnecting from network.\n" );

        enet_host_destroy( gnet.myHost );
        gnet.myHost = NULL;

        gnet.pclient->gameHost = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void egonet_listen_for_packets( void )
{
    /// @author ZZ
    /// @details This function reads any new messages and sets the player latch and matrix needed
    ///    lists...

    ENetEvent event;

    if ( gnet.enet_on )
    {
        // Listen for new messages
        while ( 0 != enet_host_service( gnet.myHost, &event, 0 ) )
        {
            switch ( event.type )
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    if ( !egonet_dispatchEvent( &event ) )
                    {
                        log_warning( " %s(%d) - packet not handled\n", __FUNCTION__, __LINE__ );
                    }
                    enet_packet_destroy( event.packet );
                    break;

                case ENET_EVENT_TYPE_CONNECT:
                    // don't allow anyone to connect during the game session
                    log_warning( "egonet_listen_for_packets: Client tried to connect during the game: %x:%u\n",
                                 event.peer->address.host, event.peer->address.port );
#if defined(ENET11)
                    enet_peer_disconnect( event.peer, 0 );
#else
                    enet_peer_disconnect( event.peer, 0 );
#endif
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:

                    // Is this a player disconnecting, or just a rejected connection
                    // from above?
                    if ( event.peer->data != 0 )
                    {
                        BaseConnectionInfo_t *info = ( BaseConnectionInfo_t * )event.peer->data;

                        // uh oh, how do we handle losing a player?
                        log_warning( "egonet_listen_for_packets: Player %d disconnected!\n",
                                     info->connection_slot );
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
// time_latch_t
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
// ego_packet_t
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
C_BOOLEAN ego_packet_begin( ego_packet_t * ptr )
{
    /// @author ZZ
    /// @details This function starts building a network packet

    if ( NULL == ptr ) return C_FALSE;

    ptr->head = 0;
    ptr->size = 0;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addUint8( ego_packet_t * ptr, Uint8 uc )
{
    /// @author ZZ
    /// @details This function appends an Uint8 to the packet

    Uint8 * ucp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    ucp = ( Uint8* )( ptr->buffer + ptr->head );
    *ucp = uc;

    ptr->head += sizeof( uc );
    ptr->size += sizeof( uc );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addSint8( ego_packet_t * ptr, Sint8 sc )
{
    /// @author ZZ
    /// @details This function appends a Sint8 to the packet

    Sint8 * scp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    scp = ( Sint8* )( ptr->buffer + ptr->head );
    *scp = sc;

    ptr->head += sizeof( sc );
    ptr->size += sizeof( sc );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addUint16( ego_packet_t * ptr, Uint16 us )
{
    /// @author ZZ
    /// @details This function appends an Uint16 to the packet

    Uint16 * usp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    usp = ( Uint16* )( ptr->buffer + ptr->head );
    *usp = ENET_HOST_TO_NET_16( us );

    ptr->head += sizeof( us );
    ptr->size += sizeof( us );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addSint16( ego_packet_t * ptr, Sint16 ss )
{
    /// @author ZZ
    /// @details This function appends a Sint16 to the packet

    Sint16* ssp;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    ssp = ( Sint16* )( ptr->buffer + ptr->head );
    *ssp = ENET_HOST_TO_NET_16( ss );

    ptr->head += sizeof( ss );
    ptr->size += sizeof( ss );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addUint32( ego_packet_t * ptr, Uint32 ui )
{
    /// @author ZZ
    /// @details This function appends an Uint32 to the packet

    Uint32* uip;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    uip = ( Uint32* )( ptr->buffer + ptr->head );
    *uip = ENET_HOST_TO_NET_32( ui );

    ptr->head += sizeof( ui );
    ptr->size += sizeof( ui );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addSint32( ego_packet_t * ptr, Sint32 si )
{
    /// @author ZZ
    /// @details This function appends a Sint32 to the packet

    Sint32 * sip;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

    sip = ( Sint32 * )( ptr->buffer + ptr->head );
    *sip = ENET_HOST_TO_NET_32( si );

    ptr->head += sizeof( si );
    ptr->size += sizeof( si );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN ego_packet_addString( ego_packet_t * ptr, const char *string )
{
    /// @author ZZ
    /// @details This function appends a null terminated string to the packet

    char * dst_ptr, * dst_end;
    const char * src_ptr;
    size_t buffer_size;

    if ( NULL == ptr || ptr->size >= sizeof( ptr->buffer ) ) return C_FALSE;

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

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_broadcastPacket( ego_packet_t * ptr )
{
    /// @author ZZ
    /// @details This function sends a packet to all the players

    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return C_FALSE;

    packet = enet_packet_create( ptr->buffer, ptr->size, 0 );

    if ( NULL != packet )
    {
        enet_host_broadcast( gnet.myHost, NET_UNRELIABLE_CHANNEL, packet );
    }

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_broadcastPacketGuaranteed( ego_packet_t * ptr )
{
    /// @author ZZ
    /// @details This function sends a packet to all the players

    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return C_FALSE;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    if ( NULL != packet )
    {
        enet_host_broadcast( gnet.myHost, NET_GUARANTEED_CHANNEL, packet );
    }

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_sendPacketToOneClientGuaranteed( ego_packet_t * ptr, int player )
{
    /// @author ZZ
    /// @details This function sends a packet to one of the players

    ENetPacket *packet;
    int enet_retval;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return C_FALSE;

    // valid player?
    if ( player >= gnet.pserver->client_count ) return C_FALSE;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( &gnet.myHost->peers[player], NET_GUARANTEED_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_sendPacketToPeer( ego_packet_t * ptr, ENetPeer *peer )
{
    /// @author JF
    /// @details This function sends a packet to a given peer

    int enet_retval;
    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return C_FALSE;

    packet = enet_packet_create( ptr->buffer, ptr->size, ENET_PACKET_FLAG_RELIABLE );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( peer, NET_UNRELIABLE_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_sendPacketToPeerGuaranteed( ego_packet_t * ptr, ENetPeer *peer )
{
    /// @author JF
    /// @details This funciton sends a packet to a given peer, with guaranteed delivery

    int enet_retval;
    ENetPacket *packet;

    // valid data?
    if ( NULL == ptr || 0 == ptr->size ) return C_FALSE;

    packet = enet_packet_create( ptr->buffer, ptr->size, 0 );

    enet_retval = -1;
    if ( NULL != packet )
    {
        enet_retval = enet_peer_send( peer, NET_GUARANTEED_CHANNEL, packet );
    }

    return ( 0 == enet_retval );
}

//--------------------------------------------------------------------------------------------
// enet_packet_t
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
C_BOOLEAN enet_packet_startReading( enet_packet_t * pkt, ENetPacket *packet )
{
    if ( NULL == pkt ) return C_FALSE;

    pkt->ptr = packet;
    pkt->read_location = 0;

    return ( NULL != pkt->ptr );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_doneReading( enet_packet_t * pkt )
{
    if ( NULL == pkt ) return C_FALSE;

    pkt->ptr = NULL;
    pkt->read_location = 0;

    return ( NULL == pkt->ptr );
}

//--------------------------------------------------------------------------------------------
size_t enet_packet_remainingSize( enet_packet_t * pkt )
{
    /// @author ZZ
    /// @details This function tells if there's still data left in the packet

    if ( NULL == pkt ) return 0;

    return pkt->ptr->dataLength - pkt->read_location;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readString( enet_packet_t * pkt, char *buffer, size_t maxLen )
{
    /// @author ZZ
    /// @details This function reads a null terminated buffer from the packet

    size_t buffer_size;

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

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

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readUint8( enet_packet_t * pkt, Uint8 * pval )
{
    /// @author ZZ
    /// @details This function reads an Uint8 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Uint8 * ucp = ( Uint8 * )( pkt->ptr->data + pkt->read_location );

        *pval = *ucp;
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readSint8( enet_packet_t * pkt, Sint8 * pval )
{
    /// @author ZZ
    /// @details This function reads a Sint8 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Sint8 * scp = ( Sint8 * )( pkt->ptr->data + pkt->read_location );

        *pval = *scp;
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readUint16( enet_packet_t * pkt, Uint16 * pval )
{
    /// @author ZZ
    /// @details This function reads an Uint16 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Uint16 * usp = ( Uint16* )( pkt->ptr->data + pkt->read_location );

        *pval = ENET_NET_TO_HOST_16( *usp );
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readSint16( enet_packet_t * pkt, Sint16 * pval )
{
    /// @author ZZ
    /// @details This function reads a Sint16 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Sint16 * ssp = ( Sint16* )( pkt->ptr->data + pkt->read_location );

        *pval = ENET_NET_TO_HOST_16( *ssp );
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readUint32( enet_packet_t * pkt, Uint32 * pval )
{
    /// @author ZZ
    /// @details This function reads an Uint32 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Uint32* uip = ( Uint32* )( pkt->ptr->data + pkt->read_location );
        *pval = ENET_NET_TO_HOST_32( *uip );
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN enet_packet_readSint32( enet_packet_t * pkt, Sint32 * pval )
{
    /// @author ZZ
    /// @details This function reads a Sint32 from the packet

    // valid packet?
    if ( NULL == pkt || NULL == pkt->ptr ) return C_FALSE;

    // read past the end?
    if ( pkt->read_location >= pkt->ptr->dataLength ) return C_FALSE;

    if ( NULL != pval )
    {
        Sint32* sip = ( Sint32* )( pkt->ptr->data + pkt->read_location );
        *pval = ENET_NET_TO_HOST_32( *sip );
    }

    pkt->read_location += sizeof( *pval );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BaseServerState_t * BaseServerState_ctor( BaseServerState_t * ptr, size_t size )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    if ( 0 != size )
    {
        ptr->client_name = EGOBOO_NEW_ARY( STRING, size );

        if ( NULL != ptr->client_name )
        {
            ptr->client_info = EGOBOO_NEW_ARY( BaseConnectionInfo_t, size );

            if ( NULL == ptr->client_info )
            {
                EGOBOO_DELETE_ARY( ptr->client_info );
            }
            else
            {
                for (size_t cnt = 0; cnt < size; cnt++ )
                {
                    BaseConnectionInfo_ctor( ptr->client_info + cnt );
                }

                ptr->client_size = size;
            }
        }
    }

    ptr->last_frame = ( Uint32 )( ~0 );

    return ptr;
}

//--------------------------------------------------------------------------------------------
BaseServerState_t * BaseServerState_dtor( BaseServerState_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    if ( NULL != ptr->client_name && NULL != ptr->client_info && 0 != ptr->client_size )
    {
        EGOBOO_DELETE_ARY( ptr->client_name );
        EGOBOO_DELETE_ARY( ptr->client_info );
        ptr->client_size = 0;
    }

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
int BaseServerState_count_connections( BaseServerState_t * ptr )
{
    if ( NULL == ptr ) return 0;

    // count the valid connections
    ptr->client_count = 0;
    if ( NULL != ptr->client_info )
    {
        for (size_t cnt = 0; cnt < ptr->client_size; cnt++ )
        {
            if ( -1 != ptr->client_info[cnt].connection_slot )
            {
                ptr->client_count++;
            }
        }
    }

    return ptr->client_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BaseClientState_t * BaseClientState_ctor( BaseClientState_t * ptr, size_t peers )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    // allocate the peers
    if ( 0 != peers )
    {
        ptr->peer_ary = EGOBOO_NEW_ARY( ENetPeer, peers );

        if ( NULL != ptr->peer_ary )
        {
            ptr->peer_cnt = peers;
        }
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
BaseClientState_t * BaseClientState_dtor( BaseClientState_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    if ( NULL != ptr->peer_ary || 0 != ptr->peer_cnt )
    {
        EGOBOO_DELETE_ARY( ptr->peer_ary );
        ptr->peer_cnt = 0;
    }

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BaseConnectionInfo_t * BaseConnectionInfo_ctor( BaseConnectionInfo_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    ptr->connection_slot = -1;

    return ptr;
}

//--------------------------------------------------------------------------------------------
BaseConnectionInfo_t * BaseConnectionInfo_dtor( BaseConnectionInfo_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
// egonet_instance_t
//--------------------------------------------------------------------------------------------
egonet_instance_t * egonet_instance_ctor( egonet_instance_t * pnet )
{
    if ( NULL == pnet ) return pnet;

    BLANK_STRUCT_PTR( pnet )

    gnet.local_machine  = 0;        // 0 is host, 1 is 1st remote, 2 is 2nd...

    return pnet;
}

//--------------------------------------------------------------------------------------------
egonet_instance_t * egonet_instance_dtor( egonet_instance_t * pnet )
{
    if ( NULL == pnet ) return pnet;

    BLANK_STRUCT_PTR( pnet )

    return pnet;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_instance_get_serviceon( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    return pnet->service_on;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_instance_get_hostactive( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    return pnet->hostactive;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_instance_get_readytostart( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    return pnet->readytostart;
}

//--------------------------------------------------------------------------------------------
const C_BOOLEAN egonet_instance_get_waitingforclients( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    return pnet->waitingforclients;
}

//--------------------------------------------------------------------------------------------
const int egonet_instance_get_local_machine( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return -1;

    return pnet->local_machine;
}

//--------------------------------------------------------------------------------------------
const ENetHost* egonet_instance_get_myHost( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized ) return NULL;

    return pnet->myHost;
}

//--------------------------------------------------------------------------------------------
const int egonet_instance_get_client_count( const egonet_instance_t * pnet )
{
    if ( NULL == pnet || !pnet->initialized || NULL == pnet->pserver ) return 0;

    return pnet->pserver->client_count;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_instance_set_hostactive( egonet_instance_t * pnet, const C_BOOLEAN val )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    pnet->hostactive = val;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_instance_set_waitingforclients( egonet_instance_t * pnet, const C_BOOLEAN val )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    pnet->waitingforclients = val;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_instance_set_myHost( egonet_instance_t * pnet, const ENetHost* val )
{
    if ( NULL == pnet || !pnet->initialized ) return C_FALSE;

    pnet->myHost = ( ENetHost* )val;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN egonet_instance_set_readytostart( egonet_instance_t * pnet, const C_BOOLEAN val )
{
    if ( NULL == pnet ) return C_FALSE;

    pnet->readytostart = val;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------
//void egonet_turn_on_service( int service )
//{
//    /// @author ZZ
//    /// @details This function turns on a network service ( IPX, TCP, serial, modem )
//    /// @note BB@> this function has no purpose with Enet, which only communicates over TCP
//}
