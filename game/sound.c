/* Egoboo - sound.c
 * Sound code in Egoboo is implemented using SDL_mixer.
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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "log.h"

//--------------------------------------------------------------------------------------------
void load_global_waves( char *modname )
{
  // ZZ> This function loads the global waves
  char tmploadname[256];
  char newloadname[256];
  STRING wavename;
  Uint8 cnt = 0;

  if(!soundvalid) return;

  //First load weather sounds
  make_newloadname( modname, ( "gamedat" SLASH_STR ), tmploadname );
  while ( cnt < MAXWAVE )
  {
    snprintf( wavename, sizeof(wavename), "sound%d.ogg", cnt );
    make_newloadname( tmploadname, wavename, newloadname );
    if((globalwave[cnt] = Mix_LoadWAV( newloadname )) == NULL)
	{
		snprintf( wavename, sizeof(wavename), "sound%d.wav", cnt );
		make_newloadname( tmploadname, wavename, newloadname );
	}
    cnt++;
  }

  // These sounds are always standard
  if((globalwave[SND_GETCOIN] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "coinget.ogg" )) == NULL)
      globalwave[SND_GETCOIN] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "coinget.wav" );
  if((globalwave[SND_DEFEND] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.ogg" )) == NULL)
      globalwave[SND_DEFEND] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.wav" );
  if((globalwave[SND_COINFALL] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "coinfall.ogg" )) == NULL)
      globalwave[SND_COINFALL] = Mix_LoadWAV( "basicdat" SLASH_STR "globalparticles" SLASH_STR "coinfall.wav" );
  if((globalwave[SND_LEVELUP] = Mix_LoadWAV( "basicdat" SLASH_STR "levelup.ogg" )) == NULL)
      globalwave[SND_LEVELUP] = Mix_LoadWAV( "basicdat" SLASH_STR "levelup.wav" );

  /*  The Global Sounds
  *  0  - Pick up coin
  *  1  - Defend clank
  *  2  - Weather Effect
  *  3  - Hit Water (Splash)
  *  4  - Coin falls on ground

  // These new values todo should determine sound and particle effects
  Weather Type: DROPS, RAIN, SNOW
  Water Type: LAVA, WATER, DARK
  */
}

// This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
  if ( ( musicvalid || soundvalid ) && !mixeron )
  {
    log_info( "Initializing SDL_mixer audio services... " );
    Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize );
    Mix_VolumeMusic( musicvolume );
    Mix_AllocateChannels( maxsoundchannel );
    if ( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize ) != 0 )
    {
      log_error( "Unable to initialize audio: %s\n", Mix_GetError() );
    }
	else log_message("Success!\n");
  }
  return mixeron;
}

//------------------------------------
// SOUND-------------------------------
//------------------------------------
int play_sound( float xpos, float ypos, Mix_Chunk *loadedwave )
{
  // This function plays a specified sound
  if ( soundvalid )
  {
    int distance, volume, pan;
    distance = SQRT( POW( ABS( camtrackx - xpos ), 2 ) + POW( ABS( camtracky - ypos ), 2 ) ); // Ugly, but just the distance formula
    if ( listening ) distance *= 0.66f;
    volume = ( ( distance / VOLUMERATIO ) * ( 1 + ( soundvolume / 100 ) ) ); // adjust volume with ratio and sound volume
    pan = 57.3f * ( ( 1.5f * PI ) - ATAN2( camy - ypos, camx - xpos ) - camturnleftright ); // Convert the camera angle to the nearest integer degree
    if ( pan < 0 ) pan += 360;
    if ( pan > 360 ) pan -= 360;
    if ( volume < 255 )
    {
      if ( loadedwave == NULL )
      {
        log_warning( "Sound file not correctly loaded (Not found?).\n" );
      }

      channel = Mix_PlayChannel( -1, loadedwave, 0 );
      if ( channel != -1 )
      {
        Mix_SetDistance( channel, volume );
        if ( pan < 180 )
        {
          if ( pan < 90 ) Mix_SetPanning( channel, 255 - ( pan * 2.83f ), 255 );
          else Mix_SetPanning( channel, 255 - ( ( 180 - pan ) * 2.83f ), 255 );
        }
        else
        {
          if ( pan < 270 ) Mix_SetPanning( channel, 255, 255 - ( ( pan - 180 ) * 2.83f ) );
          else Mix_SetPanning( channel, 255, 255 - ( ( 360 - pan ) * 2.83f ) );
        }
      }
      else log_warning( "All sound channels are currently in use. Sound is NOT playing.\n" );
    }
  }
  return channel;
}

// TODO:
void stop_sound( int whichchannel )
{
  if ( soundvalid ) Mix_HaltChannel( whichchannel );
}

//------------------------------------------------------------------------------
// Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
void load_all_music_sounds()
{
  // This function loads all of the music sounds
  char loadpath[128];
  char songname[128];
  FILE *playlist;
  int cnt;

  // Open the playlist listing all music files
  playlist = fopen( "basicdat" SLASH_STR "music" SLASH_STR "playlist.txt", "r" );
  if ( playlist == NULL )
  {
    log_warning( "Error opening playlist.txt\n" );
    return;
  }

  // Load the music data into memory
  if ( musicvalid && !musicinmemory )
  {
    cnt = 0;
    while ( cnt < MAXPLAYLISTLENGHT && !feof( playlist ) )
    {
      goto_colon_yesno( playlist );
      fscanf( playlist, "%s", songname );
      sprintf( loadpath, ( "basicdat" SLASH_STR "music" SLASH_STR "%s" ), songname );
      musictracksloaded[cnt] = Mix_LoadMUS( loadpath );
      cnt++;
    }
    musicinmemory = btrue;
  }
  fclose( playlist );
}

void play_music( Sint8 songnumber, Uint16 fadetime, Sint8 loops )
{
  // This functions plays a specified track loaded into memory
  if ( songplaying != songnumber && musicvalid )
  {
    // Mix_FadeOutMusic(fadetime);      // Stops the game too
    Mix_PlayMusic( musictracksloaded[songnumber], loops );
    songplaying = songnumber;
  }
}

void stop_music()
{
  // This function sets music track to pause
  if ( musicvalid )
  {
    Mix_HaltMusic();
  }
}
