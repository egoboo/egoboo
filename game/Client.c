/* Egoboo - Client.c
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

#include "Client.h"
#include "Network.h"
#include "Log.h"
#include "egoboo.h"
#include "enet.h"
#include "input.h"

// Global ClientState instance
ClientState AClientState;

//--------------------------------------------------------------------------------------------
void cl_frameStep()
{

}


//--------------------------------------------------------------------------------------------
int cl_joinGame( const char* hostname )
{
  // ZZ> This function tries to join one of the sessions we found
  ENetAddress address;
  ENetEvent event;

  if ( CData.network_on )
  {
    log_info( "cl_joinGame: Creating client network connection... " );
    // Create my host thingamabober
    // TODO: Should I limit client bandwidth here?
    net_myHost = enet_host_create( NULL, 1, 0, 0 );
    if ( net_myHost == NULL )
    {
      // can't create a network connection at all
      log_message( "Failed!\n" );
      return bfalse;
    }
    log_message( "Succeeded!\n" );

    // Now connect to the remote host
    log_info( "cl_joinGame: Attempting to connect to %s:%d\n", hostname, NET_EGOBOO_PORT );
    enet_address_set_host( &address, hostname );
    address.port = NET_EGOBOO_PORT;
    net_gameHost = enet_host_connect( net_myHost, &address, NET_EGOBOO_NUM_CHANNELS );
    if ( net_gameHost == NULL )
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
void cl_talkToHost( ClientState * cs )
{
  // ZZ> This function sends the latch packets to the host machine
  Uint8 player;

  // Start talkin'
  if ( CData.network_on && wldframe > STARTTALK && !hostactive && numlocalpla > 0 )
  {
    Uint32 ichr;
    Uint32 time = ( wldframe + 1 ) & LAGAND;


    net_startNewPacket();
    packet_addUnsignedShort( TO_HOST_LATCH );   // The message header

    for ( player = 0; player < MAXPLAYER; player++ )
    {
      // Find the local players
      if ( !VALID_PLA( player ) || INBITS_NONE == PlaList[player].device ) continue;

      ichr = pla_get_character( player );
      if ( VALID_CHR( ichr ) )
      {
        packet_addUnsignedShort( ichr );                                 // The character index
        packet_addUnsignedByte( cs->timelatchbutton[ichr][time] );       // Player button states
        packet_addSignedShort( cs->timelatchx[ichr][time]*SHORTLATCH );  // Player motion
        packet_addSignedShort( cs->timelatchy[ichr][time]*SHORTLATCH );  // Player motion
      }
    }

    // Send it to the host
    net_sendPacketToHost();
  }
}

//--------------------------------------------------------------------------------------------
void cl_unbufferLatches( ClientState * cs )
{
  // ZZ> This function sets character latches based on player input to the host
  int    cnt;
  Uint32 uiTime, stamp;
  Sint32 dframes;

  // Copy the latches
  stamp = wldframe;
  uiTime  = stamp & LAGAND;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) ) continue;
    if ( !cs->timelatchvalid[cnt][uiTime] ) continue;
    if ( INVALID_TIMESTAMP == cs->timelatchstamp[cnt][uiTime] ) continue;

    dframes = ( float )( cs->timelatchvalid[cnt][uiTime] - stamp );

    // copy the data over
    ChrList[cnt].aistate.latch.x      = cs->timelatchx[cnt][uiTime];
    ChrList[cnt].aistate.latch.y      = cs->timelatchy[cnt][uiTime];
    ChrList[cnt].aistate.latch.b = cs->timelatchbutton[cnt][uiTime];

    // set the data to invalid
    cs->timelatchvalid[cnt][uiTime] = bfalse;
    cs->timelatchstamp[cnt][uiTime] = INVALID_TIMESTAMP;
  }
  cs->numplatimes--;
}



//--------------------------------------------------------------------------------------------
bool_t cl_handlePacket( ClientState * cs, ENetEvent *event )
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  Uint32 stamp;
  int uiTime;
  FILE *file;
  bool_t retval = bfalse;

  // do some error trapping
  if ( hostactive ) return bfalse;

  // send some log info
  log_info( "cl_handlePacket: Processing " );

  // rewind the packet
  packet_startReading( event->packet );
  header = packet_readUnsignedShort();

  // process our messages
  switch ( header )
  {
    case TO_REMOTE_FILESENT:
      log_info( "TO_REMOTE_FILESENT\n" );

      numfileexpected += packet_readUnsignedInt();
      numplayerrespond++;

      retval = btrue;
      break;

    case TO_REMOTE_MODULE:
      log_info( "TO_REMOTE_MODULE\n" );
      if ( !readytostart )
      {
        seed = packet_readUnsignedInt();
        packet_readString( filename, 255 );
        strcpy( pickedmodule, filename );

        // Check to see if the module exists
        pickedindex = find_module( pickedmodule );
        if ( pickedindex == -1 )
        {
          // The module doesn't exist locally
          // !!!BAD!!!  Copy the data from the host
          pickedindex = 0;
        }

        // Make ourselves ready
        readytostart = btrue;

        // Tell the host we're ready
        net_startNewPacket();
        packet_addUnsignedShort( TO_HOST_MODULEOK );
        net_sendPacketToHostGuaranteed();
      }
      retval = btrue;
      break;

    case TO_REMOTE_START:
      log_info( "TO_REMOTE_START\n" );

      waitingforplayers = bfalse;

      retval = btrue;
      break;

    case TO_REMOTE_RTS:
      log_info( "TO_REMOTE_RTS\n" );

      /*  whichorder = get_empty_order();
      if(whichorder < MAXORDER)
      {
      // Add the order on the remote machine
      cnt = 0;
      while(cnt < MAXSELECT)
      {
      who = packet_readUnsignedByte();
      GOrder.who[whichorder][cnt] = who;
      cnt++;
      }
      what = packet_readUnsignedInt();
      when = packet_readUnsignedInt();
      GOrder.what[whichorder] = what;
      GOrder.when[whichorder] = when;
      }*/

      retval = btrue;
      break;

    case TO_REMOTE_FILE:
      log_info( "TO_REMOTE_FILE\n" );

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
      if ( file )
      {
        if ( fseek( file, fileposition, SEEK_SET ) == 0 )
        {
          while ( packet_remainingSize() > 0 )
          {
            fputc( packet_readUnsignedByte(), file );
          }
        }
        fs_fileClose( file );
      }

      retval = btrue;
      break;

    case TO_REMOTE_DIR:
      log_info( "TO_REMOTE_DIR\n" );

      packet_readString( filename, 255 );
      fs_createDirectory( filename );

      retval = btrue;
      break;

    case TO_REMOTE_LATCH:
      log_info( "TO_REMOTE_LATCH\n" );

      stamp = packet_readUnsignedInt();
      uiTime = stamp & LAGAND;
      if ( INVALID_TIMESTAMP == cs->nexttimestamp )
      {
        cs->nexttimestamp = stamp;
      }
      if ( stamp < cs->nexttimestamp )
      {
        log_warning( "net_handlePacket: OUT OF ORDER PACKET\n" );
        outofsync = btrue;
      }
      if ( stamp <= wldframe )
      {
        log_warning( "net_handlePacket: LATE PACKET\n" );
        outofsync = btrue;
      }
      if ( stamp > cs->nexttimestamp )
      {
        log_warning( "net_handlePacket: MISSED PACKET\n" );
        cs->nexttimestamp = stamp;  // Still use it
        outofsync = btrue;
      }
      if ( stamp == cs->nexttimestamp )
      {
        CHR_REF ichr;

        // Remember that we got it
        cs->numplatimes++;

        for ( ichr = 0; ichr < MAXCHR; ichr++ )
        {
          cs->timelatchstamp[ichr][uiTime] = INVALID_TIMESTAMP;
          cs->timelatchvalid[ichr][uiTime] = bfalse;
        };

        // Read latches for each player sent
        while ( packet_remainingSize() > 0 )
        {
          ichr = packet_readUnsignedShort();
          cs->timelatchstamp[ichr][uiTime]  = stamp;
          cs->timelatchvalid[ichr][uiTime]  = btrue;
          cs->timelatchbutton[ichr][uiTime] = packet_readUnsignedByte();
          cs->timelatchx[ichr][uiTime]      = ( float ) packet_readSignedShort() / ( float ) SHORTLATCH;
          cs->timelatchy[ichr][uiTime]      = ( float ) packet_readSignedShort() / ( float ) SHORTLATCH;
        };

        cs->nexttimestamp = stamp + 1;
      }

      retval = btrue;
      break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void cl_reset( ClientState * cs )
{
  int cnt;
  if ( NULL == cs ) return;

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    cl_resetTimeLatches( cs, cnt );
  };

  cs->nexttimestamp = INVALID_TIMESTAMP;
  cs->numplatimes   = STARTTALK + 1;
};


//--------------------------------------------------------------------------------------------
void cl_resetTimeLatches( ClientState * cs, Sint32 ichr )
{
  int cnt;

  if ( NULL == cs ) return;
  if ( !VALID_CHR( ichr ) ) return;

  for ( cnt = 0; cnt < MAXLAG; cnt++ )
  {
    cs->timelatchvalid[ichr][cnt]  = bfalse;
    cs->timelatchstamp[ichr][cnt]  = INVALID_TIMESTAMP;
    cs->timelatchx[ichr][cnt]      = 0;
    cs->timelatchy[ichr][cnt]      = 0;
    cs->timelatchbutton[ichr][cnt] = 0;
  }
};

//--------------------------------------------------------------------------------------------
void cl_bufferLatches( ClientState * cs )
{
  // ZZ> This function buffers the player data
  Uint32 player, stamp, uiTime, ichr;

  stamp = wldframe + 1;
  uiTime = stamp & LAGAND;

  for ( player = 0; player < MAXPLAYER; player++ )
  {
    if ( !VALID_PLA( player ) ) continue;

    ichr = pla_get_character( player );

    if ( VALID_CHR( ichr ) )
    {
      cs->timelatchvalid[ichr][uiTime]  = btrue;
      cs->timelatchstamp[ichr][uiTime]  = stamp;
      cs->timelatchbutton[ichr][uiTime] = PlaList[player].latch.b;
      cs->timelatchx[ichr][uiTime]      = (( Sint32 )( PlaList[player].latch.x * SHORTLATCH ) ) / SHORTLATCH;
      cs->timelatchy[ichr][uiTime]      = (( Sint32 )( PlaList[player].latch.y * SHORTLATCH ) ) / SHORTLATCH;
    }
  }

  cs->numplatimes++;

};