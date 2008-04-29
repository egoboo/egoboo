/* Egoboo - network.c
 * Shuttles bits across the network, using Enet.  Networked play doesn't
 * really work at the moment.
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

#include "Network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "input.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// ENet host & client identifiers
ENetHost* net_myHost = NULL;
ENetPeer* net_gameHost = NULL;
ENetPeer* net_playerPeers[MAXPLAYER];
NetPlayerInfo net_playerInfo[MAXNETPLAYER];

NETWORK_INFO GNet;

bool_t net_amHost = bfalse;
PACKET gPacket;

// Packet reading
ENetPacket* net_readPacket = NULL;
size_t      net_readLocation = 0;

// File transfer variables & structures
typedef struct net_file_transfer_t
{
  char sourceName[NET_MAX_FILE_NAME];
  char destName[NET_MAX_FILE_NAME];
  ENetPeer *target;
} NetFileTransfer;

// File transfer queue
NetFileTransfer net_transferStates[NET_MAX_FILE_TRANSFERS];
int net_numFileTransfers = 0;
int net_fileTransferHead = 0; // Queue indices
int net_fileTransferTail = 0;
int net_waitingForXferAck = 0;

// Receiving files
NetFileTransfer net_receiveState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void packet_addUnsignedByte( Uint8 uc )
{
  // ZZ> This function appends an Uint8 to the packet
  Uint8* ucp;
  ucp = ( Uint8* )( &gPacket.buffer[gPacket.head] );
  *ucp = uc;
  gPacket.head += 1;
  gPacket.size += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedByte( Sint8 sc )
{
  // ZZ> This function appends a Sint8 to the packet
  Sint8* scp;
  scp = ( Sint8* )( &gPacket.buffer[gPacket.head] );
  *scp = sc;
  gPacket.head += 1;
  gPacket.size += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedShort( Uint16 us )
{
  // ZZ> This function appends an Uint16 to the packet
  Uint16* usp;
  usp = ( Uint16* )( &gPacket.buffer[gPacket.head] );

  *usp = ENET_HOST_TO_NET_16( us );
  gPacket.head += 2;
  gPacket.size += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedShort( Sint16 ss )
{
  // ZZ> This function appends a Sint16 to the packet
  Sint16* ssp;
  ssp = ( Sint16* )( &gPacket.buffer[gPacket.head] );

  *ssp = ENET_HOST_TO_NET_16( ss );

  gPacket.head += 2;
  gPacket.size += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedInt( Uint32 ui )
{
  // ZZ> This function appends an Uint32 to the packet
  Uint32* uip;
  uip = ( Uint32* )( &gPacket.buffer[gPacket.head] );

  *uip = ENET_HOST_TO_NET_32( ui );

  gPacket.head += 4;
  gPacket.size += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedInt( Sint32 si )
{
  // ZZ> This function appends a Sint32 to the packet
  Sint32* sip;
  sip = ( Sint32* )( &gPacket.buffer[gPacket.head] );

  *sip = ENET_HOST_TO_NET_32( si );

  gPacket.head += 4;
  gPacket.size += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addString( char *string )
{
  // ZZ> This function appends a null terminated string to the packet
  char* cp;
  char cTmp;
  int cnt;

  cnt = 0;
  cTmp = 1;
  cp = ( char* )( &gPacket.buffer[gPacket.head] );
  while ( cTmp != 0 )
  {
    cTmp = string[cnt];
    *cp = cTmp;
    cp += 1;
    gPacket.head += 1;
    gPacket.size += 1;
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
  // ZZ> This function reads a null terminated string from the packet
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
  // ZZ> This function reads an Uint8 from the packet
  Uint8 uc;
  uc = ( Uint8 ) net_readPacket->data[net_readLocation];
  net_readLocation++;
  return uc;
}

//--------------------------------------------------------------------------------------------
Sint8 packet_readSignedByte()
{
  // ZZ> This function reads a Sint8 from the packet
  Sint8 sc;
  sc = ( Sint8 ) net_readPacket->data[net_readLocation];
  net_readLocation++;
  return sc;
}

//--------------------------------------------------------------------------------------------
Uint16 packet_readUnsignedShort()
{
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = ( Uint16* )( &net_readPacket->data[net_readLocation] );

  us = ENET_NET_TO_HOST_16( *usp );

  net_readLocation += 2;
  return us;
}

//--------------------------------------------------------------------------------------------
Uint16 packet_peekUnsignedShort()
{
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = ( Uint16* )( &net_readPacket->data[net_readLocation] );

  us = ENET_NET_TO_HOST_16( *usp );

  return us;
}

//--------------------------------------------------------------------------------------------
Sint16 packet_readSignedShort()
{
  // ZZ> This function reads a Sint16 from the packet
  Sint16 ss;
  Sint16* ssp;
  ssp = ( Sint16* )( &net_readPacket->data[net_readLocation] );

  ss = ENET_NET_TO_HOST_16( *ssp );

  net_readLocation += 2;
  return ss;
}

//--------------------------------------------------------------------------------------------
Uint32 packet_readUnsignedInt()
{
  // ZZ> This function reads an Uint32 from the packet
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
  // ZZ> This function reads a Sint32 from the packet
  Sint32 si;
  Sint32* sip;
  sip = ( Sint32* )( &net_readPacket->data[net_readLocation] );

  si = ENET_NET_TO_HOST_32( *sip );

  net_readLocation += 4;
  return si;
}

//--------------------------------------------------------------------------------------------
size_t packet_remainingSize()
{
  // ZZ> This function tells if there's still data left in the packet
  return net_readPacket->dataLength - net_readLocation;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void net_startNewPacket()
{
  // ZZ> This function starts building a network packet
  gPacket.head = 0;
  gPacket.size = 0;
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHost()
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet = enet_packet_create( gPacket.buffer, gPacket.size, 0 );
  enet_peer_send( net_gameHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayers()
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet;

  if ( NULL == net_myHost ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, 0 );
  enet_host_broadcast( net_myHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHostGuaranteed()
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet;

  if ( NULL == net_myHost ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, ENET_PACKET_FLAG_RELIABLE );
  enet_peer_send( net_gameHost, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayersGuaranteed()
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet;

  if ( NULL == net_myHost ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, ENET_PACKET_FLAG_RELIABLE );
  enet_host_broadcast( net_myHost, NET_GUARANTEED_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToOnePlayerGuaranteed( int player )
{
  // ZZ> This function sends a packet to one of the players
  ENetPacket *packet;

  if ( NULL == net_myHost ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, ENET_PACKET_FLAG_RELIABLE );
  if ( player < GNet.num_player )
  {
    enet_peer_send( &net_myHost->peers[player], NET_GUARANTEED_CHANNEL, packet );
  }
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeer( ENetPeer *peer )
{
  // JF> This function sends a packet to a given peer
  ENetPacket *packet;

  if ( NULL == peer ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, ENET_PACKET_FLAG_RELIABLE );
  enet_peer_send( peer, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeerGuaranteed( ENetPeer *peer )
{
  // JF> This funciton sends a packet to a given peer, with guaranteed delivery
  ENetPacket *packet;

  if ( NULL == peer ) return;

  packet = enet_packet_create( gPacket.buffer, gPacket.size, 0 );
  enet_peer_send( peer, NET_GUARANTEED_CHANNEL, packet );
}

//------------------------------------------------------------------------------
void net_copyFileToAllPlayers( char *source, char *dest )
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to MAXPLAYER players...
  NetFileTransfer *state;

  if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = & ( net_transferStates[net_fileTransferTail] );
    assert( state->sourceName[0] == 0 );

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

//------------------------------------------------------------------------------
void net_copyFileToAllPlayersOld( char *source, char *dest )
{
  // ZZ> This function copies a file on the host to every remote computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetend, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;

  if ( !CData.network_on  || !hostactive || NULL == net_myHost ) return;

  log_info( "net_copyFileToAllPlayers: %s, %s\n", source, dest );

  fileisdir = fs_fileIsDirectory( source );
  if ( fileisdir )
  {
    net_startNewPacket();
    packet_addUnsignedShort( TO_REMOTE_DIR );
    packet_addString( dest );
    net_sendPacketToAllPlayersGuaranteed();
  }
  else
  {
    fileread = fs_fileOpen( PRI_NONE, NULL, source, "rb" );
    if ( NULL != fileread )
    {
      fseek( fileread, 0, SEEK_END );
      filesize = ftell( fileread );
      fseek( fileread, 0, SEEK_SET );
      if ( filesize > 0 && filesize < TOTALSIZE )
      {
        packetend = 0;
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
          //fread((gPacket.buffer + gPacket.head), COPYSIZE, 1, fileread);

          // But I'll leave it alone for now
          fscanf( fileread, "%c", &cTmp );

          packet_addUnsignedByte( cTmp );
          packetend++;
          packetstart++;
          if ( packetend >= COPYSIZE )
          {
            // Send off the packet
            net_sendPacketToAllPlayersGuaranteed();
            enet_host_flush( net_myHost );

            // Start on the next 4K
            packetend = 0;
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
      fs_fileClose( fileread );
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToHost( char *source, char *dest )
{
  NetFileTransfer *state;

  // JF> New function merely queues up a new file to be sent

  // If this is the host, just copy the file locally
  if ( hostactive )
  {
    // Simulate a network transfer
    if ( fs_fileIsDirectory( source ) )
    {
      fs_createDirectory( dest );
    }
    else
    {
      fs_copyFile( source, dest );
    }
    return;
  }

  if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = & ( net_transferStates[net_fileTransferTail] );
    assert( state->sourceName[0] == 0 );

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

//------------------------------------------------------------------------------
void net_copyFileToHostOld( char *source, char *dest )
{
  // ZZ> This function copies a file on the remote to the host computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetend, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;

  log_info( "net_copyFileToHost: \n" );
  fileisdir = fs_fileIsDirectory( source );
  if ( hostactive )
  {
    // Simulate a network transfer
    if ( fileisdir )
    {
      log_info( "Creating local directory %s\n", dest );
      fs_createDirectory( dest );
    }
    else
    {
      log_info( "Copying local file %s --> %s\n", source, dest );
      fs_copyFile( source, dest );
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
//   net_sendPacketToAllPlayersGuaranteed();
      net_sendPacketToHost();
    }
    else
    {
      log_info( "Copying local file to host file: %s --> %s\n", source, dest );
      fileread = fs_fileOpen( PRI_NONE, NULL, source, "rb" );
      if ( NULL != fileread )
      {
        fseek( fileread, 0, SEEK_END );
        filesize = ftell( fileread );
        fseek( fileread, 0, SEEK_SET );
        if ( filesize > 0 && filesize < TOTALSIZE )
        {
          numfilesent++;
          packetend = 0;
          packetstart = 0;
          net_startNewPacket();
          packet_addUnsignedShort( TO_HOST_FILE );
          packet_addString( dest );
          packet_addUnsignedInt( filesize );
          packet_addUnsignedInt( packetstart );
          while ( packetstart < filesize )
          {
            fscanf( fileread, "%c", &cTmp );
            packet_addUnsignedByte( cTmp );
            packetend++;
            packetstart++;
            if ( packetend >= COPYSIZE )
            {
              // Send off the packet
              net_sendPacketToHostGuaranteed();
              enet_host_flush( net_myHost );

              // Start on the next 4K
              packetend = 0;
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
        fs_fileClose( fileread );
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToHost( char *dirname, char *todirname )
{
  // ZZ> This function copies all files in a directory
  char searchname[128];
  char fromname[128];
  char toname[128];
  const char *searchResult;

  log_info( "net_copyDirectoryToHost: %s, %s\n", dirname, todirname );
  // Search for all files
  snprintf( searchname, sizeof( searchname ), "%s/*", dirname );
  searchResult = fs_findFirstFile( dirname, NULL );
  if ( searchResult != NULL )
  {
    // Make the new directory
    net_copyFileToHost( dirname, todirname );

    // Copy each file
    while ( searchResult != NULL )
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      // Also avoid copying directories in general.
      snprintf( fromname, sizeof( fromname ), "%s/%s", dirname, searchResult );
      if ( searchResult[0] == '.' || fs_fileIsDirectory( fromname ) )
      {
        searchResult = fs_findNextFile();
        continue;
      }

      snprintf( fromname, sizeof( fromname ), "%s/%s", dirname, searchResult );
      snprintf( toname, sizeof( toname ), "%s/%s", todirname, searchResult );

      net_copyFileToHost( fromname, toname );
      searchResult = fs_findNextFile();
    }
  }

  fs_findClose();
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToAllPlayers( char *dirname, char *todirname )
{
  // ZZ> This function copies all files in a directory
  char searchname[128];
  char fromname[128];
  char toname[128];
  const char *searchResult;

  log_info( "net_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname );
  // Search for all files
  snprintf( searchname, sizeof( searchname ), "%s/*.*", dirname );
  searchResult = fs_findFirstFile( dirname, NULL );
  if ( searchResult != NULL )
  {
    // Make the new directory
    net_copyFileToAllPlayers( dirname, todirname );

    // Copy each file
    while ( searchResult != NULL )
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      if ( searchResult[0] == '.' )
      {
        searchResult = fs_findNextFile();
        continue;
      }

      snprintf( fromname, sizeof( fromname ), "%s/%s", dirname, searchResult );
      snprintf( toname, sizeof( toname ), "%s/%s", todirname, searchResult );
      net_copyFileToAllPlayers( fromname, toname );

      searchResult = fs_findNextFile();
    }
  }
  fs_findClose();
}

//--------------------------------------------------------------------------------------------
void net_sayHello()
{
  // ZZ> This function lets everyone know we're here
  if ( CData.network_on )
  {
    if ( hostactive )
    {
      log_info( "net_sayHello: Server saying hello.\n" );
      playersloaded++;
      if ( playersloaded >= GNet.num_player )
      {
        waitingforplayers = bfalse;
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
  else
  {
    waitingforplayers = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
bool_t net_handlePacket( ENetEvent *event )
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  FILE *file;
  size_t fileSize;
  bool_t retval = bfalse;

  log_info( "net_handlePacket: Received " );

  if ( cl_handlePacket( &AClientState, event ) ) return btrue;
  if ( net_amHost && sv_handlePacket( &AServerState, event ) ) return btrue;

  log_info( "net_handlePacket: Processing " );

  packet_startReading( event->packet );
  header = packet_readUnsignedShort();
  switch ( header )
  {
    case TO_ANY_TEXT:
      log_info( "TO_ANY_TEXT\n" );
      packet_readString( filename, 255 );
      debug_message( 1, filename );
      retval = btrue;
      break;

    case NET_TRANSFER_FILE:
      packet_readString( filename, 256 );
      fileSize = packet_readUnsignedInt();

      log_info( "NET_TRANSFER_FILE: %s with size %d.\n", filename, fileSize );

      // Try and save the file
      file = fs_fileOpen( PRI_NONE, NULL, filename, "wb" );
      if ( file != NULL )
      {
        fwrite( net_readPacket->data + net_readLocation, 1, fileSize, file );
        fs_fileClose( file );
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
      retval = btrue;
      break;

    case NET_TRANSFER_OK:
      log_info( "NET_TRANSFER_OK. The last file sent was successful.\n" );
      net_waitingForXferAck = 0;
      net_numFileTransfers--;
      retval = btrue;
      break;

    case NET_CREATE_DIRECTORY:
      packet_readString( filename, 256 );
      log_info( "NET_CREATE_DIRECTORY: %s\n", filename );

      fs_createDirectory( filename );

      // Acknowledge that we got this file
      net_startNewPacket();
      packet_addUnsignedShort( NET_TRANSFER_OK );
      net_sendPacketToPeer( event->peer );

      numfile++; // The client considers directories it sends to be files, so ya.
      retval = btrue;
      break;

    case NET_DONE_SENDING_FILES:
      log_info( "NET_DONE_SENDING_FILES\n" );
      numplayerrespond++;
      retval = btrue;
      break;

    case NET_NUM_FILES_TO_SEND:
      log_info( "NET_NUM_FILES_TO_SEND\n" );
      numfileexpected = ( int ) packet_readUnsignedShort();
      retval = btrue;
      break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void net_initialize()
{
  // ZZ> This starts up the network and logs whatever goes on
  serviceon = bfalse;
  GNet.num_session = 0;
  GNet.num_service = 0;

  // Clear all the state variables to 0 to start.
  memset( net_playerPeers, 0, sizeof( ENetPeer* ) * MAXPLAYER );
  memset( net_playerInfo, 0, sizeof( NetPlayerInfo ) * MAXPLAYER );
  memset( gPacket.buffer, 0, MAXSENDSIZE );
  memset( net_transferStates, 0, sizeof( NetFileTransfer ) * NET_MAX_FILE_TRANSFERS );
  memset( &net_receiveState, 0, sizeof( NetFileTransfer ) );

  if ( CData.network_on )
  {
    // initialize enet
    log_info( "net_initialize: Initializing enet..." );
    if ( enet_initialize() != 0 )
    {
      log_message( "Failed!\n" );
      CData.network_on = bfalse;
      serviceon = bfalse;
    }
    else
    {
      log_message( "Succeeded!\n" );
      serviceon = btrue;
      GNet.num_service = 1;
    }
  }
  else
  {
    // We're not doing GNet.working this time...
    log_info( "net_initialize: Networking not enabled.\n" );
  }
}

//--------------------------------------------------------------------------------------------
void net_shutDown()
{
  if( !CData.network_on ) return;

  log_info( "net_shutDown: Turning off GNet.working.\n" );
  enet_deinitialize();
}

//--------------------------------------------------------------------------------------------
int  net_pendingFileTransfers()
{
  return net_numFileTransfers;
}

//--------------------------------------------------------------------------------------------
Uint8 *transferBuffer = NULL;
size_t   transferSize = 0;

void net_updateFileTransfers()
{
  NetFileTransfer *state;
  ENetPacket *packet;
  size_t nameLen, fileSize;
  Uint32 networkSize;
  FILE *file;
  char *p;

  // Are there any pending file sends?
  if ( net_numFileTransfers > 0 )
  {
    if ( !net_waitingForXferAck )
    {
      state = &net_transferStates[net_fileTransferHead];

      // Check and see if this is a directory, instead of a file
      if ( fs_fileIsDirectory( state->sourceName ) )
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
        file = fs_fileOpen( PRI_NONE, NULL, state->sourceName, "rb" );
        if ( file )
        {
          log_info( "net_updateFileTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName );

          fseek( file, 0, SEEK_END );
          fileSize = ftell( file );
          fseek( file, 0, SEEK_SET );

          // Make room for the file's name
          nameLen = strlen( state->destName ) + 1;
          transferSize = nameLen;

          // And for the file's size
          transferSize += 6; // Uint32 size, and Uint16 message type
          transferSize += fileSize;

          transferBuffer = malloc( transferSize );
          * ( Uint16* ) transferBuffer = ENET_HOST_TO_NET_16( NET_TRANSFER_FILE );

          // Add the string and file length to the buffer
          p = ( char * )( transferBuffer + 2 );
          strcpy( p, state->destName );
          p += nameLen;

          networkSize = ENET_HOST_TO_NET_32(( u_long ) fileSize );
          * ( size_t* ) p = networkSize;
          p += 4;

          fread( p, 1, fileSize, file );
          fs_fileClose( file );

          packet = enet_packet_create( transferBuffer, transferSize, ENET_PACKET_FLAG_RELIABLE );
          enet_peer_send( state->target, NET_GUARANTEED_CHANNEL, packet );

          FREE( transferBuffer );
          transferSize = 0;

          net_waitingForXferAck = 1;
        }
        else
        {
          log_warning( "net_updateFileTransfers: Could not open file %s to send it!\n", state->sourceName );
        }
      }

      // update transfer queue state
      memset( state, 0, sizeof( NetFileTransfer ) );
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
//--------------------------------------------------------------------------------------------
void close_session()
{
  size_t i, numPeers;
  ENetEvent event;

  // ZZ> This function gets the computer out of a network game
  if ( !CData.network_on ) return;

  if ( net_amHost )
  {
    // Disconnect the peers
    numPeers = net_myHost->peerCount;
    for ( i = 0;i < numPeers;i++ )
    {
#ifdef ENET11
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

      }
    }

    // Forcefully disconnect any peers leftover
    for ( i = 0;i < net_myHost->peerCount;i++ )
    {
      enet_peer_reset( &net_myHost->peers[i] );
    }
  }

  log_info( "close_session: Disconnecting from network.\n" );

  if ( NULL != net_myHost )
  {
    enet_host_destroy( net_myHost );
    net_myHost = NULL;
  };

  net_gameHost = NULL;
}

//--------------------------------------------------------------------------------------------
int add_player( CHR_REF character, Uint16 player, Uint8 device )
{
  // ZZ> This function adds a player, returning bfalse if it fails, btrue otherwise

  if ( player >= MAXPLAYER || PlaList[player].valid ) return bfalse;

  ChrList[character].isplayer = btrue;
  PlaList[player].chr = character;
  PlaList[player].valid = btrue;
  PlaList[player].device = device;
  if ( device != INBITS_NONE )  nolocalplayers = bfalse;
  PlaList[player].latch.x = 0;
  PlaList[player].latch.y = 0;
  PlaList[player].latch.b = 0;

  cl_resetTimeLatches( &AClientState, character );
  sv_resetTimeLatches( &AServerState, character );

  if ( INBITS_NONE != device )
  {
    ChrList[character].islocalplayer = btrue;
    numlocalpla++;
  }
  numpla++;
  return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_messages()
{
  // ZZ> This function empties the message buffer
  int cnt;

  cnt = 0;
  while ( cnt < MAXMESSAGE )
  {
    GMsg.time[cnt] = 0;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void check_add( Uint8 key, char bigletter, char littleletter )
{
  // ZZ> This function adds letters to the net message
  /*PORT
  if(SDLKEYDOWN(key))
  {
  if(!keypress[key])
  {
  keypress[key] = btrue;
  if(GNet.messagewrite < MESSAGESIZE-2)
  {
  if(SDLKEYDOWN(SDLK_LSHIFT) || SDLKEYDOWN(SDLK_RSHIFT))
  {
  GNet.message[GNet.messagewrite] = bigletter;
  }
  else
  {
  GNet.message[GNet.messagewrite] = littleletter;
  }
  GNet.messagewrite++;
  GNet.message[GNet.messagewrite] = '?'; // The flashing input cursor
  GNet.message[GNet.messagewrite+1] = 0;
  }
  }
  }
  else
  {
  keypress[key] = bfalse;
  }
  */
}

//--------------------------------------------------------------------------------------------
void input_net_message()
{
  // ZZ> This function lets players communicate over network by hitting return, then
  //     typing text, then return again
  /*PORT
  int cnt;
  char cTmp;


  if(GNet.messagemode)
  {
  // Add new letters
  check_add(DIK_A, 'A', 'a');
  check_add(DIK_B, 'B', 'b');
  check_add(DIK_C, 'C', 'c');
  check_add(DIK_D, 'D', 'd');
  check_add(DIK_E, 'E', 'e');
  check_add(DIK_F, 'F', 'f');
  check_add(DIK_G, 'G', 'g');
  check_add(DIK_H, 'H', 'h');
  check_add(DIK_I, 'I', 'i');
  check_add(DIK_J, 'J', 'j');
  check_add(DIK_K, 'K', 'k');
  check_add(DIK_L, 'L', 'l');
  check_add(DIK_M, 'M', 'm');
  check_add(DIK_N, 'N', 'n');
  check_add(DIK_O, 'O', 'o');
  check_add(DIK_P, 'P', 'p');
  check_add(DIK_Q, 'Q', 'q');
  check_add(DIK_R, 'R', 'r');
  check_add(DIK_S, 'S', 's');
  check_add(DIK_T, 'T', 't');
  check_add(DIK_U, 'U', 'u');
  check_add(DIK_V, 'V', 'v');
  check_add(DIK_W, 'W', 'w');
  check_add(DIK_X, 'X', 'x');
  check_add(DIK_Y, 'Y', 'y');
  check_add(DIK_Z, 'Z', 'z');


  check_add(DIK_1, '!', '1');
  check_add(DIK_2, '@', '2');
  check_add(DIK_3, '#', '3');
  check_add(DIK_4, '$', '4');
  check_add(DIK_5, '%', '5');
  check_add(DIK_6, '^', '6');
  check_add(DIK_7, '&', '7');
  check_add(DIK_8, '*', '8');
  check_add(DIK_9, '(', '9');
  check_add(DIK_0, ')', '0');


  check_add(DIK_APOSTROPHE, 34, 39);
  check_add(DIK_SPACE,      ' ', ' ');
  check_add(DIK_SEMICOLON,  ':', ';');
  check_add(DIK_PERIOD,     '>', '.');
  check_add(DIK_COMMA,      '<', ',');
  check_add(DIK_GRAVE,      '`', '`');
  check_add(DIK_MINUS,      '_', '-');
  check_add(DIK_EQUALS,     '+', '=');
  check_add(DIK_LBRACKET,   '{', '[');
  check_add(DIK_RBRACKET,   '}', ']');
  check_add(DIK_BACKSLASH,  '|', '\\');
  check_add(DIK_SLASH,      '?', '/');



  // Make cursor flash
  if(GNet.messagewrite < MESSAGESIZE-1)
  {
  if((wldframe & 8) == 0)
  {
  GNet.message[GNet.messagewrite] = '#';
  }
  else
  {
  GNet.message[GNet.messagewrite] = '+';
  }
  }


  // Check backspace and return
  if(GNet.messagedelay == 0)
  {
  if(SDLKEYDOWN(SDLK_BACK))
  {
  if(GNet.messagewrite < MESSAGESIZE)  GNet.message[GNet.messagewrite] = 0;
  if(GNet.messagewrite > GNet.messagewritemin) GNet.messagewrite--;
  GNet.messagedelay = 3;
  }


  // Ship out the message
  if(SDLKEYDOWN(SDLK_RETURN))
  {
  // Is it long enough to bother?
  if(GNet.messagewrite > 0)
  {
  // Yes, so send it
  GNet.message[GNet.messagewrite] = 0;
  if(CData.network_on)
  {
  start_building_packet();
  add_packet_us(TO_ANY_TEXT);
  add_packet_sz(GNet.message);
  send_packet_to_all_players();
  }
  }
  GNet.messagemode = bfalse;
  GNet.messagedelay = 20;
  }
  }
  else
  {
  GNet.messagedelay--;
  }
  }
  else
  {
  // Input a new message?
  if(GNet.messagedelay == 0)
  {
  if(SDLKEYDOWN(SDLK_RETURN))
  {
  // Copy the name
  cnt = 0;
  cTmp = CData.net_messagename[cnt];
  while(cTmp != 0 && cnt < 64)
  {
  GNet.message[cnt] = cTmp;
  cnt++;
  cTmp = CData.net_messagename[cnt];
  }
  GNet.message[cnt] = '>';  cnt++;
  GNet.message[cnt] = ' ';  cnt++;
  GNet.message[cnt] = '?';
  GNet.message[cnt+1] = 0;
  GNet.messagewrite = cnt;
  GNet.messagewritemin = cnt;

  GNet.messagemode = btrue;
  GNet.messagedelay = 20;
  }
  }
  else
  {
  GNet.messagedelay--;
  }
  }
  */
}


//--------------------------------------------------------------------------------------------
bool_t listen_for_packets()
{
  // ZZ> This function reads any new messages and sets the player latch and matrix needed
  //     lists...
  ENetEvent event;
  bool_t retval = bfalse;

  if ( !CData.network_on || NULL == net_myHost ) return bfalse;

  // Listen for new messages
  while ( 0 != enet_host_service( net_myHost, &event, 0 ) )
  {
    switch ( event.type )
    {
      case ENET_EVENT_TYPE_RECEIVE:
        if ( !net_handlePacket( &event ) )
        {
          log_warning( "listen_for_packets() - Unhandled packet\n" );
        }
        enet_packet_destroy( event.packet );
        retval = btrue;
        break;

      case ENET_EVENT_TYPE_CONNECT:
        // don't allow anyone to connect during the game session
        log_warning( "listen_for_packets: Client tried to connect during the game: %x:%u\n",
                     event.peer->address.host, event.peer->address.port );
#ifdef ENET11
        enet_peer_disconnect( event.peer, 0 );
#else
        enet_peer_disconnect( event.peer );
#endif
        retval = btrue;
        break;

      case ENET_EVENT_TYPE_DISCONNECT:
        // Is this a player disconnecting, or just a rejected connection
        // from above?
        if ( event.peer->data != 0 )
        {
          NetPlayerInfo *info = event.peer->data;

          // uh oh, how do we handle losing a player?
          log_warning( "listen_for_packets: Player %d disconnected!\n",
                       info->playerSlot );
        }
        retval = btrue;
        break;
    }
  }

  return retval;
}


//--------------------------------------------------------------------------------------------
void find_open_sessions()
{
  /*PORT
  // ZZ> This function finds some open games to join
  DPSESSIONDESC2      sessionDesc;
  HRESULT             hr;

  if(CData.network_on)
  {
  GNet.num_session = 0;
  if(globalnetworkerr)  fprintf(globalnetworkerr, "  Looking for open games...\n");
  ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
  sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
  sessionDesc.guidApplication = NETWORKID;
  hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
  if(globalnetworkerr)  fprintf(globalnetworkerr, "    %d sessions found\n", GNet.num_session);
  }
  */
}


//--------------------------------------------------------------------------------------------
void stop_players_from_joining()
{
  // ZZ> This function stops players from joining a game
}


////--------------------------------------------------------------------------------------------
//void turn_on_service(int service)
//{
//  // This function turns on a network service ( IPX, TCP, serial, modem )
//}

//--------------------------------------------------------------------------------------------
//void send_rts_order(int x, int y, Uint8 order, Uint8 target)
//{
//  // ZZ> This function asks the host to order the selected characters
//  /* Uint32 what, when, whichorder, cnt;
//
//   if(numrtsselect > 0)
//   {
//    x = (x >> 6) & 1023;
//    y = (y >> 6) & 1023;
//    what = (target << 24) | (x << 14) | (y << 4) | (order&15);
//    if(hostactive)
//    {
//     when = wldframe + CData.GOrder.lag;
//     whichorder = get_empty_order();
//     if(whichorder != MAXORDER)
//     {
//      // Add a new order on own machine
//      GOrder.when[whichorder] = when;
//      GOrder.what[whichorder] = what;
//      cnt = 0;
//      while(cnt < numrtsselect)
//      {
//       GOrder.who[whichorder][cnt] = GRTS.select[cnt];
//       cnt++;
//      }
//      while(cnt < MAXSELECT)
//      {
//       GOrder.who[whichorder][cnt] = MAXCHR;
//       cnt++;
//      }
//
//
//      // Send the order off to everyone else
//      if(CData.network_on)
//      {
//       net_startNewPacket();
//       packet_addUnsignedShort(TO_REMOTE_RTS);
//       cnt = 0;
//       while(cnt < MAXSELECT)
//       {
//        packet_addUnsignedByte(GOrder.who[whichorder][cnt]);
//        cnt++;
//       }
//       packet_addUnsignedInt(what);
//       packet_addUnsignedInt(when);
//       net_sendPacketToAllPlayersGuaranteed();
//      }
//     }
//    }
//    else
//    {
//     // Send the order off to the host
//     net_startNewPacket();
//     packet_addUnsignedShort(TO_HOST_RTS);
//     cnt = 0;
//     while(cnt < numrtsselect)
//     {
//      packet_addUnsignedByte(GRTS.select[cnt]);
//      cnt++;
//     }
//     while(cnt < MAXSELECT)
//     {
//      packet_addUnsignedByte(MAXCHR);
//      cnt++;
//     }
//     packet_addUnsignedInt(what);
//     net_sendPacketToHostGuaranteed();
//    }
//   }*/
//}
//
