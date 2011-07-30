#pragma once

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

/// @file network.h
/// @brief Skeleton for Egoboo networking

#include "../egolib/typedef.h"

#include <enet/enet.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// an opaque struct for encapsulating network data
    struct s_egonet_instance;
    typedef struct s_egonet_instance egonet_instance_t;

    struct s_time_latch;
    typedef struct s_time_latch time_latch_t;

/// opaque wrapper for the enet packet data
    struct s_ego_packet;
    typedef struct s_ego_packet ego_packet_t;

/// opaque wrapper for the enet packet data
    struct s_enet_packet;
    typedef struct s_enet_packet enet_packet_t;

    struct s_BaseConnectionInfo;
    typedef struct s_BaseConnectionInfo BaseConnectionInfo_t;

    struct s_BaseServerState;
    typedef struct s_BaseServerState BaseServerState_t;

    struct s_BaseClientState;
    typedef struct s_BaseClientState BaseClientState_t;

//--------------------------------------------------------------------------------------------
// Network constants
//--------------------------------------------------------------------------------------------

#define MAX_LOCAL_PLAYERS    4
#define MAX_PLAYER           MAX_LOCAL_PLAYERS      ///< ZF> used to be 8, but caused some memset issues if MAX_PLAYER > MAX_LOCAL_PLAYERS

#define NETREFRESH          1000                    ///< Every second
#define NONETWORK           service_count
#define MAXSERVICE          16
#define NETNAMESIZE         16
#define MAXSESSION          16
#define MAXLAG              (1 << 6)
#define LAGAND              ( MAXLAG - 1 )

#define SHORTLATCH 1024.0f
#define MAXSENDSIZE 0x2000

/// packet headers
    enum e_netfile_commands
    {
        NETFILE_TO_HOST_FILE        = 0x5002,
        NETFILE_TO_HOST_DIR         = 0xC04E,
        NETFILE_TO_HOST_SENT    = 0x334B,

        NETFILE_TO_REMOTE_FILE      = 0xF2F6,
        NETFILE_TO_REMOTE_DIR       = 0x2B1A,
        NETFILE_TO_REMOTE_SENT  = 0x4DBF
    };

/// Networking constants
    enum NetworkConstant
    {
        NET_UNRELIABLE_CHANNEL    = 0,
        NET_GUARANTEED_CHANNEL    = 1,
        NET_EGOBOO_NUM_CHANNELS,
        NET_MAX_FILE_NAME         = 128,
        NET_MAX_FILE_TRANSFERS    = 1024  // Maximum files queued up at once
    };

//--------------------------------------------------------------------------------------------
// egonet_instance_t
//--------------------------------------------------------------------------------------------

    const bool_t     egonet_instance_get_serviceon( const egonet_instance_t * pnet );
    const bool_t     egonet_instance_get_hostactive( const egonet_instance_t * pnet );
    const bool_t     egonet_instance_get_readytostart( const egonet_instance_t * pnet );
    const bool_t     egonet_instance_get_waitingforclients( const egonet_instance_t * pnet );
    const int        egonet_instance_get_local_machine( const egonet_instance_t * pnet );
    const int        egonet_instance_get_client_count( const egonet_instance_t * pnet );
    const ENetHost * egonet_instance_get_myHost( const egonet_instance_t * pnet );

    bool_t egonet_instance_set_hostactive( egonet_instance_t * pnet, const bool_t val );
    bool_t egonet_instance_set_waitingforclients(  egonet_instance_t * pnet, const bool_t val );
    bool_t egonet_instance_set_readytostart(  egonet_instance_t * pnet, const bool_t val );
    bool_t egonet_instance_set_myHost(  egonet_instance_t * pnet, const ENetHost* phost );

//--------------------------------------------------------------------------------------------
// BaseConnectionInfo_t
//--------------------------------------------------------------------------------------------

/// Network information on connected players
    struct s_BaseConnectionInfo
    {
        int connection_slot;
    };

#define BASE_CONNECTION_INFO_INIT \
    { \
        -1 /* int playerSlot */ \
    }

    BaseConnectionInfo_t * BaseConnectionInfo_ctor( BaseConnectionInfo_t * ptr );
    BaseConnectionInfo_t * BaseConnectionInfo_dtor( BaseConnectionInfo_t * ptr );

//--------------------------------------------------------------------------------------------
// BaseServerState_t
//--------------------------------------------------------------------------------------------

    struct s_BaseServerState
    {
        bool_t am_host;

        Uint32 last_frame;

        size_t                 client_size;             ///< Number of units allocated
        STRING               * client_name;             ///< Names of machines
        BaseConnectionInfo_t * client_info;             ///< Connection info for each client
        int                    client_count;            ///< How many we found
    };

#define BASE_SERVER_STATE_INIT \
    { \
        bfalse,       /* bool_t                 am_host       */ \
        (Uint32)(~0), /* Uint32                 last_frame    */ \
        0,            /* size_t                 client_size   */ \
        NULL,         /* STRING               * client_name   */ \
        NULL,         /* BaseConnectionInfo_t * client_info   */ \
        0             /* int                    client_count  */ \
    }

    BaseServerState_t * BaseServerState_ctor( BaseServerState_t * ptr, size_t size );
    BaseServerState_t * BaseServerState_dtor( BaseServerState_t * ptr );

//--------------------------------------------------------------------------------------------

/// A mockup of an actual client state
    struct s_BaseClientState
    {
        ENetPeer* gameHost;

        size_t    peer_cnt;
        ENetPeer* peer_ary;
    };

#define BASE_CLIENT_STATE_INIT \
    { \
        NULL, /* ENetPeer* gameHost */ \
        0,    /* size_t    peer_cnt */ \
        NULL, /* ENetPeer* peer_ary */ \
    }

    BaseClientState_t * BaseClientState_ctor( BaseClientState_t * ptr, size_t peers );
    BaseClientState_t * BaseClientState_dtor( BaseClientState_t * ptr);

//--------------------------------------------------------------------------------------------
// time_latch_t
//--------------------------------------------------------------------------------------------

/// A latch with a time attached
/// @details This is recieved over the network, or inserted into the list by the local system to simulate
///  network traffic
    struct s_time_latch
    {
        float   x;
        float   y;
        Uint32  button;
        Uint32  time;
    };

    void tlatch_ary_init( time_latch_t ary[], size_t len );

//--------------------------------------------------------------------------------------------
// egonet_instance_t
//--------------------------------------------------------------------------------------------

    const bool_t     egonet_on( void );

//--------------------------------------------------------------------------------------------
// Packet reading/writing
//--------------------------------------------------------------------------------------------

    struct s_ego_packet
    {
        Uint32  head;                             // The write head
        Uint32  size;                             // The size of the packet
        Uint8   buffer[MAXSENDSIZE];              // The data packet
    };

    ego_packet_t * ego_packet_ctor( ego_packet_t * ptr );
    ego_packet_t * ego_packet_dtor( ego_packet_t * ptr );
    bool_t ego_packet_begin( ego_packet_t * ptr );
    bool_t ego_packet_addUint8( ego_packet_t * ptr, Uint8 uc );
    bool_t ego_packet_addSint8( ego_packet_t * ptr, Sint8 sc );
    bool_t ego_packet_addUint16( ego_packet_t * ptr, Uint16 us );
    bool_t ego_packet_addSint16( ego_packet_t * ptr, Sint16 ss );
    bool_t ego_packet_addUint32( ego_packet_t * ptr, Uint32 ui );
    bool_t ego_packet_addSint32( ego_packet_t * ptr, Sint32 si );
    bool_t ego_packet_addString( ego_packet_t * ptr, const char *string );

    // functions that have no use at the moment
    bool_t ego_packet_readString( ego_packet_t * ptr, char *buffer, int maxLen );
    bool_t ego_packet_readUint8( ego_packet_t * ptr, Uint8 * pval );
    bool_t ego_packet_readSint8( ego_packet_t * ptr, Sint8 * pval );
    bool_t ego_packet_readUint16( ego_packet_t * ptr, Uint16 * pval );
    bool_t ego_packet_readSint16( ego_packet_t * ptr, Sint16 * pval );
    bool_t ego_packet_readUint32( ego_packet_t * ptr, Uint32 * pval );
    bool_t ego_packet_readSint32( ego_packet_t * ptr, Sint32 * pval );

/// enet packet wrapper
    struct s_enet_packet
    {
        ENetPacket*    ptr;
        size_t         read_location;
    };

    enet_packet_t * enet_packet_ctor( enet_packet_t * );
    enet_packet_t * enet_packet_dtor( enet_packet_t * );
    bool_t enet_packet_startReading( enet_packet_t * ptr, ENetPacket *packet );
    bool_t enet_packet_doneReading( enet_packet_t * ptr );
    size_t enet_packet_remainingSize( enet_packet_t * ptr );
    bool_t enet_packet_readString( enet_packet_t * ptr, char *buffer, size_t maxLen );
    bool_t enet_packet_readUint8( enet_packet_t * ptr, Uint8 * pval );
    bool_t enet_packet_readSint8( enet_packet_t * ptr, Sint8 * pval );
    bool_t enet_packet_readUint16( enet_packet_t * ptr, Uint16 * pval );
    bool_t enet_packet_readSint16( enet_packet_t * ptr, Sint16 * pval );
    bool_t enet_packet_readUint32( enet_packet_t * ptr, Uint32 * pval );
    bool_t enet_packet_readSint32( enet_packet_t * ptr, Sint32 * pval );

//--------------------------------------------------------------------------------------------
// Networking functions
//--------------------------------------------------------------------------------------------

    void egonet_initialize( BaseServerState_t * ps, BaseClientState_t * pc, size_t size, bool_t req_enet );
    void egonet_shutDown( void );
    bool_t egonet_begin( bool_t req_enet );
    bool_t egonet_end();

    egonet_instance_t * egonet_get_instance( void );

    void egonet_listen_for_packets( void );
    void egonet_unbuffer_player_latches( void );
    void egonet_close_session( void );
    void egonet_turn_on_service( int service );
    void egonet_logf( const char *format, ... );

// network packet handling
    bool_t egonet_broadcastPacket( ego_packet_t * pkt );
    bool_t egonet_broadcastPacketGuaranteed( ego_packet_t * pkt );
    bool_t egonet_sendPacketToOneClientGuaranteed( ego_packet_t * pkt, int player );
    bool_t egonet_sendPacketToPeer( ego_packet_t * ptr, ENetPeer *peer );
    bool_t egonet_sendPacketToPeerGuaranteed( ego_packet_t * ptr, ENetPeer *peer );

    // get values from the instance singleton
    const bool_t     egonet_get_serviceon( void );
    const bool_t     egonet_get_hostactive( void );
    const bool_t     egonet_get_readytostart( void );
    const bool_t     egonet_get_waitingforclients( void );
    const int        egonet_get_local_machine( void );
    const int        egonet_get_client_count( void );
    const ENetHost * egonet_get_myHost( void );

    // set values from the instance singleton
    bool_t egonet_set_hostactive( const bool_t val );
    bool_t egonet_set_waitingforclients(  const bool_t val );
    bool_t egonet_set_readytostart(  const bool_t val );
    bool_t egonet_set_myHost(  const ENetHost* phost );

// functions that must be implemented externally
    extern egolib_rv egonet_dispatchEvent( ENetEvent *event );
    extern egolib_rv egonet_handlePacket(  enet_packet_t * enet_pkt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_network_h