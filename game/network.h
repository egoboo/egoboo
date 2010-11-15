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
#include "IDSZ_map.h"

//--------------------------------------------------------------------------------------------
// Network stuff
//--------------------------------------------------------------------------------------------

#define NETREFRESH          1000                    ///< Every second
#define NONETWORK           numservice
#define MAXSERVICE          16
#define NETNAMESIZE         16
#define MAXSESSION          16
#define MAXNETPLAYER         8

#define TO_ANY_TEXT         25935                               ///< Message headers
#define TO_HOST_MODULEOK    14951
#define TO_HOST_MODULEBAD   14952
#define TO_HOST_LATCH       33911
#define TO_HOST_RTS         30376
#define TO_HOST_IM_LOADED   40192
#define TO_HOST_FILE        20482
#define TO_HOST_DIR         49230
#define TO_HOST_FILESENT    13131
#define TO_REMOTE_MODULE    56025
#define TO_REMOTE_LATCH     12715
#define TO_REMOTE_FILE      62198
#define TO_REMOTE_DIR       11034
#define TO_REMOTE_RTS        5143
#define TO_REMOTE_START     51390
#define TO_REMOTE_FILESENT  19903

#define SHORTLATCH 1024.0f
#define CHARVEL 5.0f
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152
#define MAX_PLAYER   8                               ///< 2 to a power...  2^3
#define MAXLAG      64
#define LAGAND      63
#define STARTTALK   10

//--------------------------------------------------------------------------------------------
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
typedef struct s_time_latch time_latch_t;

//--------------------------------------------------------------------------------------------
struct s_input_device
{
    bool_t                  on;              ///< Is it alive?
    BIT_FIELD               bits;

    float                   sustain;         ///< Falloff rate for old movement
    float                   cover;           ///< For falloff

    latch_t                 latch;
    latch_t                 latch_old;       ///< For sustain
};
typedef struct s_input_device input_device_t;

void input_device_init( input_device_t * pdevice );
void input_device_add_latch( input_device_t * pdevice, float newx, float newy );

//--------------------------------------------------------------------------------------------

/// The state of a player
struct s_player
{
    bool_t                  valid;                    ///< Player used?
    CHR_REF                 index;                    ///< Which character?

    /// the buffered input from the local input devices
    input_device_t          device;

    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    latch_t                 local_latch;

    // quest log for this player
    IDSZ_node_t             quest_log[MAX_IDSZ_MAP_SIZE];          ///< lists all the character's quests

    // Timed latches
    Uint32                  tlatch_count;
    time_latch_t            tlatch[MAXLAG];

    /// Network latch, set by unbuffer_player_latches(), used to set the local character's latch
    latch_t                 net_latch;
};

typedef struct s_player player_t;

extern int                     local_numlpla;                                   ///< Number of local players

#define INVALID_PLAYER MAX_PLAYER

DECLARE_STACK_EXTERN( player_t, PlaStack, MAX_PLAYER );                         ///< Stack for keeping track of players

#define VALID_PLA_RANGE(IPLA) ( ((IPLA) >= 0) && ((IPLA) < MAX_PLAYER) )
#define VALID_PLA(IPLA)       ( VALID_PLA_RANGE(IPLA) && ((IPLA) < PlaStack.count) && PlaStack.lst[IPLA].valid )
#define INVALID_PLA(IPLA)     ( !VALID_PLA_RANGE(IPLA) || ((IPLA) >= PlaStack.count)|| !PlaStack.lst[IPLA].valid )

void           player_init( player_t * ppla );
void           pla_reinit( player_t * ppla );
CHR_REF        pla_get_ichr( const PLA_REF iplayer );
struct s_chr * pla_get_pchr( const PLA_REF iplayer );

//--------------------------------------------------------------------------------------------

/// The state of the network code used in old-egoboo
struct s_net_instance
{
    bool_t  on;                      ///< Try to connect?
    bool_t  serviceon;               ///< Do I need to free the interface?
    bool_t  hostactive;              ///< Hosting?
    bool_t  readytostart;            ///< Ready to hit the Start Game button?
    bool_t  waitingforplayers;       ///< Has everyone talked to the host?
};
typedef struct s_net_instance net_instance_t;

extern net_instance_t * PNet;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Network orders
//extern unsigned char           ordervalid[MAXORDER];
//extern unsigned char           orderwho[MAXORDER][MAXSELECT];
//extern unsigned int            orderwhat[MAXORDER];
//extern unsigned int            orderwhen[MAXORDER];

extern Uint32                  nexttimestamp;                ///< Expected timestamp
extern FILE                   *globalnetworkerr;             ///< For debuggin' network

extern Uint32                  randsave;                  ///< Used in network timer
extern int                     networkservice;
extern int                     numservice;                                 ///< How many we found
extern char                    netservicename[MAXSERVICE][NETNAMESIZE];    ///< Names of services
extern int                     numsession;                                 ///< How many we found
extern char                    netsessionname[MAXSESSION][NETNAMESIZE];    ///< Names of sessions
extern int                     numplayer;                                  ///< How many we found
extern char                    netplayername[MAXNETPLAYER][NETNAMESIZE];   ///< Names of machines

extern int                     local_machine;        ///< 0 is host, 1 is 1st remote, 2 is 2nd...

extern int                     playersready;               ///< Number of players ready to start
extern int                     playersloaded;

extern Uint32                  numplatimes;

//--------------------------------------------------------------------------------------------
// Networking functions
//--------------------------------------------------------------------------------------------

void listen_for_packets();
void unbuffer_player_latches();
void close_session();

void net_initialize();
void net_shutDown();
void net_logf( const char *format, ... );

void net_startNewPacket();

void packet_addUnsignedByte( Uint8 uc );
void packet_addSignedByte( Sint8 sc );
void packet_addUnsignedShort( Uint16 us );
void packet_addSignedShort( Sint16 ss );
void packet_addUnsignedInt( Uint32 ui );
void packet_addSignedInt( Sint32 si );
void packet_addString( const char *string );

void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed( int player );
void net_send_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers( const char *source, const char *dest );
void net_copyFileToHost( const char *source, const char *dest );
void net_copyDirectoryToHost( const char *dirname, const char *todirname );
void net_copyDirectoryToAllPlayers( const char *dirname, const char *todirname );
void net_sayHello();
void cl_talkToHost();
void sv_talkToRemotes();

int sv_hostGame();
int cl_joinGame( const char *hostname );

void find_open_sessions();
void sv_letPlayersJoin();
void stop_players_from_joining();
// int create_player(int host);
// void turn_on_service(int service);

void net_reset_players();

void tlatch_ary_init( time_latch_t ary[], size_t len );

player_t*      chr_get_ppla( const CHR_REF ichr );
