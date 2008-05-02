/* Egoboo - Server.c
 * This code is not currently in use.
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

#include "Server.h"
#include "Network.h"
#include "Log.h"
#include "egoboo.h"
#include "enet.h"

ServerState AServerState;

void sv_frameStep()
{

}

//--------------------------------------------------------------------------------------------
void sv_bufferLatches( ServerState * ss )
{
  // ZZ> This function buffers the character latches
  Uint32 uiTime, ichr;

  if ( !hostactive ) return;

  if ( wldframe > STARTTALK )
  {
    uiTime = wldframe + CData.lag;

    // Now pretend the host got the packet...
    uiTime &= LAGAND;
    for ( ichr = 0; ichr < MAXPLAYER; ichr++ )
    {
      ss->timelatchbutton[ichr][uiTime] = ss->latchbutton[ichr];
      ss->timelatchx[ichr][uiTime]      = (( Sint32 )( ss->latchx[ichr] * SHORTLATCH ) ) / SHORTLATCH;
      ss->timelatchy[ichr][uiTime]      = (( Sint32 )( ss->latchy[ichr] * SHORTLATCH ) ) / SHORTLATCH;
    }
    ss->numplatimes++;
  };

};

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes( ServerState * ss )
{
  // ZZ> This function sends the character data to all the remote machines
  Uint32 uiTime, ichr;

  if ( !hostactive || !CData.network_on ) return;

  if ( wldframe > STARTTALK )
  {
    uiTime = wldframe + CData.lag;

    // Send a message to all players
    net_startNewPacket();
    packet_addUnsignedShort( TO_REMOTE_LATCH );                     // The message header
    packet_addUnsignedInt( uiTime );                                  // The stamp

    // Send all player latches...
    uiTime &= LAGAND;
    for ( ichr = 0; ichr < MAXCHR; ichr++ )
    {
      if ( ChrList[ichr].on )
      {
        packet_addUnsignedShort( ichr );                                 // The character index
        packet_addUnsignedByte( ss->timelatchbutton[ichr][uiTime] );       // Player button states
        packet_addSignedShort( ss->timelatchx[ichr][uiTime]*SHORTLATCH );  // Player motion
        packet_addSignedShort( ss->timelatchy[ichr][uiTime]*SHORTLATCH );  // Player motion
      }
    }

    // Send the packet
    net_sendPacketToAllPlayers();
  }
}

//--------------------------------------------------------------------------------------------
void sv_letPlayersJoin()
{
  // ZZ> This function finds all the players in the game
  ENetEvent event;
  char hostName[64];

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
        strncpy( GNet.playername[GNet.num_player], hostName, 16 );

        event.peer->data = & ( net_playerInfo[GNet.num_player] );
        GNet.num_player++;

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
    }
  }
}

//--------------------------------------------------------------------------------------------
int sv_hostGame()
{
  // ZZ> This function tries to host a new session
  ENetAddress address;

  if ( CData.network_on )
  {
    // Try to create a new session
    address.host = ENET_HOST_ANY;
    address.port = NET_EGOBOO_PORT;

    log_info( "sv_hostGame: Creating game on port %d\n", NET_EGOBOO_PORT );
    net_myHost = enet_host_create( &address, MAXPLAYER, 0, 0 );
    if ( net_myHost == NULL )
    {
      log_info( "sv_hostGame: Could not create network connection!\n" );
      return bfalse;
    }

    // Try to create a host player
//  return create_player(btrue);
    net_amHost = btrue;

    // Moved from net_sayHello because there they cause a race issue
    waitingforplayers = btrue;
    playersloaded = 0;
  }
  // Run in solo mode
  return btrue;
}

bool_t sv_handlePacket( ServerState * ss, ENetEvent *event )
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  CHR_REF character;
  FILE *file;

  bool_t retval = bfalse;

  // do some error trapping
  if ( !hostactive ) return bfalse;

  // send some log info
  log_info( "sv_handlePacket: Processing " );

  // rewind the packet
  packet_startReading( event->packet );
  header = packet_readUnsignedShort();

  // process out messages
  switch ( header )
  {

    case TO_HOST_MODULEOK:
      log_info( "TO_HOSTMODULEOK\n" );

      playersready++;
      if ( playersready >= GNet.num_player )
      {
        readytostart = btrue;
      }
      retval = btrue;
      break;

    case TO_HOST_LATCH:
      log_info( "TO_HOST_LATCH\n" );
      while ( packet_remainingSize() > 0 )
      {
        character = packet_readUnsignedShort();
        ss->latchbutton[character] = packet_readUnsignedByte();
        ss->latchx[character]      = ( float ) packet_readSignedShort() / ( float ) SHORTLATCH;
        ss->latchy[character]      = ( float ) packet_readSignedShort() / ( float ) SHORTLATCH;
      }
      retval = btrue;
      break;

    case TO_HOST_IM_LOADED:
      log_info( "TO_HOST_IMLOADED\n" );

      playersloaded++;
      if ( playersloaded == GNet.num_player )
      {
        // Let the games begin...
        waitingforplayers = bfalse;
        net_startNewPacket();
        packet_addUnsignedShort( TO_REMOTE_START );
        net_sendPacketToAllPlayersGuaranteed();
      }
      retval = btrue;
      break;

    case TO_HOST_RTS:
      log_info( "TO_HOST_RTS\n" );

      /*whichorder = get_empty_order();
      if(whichorder < MAXORDER)
      {
      // Add the order on the host machine
      cnt = 0;
      while(cnt < MAXSELECT)
      {
      who = packet_readUnsignedByte();
      GOrder.who[whichorder][cnt] = who;
      cnt++;
      }
      what = packet_readUnsignedInt();
      when = wldframe + CData.GOrder.lag;
      GOrder.what[whichorder] = what;
      GOrder.when[whichorder] = when;


      // Send the order off to everyone else
      net_startNewPacket();
      packet_addUnsignedShort(TO_REMOTE_RTS);
      cnt = 0;
      while(cnt < MAXSELECT)
      {
      packet_addUnsignedByte(GOrder.who[whichorder][cnt]);
      cnt++;
      }
      packet_addUnsignedInt(what);
      packet_addUnsignedInt(when);
      net_sendPacketToAllPlayersGuaranteed();
      }*/
      retval = btrue;
      break;

    case TO_HOST_FILE:
      log_info( "TO_HOST_FILE\n" );
      packet_readString( filename, 255 );
      newfilesize = packet_readUnsignedInt();

      // Change the size of the file if need be
      newfile = 0;
      file = fs_fileOpen( PRI_NONE, NULL, filename, "rb" );
      if ( file )
      {
        fseek( file, 0, SEEK_END );
        filesize = ftell( file );
        fs_fileClose( file );

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
        file = fs_fileOpen( PRI_NONE, NULL, filename, "wb" );
        if ( file )
        {
          filesize = 0;
          while ( filesize < newfilesize )
          {
            fputc( 0, file );
            filesize++;
          }
          fs_fileClose( file );
        }
      }

      // Go to the position in the file and copy data
      fileposition = packet_readUnsignedInt();
      file = fs_fileOpen( PRI_NONE, NULL, filename, "r+b" );

      if ( fseek( file, fileposition, SEEK_SET ) == 0 )
      {
        while ( packet_remainingSize() > 0 )
        {
          fputc( packet_readUnsignedByte(), file );
        }
      }
      fs_fileClose( file );
      retval = btrue;
      break;

    case TO_HOST_DIR:
      log_info( "TO_HOST_DIR\n" );
      packet_readString( filename, 255 );
      fs_createDirectory( filename );
      retval = btrue;
      break;

    case TO_HOST_FILESENT:
      log_info( "TO_HOST_FILESENT\n" );
      numfileexpected += packet_readUnsignedInt();
      numplayerrespond++;
      retval = btrue;
      break;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
void sv_unbufferLatches( ServerState * ss )
{
  // ZZ> This function sets character latches based on player input to the host
  Uint32 cnt, uiTime;

  if ( !hostactive ) return;

  // Copy the latches
  uiTime = wldframe & LAGAND;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) ) continue;
    if ( !ss->timelatchvalid[cnt][uiTime] ) continue;
    if ( INVALID_TIMESTAMP == ss->timelatchstamp[cnt][uiTime] ) continue;

    ss->latchx[cnt]      = ss->timelatchx[cnt][uiTime];
    ss->latchy[cnt]      = ss->timelatchy[cnt][uiTime];
    ss->latchbutton[cnt] = ss->timelatchbutton[cnt][uiTime];

    // Let players respawn
    if ( respawnvalid && HAS_SOME_BITS( ChrList[cnt].aistate.latch.b, LATCHBUTTON_RESPAWN ) )
    {
      if ( !ChrList[cnt].alive )
      {
        respawn_character( cnt );
        TeamList[ChrList[cnt].team].leader = cnt;
        ChrList[cnt].aistate.alert |= ALERT_CLEANEDUP;

        // Cost some experience for doing this...  Never lose a level
        ChrList[cnt].experience *= EXPKEEP;
      }
      ss->latchbutton[cnt] &= ~LATCHBUTTON_RESPAWN;
    }

  }
  ss->numplatimes--;
}

//--------------------------------------------------------------------------------------------
void sv_reset( ServerState * ss )
{
  int cnt;
  if ( NULL == ss ) return;

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    sv_resetTimeLatches( ss, cnt );
  };

  ss->nexttimestamp = INVALID_TIMESTAMP;
  ss->numplatimes   = STARTTALK + 1;
};

//--------------------------------------------------------------------------------------------
void sv_resetTimeLatches( ServerState * ss, Sint32 ichr )
{
  int cnt;

  if ( NULL == ss ) return;
  if ( ichr < 0 || ichr >= MAXCHR ) return;

  ss->latchx[ichr]      = 0;
  ss->latchy[ichr]      = 0;
  ss->latchbutton[ichr] = 0;

  for ( cnt = 0; cnt < MAXLAG; cnt++ )
  {
    ss->timelatchvalid[ichr][cnt]  = bfalse;
    ss->timelatchstamp[ichr][cnt]  = INVALID_TIMESTAMP;
    ss->timelatchx[ichr][cnt]      = 0;
    ss->timelatchy[ichr][cnt]      = 0;
    ss->timelatchbutton[ichr][cnt] = 0;
  }
};