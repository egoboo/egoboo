/* Egoboo - Network.h
 * Definitions for Egoboo network functionality
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef egoboo_Network_h
#define egoboo_Network_h

#include "enet.h"
#include "egoboo.h"
#include "input.h"

#define NETREFRESH          1000                    // Every second
#define NONETWORK           GNet.num_service              //

// The ID number for host searches
// {A0F72DE8-2C17-11d3-B7FE-444553540000}
/* PORT
DEFINE_GUID(NETWORKID, 0xa0f72de8, 0x2c17, 0x11d3, 0xb7, 0xfe, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
*/
#define MAXSERVICE 16
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8
EXTERN Uint32      randsave;         //Used in network timer

typedef struct network_info_t
{
  bool_t                  messagemode;             // Input text from keyboard?
  Uint8                   messagedelay;            // For slowing down input
  int                     messagewrite;            // The cursor position
  int                     messagewritemin;         // The starting cursor position
  char                    message[MESSAGESIZE];    // The input message

  int                     num_service;                           // How many we found
  int                     service;
  char                    servicename[MAXSERVICE][NETNAMESIZE];    // Names of services

  int                     num_session;                           // How many we found
  char                    sessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions

  int                     num_player;                            // How many we found
  char                    playername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines
} NETWORK_INFO;

extern NETWORK_INFO GNet;

#define SHORTLATCH 1024.0
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152

#define MAXLAG      64                              //
#define LAGAND      63                              //
#define STARTTALK   10                              //

#define INVALID_TIMESTAMP (~(Uint32)0)

#define TO_ANY_TEXT         25935                               // Message headers
#define TO_HOST_MODULEOK    14951                               //
#define TO_HOST_LATCH       33911                               //
#define TO_HOST_RTS         30376                               //
#define TO_HOST_IM_LOADED   40192                               //
#define TO_HOST_FILE        20482                               //
#define TO_HOST_DIR         49230                               //
#define TO_HOST_FILESENT    13131                               //
#define TO_REMOTE_MODULE    56025                               //
#define TO_REMOTE_LATCH     12715                               //
#define TO_REMOTE_FILE      62198                               //
#define TO_REMOTE_DIR       11034                               //
#define TO_REMOTE_RTS        5143                               //
#define TO_REMOTE_START     51390                               //
#define TO_REMOTE_FILESENT  19903                               //


// Networking constants
enum NetworkConstant
{
  NET_UNRELIABLE_CHANNEL  = 0,
  NET_GUARANTEED_CHANNEL  = 1,
  NET_EGOBOO_NUM_CHANNELS,
  NET_EGOBOO_PORT    = 34626,
  NET_MAX_FILE_NAME   = 128,
  NET_MAX_FILE_TRANSFERS  = 1024, // Maximum files queued up at once
};

// Network messages
enum NetworkMessage
{
  NET_TRANSFER_FILE   = 10001, // Packet contains a file.
  NET_TRANSFER_OK    = 10002, // Acknowledgement packet for a file send
  NET_CREATE_DIRECTORY  = 10003, // Tell the peer to create the named directory
  NET_DONE_SENDING_FILES  = 10009, // Sent when there are no more files to send.
  NET_NUM_FILES_TO_SEND  = 10010, // Let the other person know how many files you're sending
};

// Network players information
typedef struct NetPlayerInfo
{
  int playerSlot;
} NetPlayerInfo;

// ENet host & client identifiers
extern ENetHost* net_myHost;
extern ENetPeer* net_gameHost;
extern ENetPeer* net_playerPeers[MAXPLAYER];
extern NetPlayerInfo net_playerInfo[MAXNETPLAYER];

extern bool_t net_amHost;

// Packet reading
extern ENetPacket*  net_readPacket;
extern size_t       net_readLocation;

// Packet writing
typedef struct packet_t
{
  Uint32 head;                             // The write head
  Uint32 size;                             // The size of the packet
  Uint8  buffer[MAXSENDSIZE];              // The data packet
} PACKET;

extern PACKET gPacket;

void packet_startReading( ENetPacket *packet );
void packet_doneReading();
size_t packet_remainingSize();
void packet_addUnsignedByte( Uint8 uc );
void packet_addSignedByte( Sint8 sc );
void packet_addUnsignedShort( Uint16 us );
void packet_addSignedShort( Sint16 ss );
void packet_addUnsignedInt( Uint32 ui );
void packet_addSignedInt( Sint32 si );
void packet_addString( char *string );
void packet_readString( char *buffer, int maxLen );
Uint8 packet_readUnsignedByte();
Sint8 packet_readSignedByte();
Uint16 packet_readUnsignedShort();
Sint16 packet_readSignedShort();
Uint32 packet_readUnsignedInt();
Sint32 packet_readSignedInt();
Uint16 packet_peekUnsignedShort();




void net_startNewPacket();
void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed( int player );
void net_sendPacketToPeer( ENetPeer *peer );
void net_sendPacketToPeerGuaranteed( ENetPeer *peer );
void net_copyFileToAllPlayers( char *source, char *dest );
void net_copyFileToAllPlayersOld( char *source, char *dest );
void net_copyFileToHost( char *source, char *dest );
void net_copyFileToHostOld( char *source, char *dest );
void net_copyDirectoryToHost( char *dirname, char *todirname );
void net_copyDirectoryToAllPlayers( char *dirname, char *todirname );
void net_sayHello();
bool_t net_handlePacket( ENetEvent *event );
void net_initialize();
void net_shutDown();
int  net_pendingFileTransfers();
void net_updateFileTransfers();


//---------------------------------------------------------------------------------------------
// Networking functions
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
void packet_addString( char *string );

void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed( int player );
void input_net_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers( char *source, char *dest );
void net_copyFileToHost( char *source, char *dest );
void net_copyDirectoryToHost( char *dirname, char *todirname );
void net_copyDirectoryToAllPlayers( char *dirname, char *todirname );
void net_sayHello();

bool_t listen_for_packets();
void find_open_sessions();
void stop_players_from_joining();
//int create_player(int host);
//void turn_on_service(int service);
#endif
