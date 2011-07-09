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

#include "egoboo_typedef.h"

#include <enet/enet.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// an opaque struct for encapsulating network data
struct s_net_instance;
typedef struct s_net_instance net_instance_t;

struct s_time_latch;
typedef struct s_time_latch time_latch_t;

struct s_chat_buffer;
typedef struct s_chat_buffer chat_buffer_t;

/// opaque wrapper for the enet packet data
struct s_ego_packet;
typedef struct s_ego_packet ego_packet_t;

/// opaque wrapper for the enet packet data
struct s_enet_packet;
typedef struct s_enet_packet enet_packet_t;

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

#define CHAT_BUFFER_SIZE 2048

#define NET_EGOBOO_PORT  0x8742

/// packet headers
enum e_net_commands
{
    TO_ANY_TEXT         = 0x654F,

    TO_HOST_MODULEOK    = 0x3A67,
    TO_HOST_MODULEBAD   = 0x3A68,
    TO_HOST_LATCH       = 0x8477,
    TO_HOST_IM_LOADED   = 0x9D00,
    NETFILE_TO_HOST_FILE        = 0x5002,
    NETFILE_TO_HOST_DIR         = 0xC04E,
    NETFILE_TO_HOST_SENT    = 0x334B,

    TO_REMOTE_MODULE    = 0xDAD9,
    TO_REMOTE_LATCH     = 0x31AB,
    NETFILE_TO_REMOTE_FILE      = 0xF2F6,
    NETFILE_TO_REMOTE_DIR       = 0x2B1A,
    TO_REMOTE_START     = 0xC8BE,
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
// net_instance_t
//--------------------------------------------------------------------------------------------

net_instance_t * net_instance_ctor( net_instance_t * pnet );
bool_t net_on( const net_instance_t * pnet );
bool_t net_serviceon( const net_instance_t * pnet );
bool_t net_get_hostactive( const net_instance_t * pnet );
bool_t net_get_readytostart( const net_instance_t * pnet );
bool_t net_waitingforplayers( const net_instance_t * pnet );
int    net_local_machine( const net_instance_t * pnet );
ENetHost* net_get_myHost( const net_instance_t * pnet );

bool_t net_set_hostactive( net_instance_t * pnet, bool_t val );
bool_t net_set_waitingforplayers( net_instance_t * pnet, bool_t val );
bool_t net_set_readytostart( net_instance_t * pnet, bool_t val );
bool_t net_set_myHost( net_instance_t * pnet, ENetHost* phost );

int    net_get_player_count( const net_instance_t * pnet );

//--------------------------------------------------------------------------------------------
// CHAT BUFFER

struct s_chat_buffer
{
    int     buffer_count;
    char    buffer[CHAT_BUFFER_SIZE];
};

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
//static bool_t ego_packet_readString( ego_packet_t * ptr, char *buffer, int maxLen );
//static bool_t ego_packet_readUint8( ego_packet_t * ptr, Uint8 * pval );
//static bool_t ego_packet_readSint8( ego_packet_t * ptr, Sint8 * pval );
//static bool_t ego_packet_readUint16( ego_packet_t * ptr, Uint16 * pval );
//static bool_t ego_packet_readSint16( ego_packet_t * ptr, Sint16 * pval );
//static bool_t ego_packet_readUint32( ego_packet_t * ptr, Uint32 * pval );
//static bool_t ego_packet_readSint32( ego_packet_t * ptr, Sint32 * pval );

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
//--------------------------------------------------------------------------------------------

extern net_instance_t * PNet;

extern Uint32        nexttimestamp;                ///< Expected timestamp

extern Uint32        randsave;                  ///< Used in network timer

extern int           net_players_ready;              ///< Number of players ready to start
extern int           net_players_loaded;

extern Uint32        numplatimes;

extern chat_buffer_t net_chat;

//--------------------------------------------------------------------------------------------
// Networking functions
//--------------------------------------------------------------------------------------------

void net_listen_for_packets( void );
void net_unbuffer_player_latches( void );
void net_close_session( void );

void net_initialize( void );
void net_shutDown( void );
void net_logf( const char *format, ... );

void net_sayHello( void );
void net_send_message( void );

void net_updateFileTransfers( void );
int  net_pendingFileTransfers( void );
void net_copyFileToAllPlayers( const char *source, const char *dest );
void net_copyDirectoryToAllPlayers( const char *dirname, const char *todirname );
void net_copyFileToPeer( const char *source, const char *dest, ENetPeer * peer );
void net_copyDirectoryToPeer( const char *dirname, const char *todirname, ENetPeer * peer );

void find_open_sessions( void );
void stop_players_from_joining( void );
// int create_player(int host);
// void turn_on_service(int service);

void net_count_players( net_instance_t * pnet );

bool_t net_sendPacketToAllPlayers( ego_packet_t * pkt );
bool_t net_sendPacketToAllPlayersGuaranteed( ego_packet_t * pkt );
bool_t net_sendPacketToOnePlayerGuaranteed( ego_packet_t * pkt, int player );
bool_t net_sendPacketToPeer( ego_packet_t * ptr, ENetPeer *peer );
bool_t net_sendPacketToPeerGuaranteed( ego_packet_t * ptr, ENetPeer *peer );
//bool_t net_sendPacketToHostGuaranteed( ego_packet_t * pkt );
//bool_t net_sendPacketToHost( ego_packet_t * pkt );
