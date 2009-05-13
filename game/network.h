#pragma once

#include "egoboo_typedef.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Network stuff

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice
#define MAXSERVICE          16
#define NETNAMESIZE         16
#define MAXSESSION          16
#define MAXNETPLAYER         8

#define TO_ANY_TEXT         25935                               // Message headers
#define TO_HOST_MODULEOK    14951
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
#define MAXPLAYER   8                               // 2 to a power...  2^3
#define MAXLAG      64
#define LAGAND      63
#define STARTTALK   10

extern Uint32                  nexttimestamp;                // Expected timestamp
extern FILE                   *globalnetworkerr;             // For debuggin' network

extern Uint32                  randsave;                  // Used in network timer
extern int                     networkservice;
extern char                    nethostname[64];                            // Name for hosting session
extern char                    netmessagename[64];                         // Name for messages
extern int                     numservice;                                 // How many we found
extern char                    netservicename[MAXSERVICE][NETNAMESIZE];    // Names of services
extern int                     numsession;                                 // How many we found
extern char                    netsessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions
extern int                     numplayer;                                  // How many we found
extern char                    netplayername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines

extern int                     local_machine;        // 0 is host, 1 is 1st remote, 2 is 2nd...

struct s_time_latch
{
    float   x;
    float   y;
    Uint32  button;
    Uint32  time;
};
typedef struct s_time_latch time_latch_t;

struct s_player
{
    bool_t                  valid;                    // Player used?
    Uint16                  index;                    // Which character?
    Uint8                   device;                   // Input device

    // Local latch
    float                   latchx;
    float                   latchy;
    Uint32                  latchbutton;

    // Timed latches
    Uint32                  tlatch_count;
    time_latch_t            tlatch[MAXLAG];
};

typedef struct s_player player_t;

extern int                     lag;                             // Lag tolerance
extern Uint32                  numplatimes;

extern int                     numpla;                                   // Number of players
extern int                     local_numlpla;
extern player_t                PlaList[MAXPLAYER];

//---------------------------------------------------------------------------------------------
// Networking functions

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
