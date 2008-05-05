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
along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sound.h"
#include "Log.h"
#include "camera.h"
#include "egoboo_utility.h"
#include "char.h"
#include "particle.h"
#include "egoboo.h"

bool_t mixeron       = bfalse;    // Is the SDL_Mixer loaded?
bool_t musicinmemory = bfalse;    // Is the music loaded in memory?
int    songplaying   = -1;        // Current song that is playing

Mix_Music * instrumenttosound[MAXPLAYLISTLENGTH]; //This is a specific music file loaded into memory
Mix_Chunk * globalwave[MAXWAVE];                  //All sounds loaded into memory


//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
  if (( CData.musicvalid || CData.soundvalid ) && !mixeron )
  {
    log_info( "Initializing SDL_mixer audio services version %i.%i.%i... ", MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL);
    Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, CData.buffersize );
    Mix_VolumeMusic( CData.musicvolume );
    Mix_AllocateChannels( CData.maxsoundchannel );
    if ( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, CData.buffersize ) != 0 )
    {
      log_message( "Failed!\n" );
      log_error( "Unable to initialize audio: %s\n", Mix_GetError() );
    }
    else log_message( "Succeeded!\n" );

    return btrue;
  }
  else return bfalse;
}

//------------------------------------
//SOUND-------------------------------
//------------------------------------

void sound_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  )
{
  // BB > This functions modifies an already playing 3d sound sound according position, orientation, etc.
  //      Modeled after physical parameters, but does not model doppler shift...
  float dist_xyz2, dist_xy2, volume;
  float vl, vr, dx, dy;
  int vol_left, vol_right;
  const float reverbdistance = 128 * 2.5;

  if ( INVALID_CHANNEL == channel ) return;

  dist_xy2 = ( ear_pos.x - snd_pos.x ) * ( ear_pos.x - snd_pos.x ) + ( ear_pos.y - snd_pos.y ) * ( ear_pos.y - snd_pos.y );
  dist_xyz2 = dist_xy2 + ( ear_pos.z - snd_pos.z ) * ( ear_pos.z - snd_pos.z );
  volume = intensity * ( CData.soundvolume / 100.0f ) * reverbdistance * reverbdistance / ( reverbdistance * reverbdistance + dist_xyz2 ); //adjust volume with ratio and sound volume

  // convert the camera turn to a vector
  turn_to_vec( ear_turn_lr, &dx, &dy );

  if ( dist_xy2 == 0.0f )
  {
    vl = vr = 0.5f;
  }
  else
  {
    // the the cross product of the direction with the camera direction
    // (proportional to the sine of the angle)
    float cp, ftmp;

    // normalize the square of the value
    cp = ( ear_pos.x - snd_pos.x ) * dy - ( ear_pos.y - snd_pos.y ) * dx;
    ftmp = cp * cp / dist_xy2;

    // determine the volume in the left and right speakers
    if ( cp > 0 )
    {
      vl = (1.0f + ftmp) * 0.5f;
      vr = (1.0f - ftmp) * 0.5f;
    }
    else
    {
      vl = (1.0f - ftmp) * 0.5f;
      vr = (1.0f + ftmp) * 0.5f;
    }
  }

  vol_left  = MIN(255, 255 * vl * volume);
  vol_right = MIN(255, 255 * vr * volume);

  Mix_SetPanning( channel, vol_left, vol_right );

};

int play_sound( float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, int whichobject, int soundnumber)
{
  // ZF> This function plays a specified sound
  // (Or returns -1 (INVALID_CHANNEL) if it failed to play the sound)
  int channel;

  if ( !CData.soundvalid ) return INVALID_CHANNEL;

  if ( loadedwave == NULL )
  {
    log_warning( "Sound file not correctly loaded (Not found?) - Object \"%s\" is trying to play sound%i.wav\n", CapList[ChrList[whichobject].model].classname, soundnumber );
    return INVALID_CHANNEL;
  }

  channel = Mix_PlayChannel( -1, loadedwave, loops );

  if( INVALID_CHANNEL == channel )
  {
    if(whichobject < 0)
    {
      log_warning( "All sound channels are currently in use. Global sound %d NOT playing\n", -whichobject );
    }
    else
    {
      log_warning( "All sound channels are currently in use. Sound is NOT playing - Object \"%s\" is trying to play sound%i.wav\n", CapList[ChrList[whichobject].model].classname, soundnumber );
    };
  }
  else
  {
    sound_apply_mods( channel, intensity, pos, GCamera.trackpos, GCamera.turn_lr);
  };

  return channel;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( float intensity, PRT_REF particle, Sint8 sound )
{
  //This function plays a sound effect for a particle
  if ( INVALID_SOUND == sound ) return;

  //Play local sound or else global (coins for example)
  if ( MAXMODEL != PrtList[particle].model )
  {
    play_sound( intensity, PrtList[particle].pos, CapList[PrtList[particle].model].wavelist[sound], 0, PrtList[particle].model, sound );
  }
  else
  {
    play_sound( intensity, PrtList[particle].pos, globalwave[sound], 0, -sound, sound );
  };
}


//------------------------------------------------------------------------------
void stop_sound( int whichchannel )
{
  // ZF> This function is used for stopping a looped sound, but can be used to stop
  // a particular sound too. If whichchannel is -1, all playing channels will fade out.
  if ( CData.soundvalid ) Mix_FadeOutChannel( whichchannel, 400 ); //400 ms is nice
}

//--------------------------------------------------------------------------------------------
void load_global_waves( char *modname )
{
  // ZZ> This function loads the global waves used in all modules
  STRING tmploadname, newloadname;
  Uint8 cnt;

  if ( CData.soundvalid )
  {
    // load in the sounds local to this module
    snprintf( tmploadname, sizeof( tmploadname ), "%s%s" SLASH_STRING, modname, CData.gamedat_dir );
    for ( cnt = 0; cnt < MAXWAVE; cnt++ )
    {
      snprintf( newloadname, sizeof( newloadname ), "%ssound%d.wav", tmploadname, cnt );
      globalwave[cnt] = Mix_LoadWAV( newloadname );
    };

    //These sounds are always standard, but DO NOT override sounds that were loaded local to this module
    if ( NULL == globalwave[GSOUND_COINGET] )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinget_sound );
      globalwave[GSOUND_COINGET] = Mix_LoadWAV( CStringTmp1 );
    };

    if ( NULL == globalwave[GSOUND_DEFEND] )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_sound );
      globalwave[GSOUND_DEFEND] = Mix_LoadWAV( CStringTmp1 );
    }

    if ( NULL == globalwave[GSOUND_COINFALL] )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinfall_sound );
      globalwave[GSOUND_COINFALL] = Mix_LoadWAV( CStringTmp1 );
    };

    if ( NULL == globalwave[GSOUND_LEVELUP] )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.lvlup_sound );
      globalwave[GSOUND_LEVELUP] = Mix_LoadWAV( CStringTmp1 );
    };
  }

  /*  The Global Sounds
  * 0 - Pick up coin
  * 1 - Defend clank
  * 2 - Weather Effect
  * 3 - Hit Water tile (Splash)
  * 4 - Coin falls on ground
  * 5 - Level up

  //TODO: These new values should determine sound and particle effects (examples below)
  Weather Type: DROPS, RAIN, SNOW, LAVABUBBLE (Which weather effect to spawn)
  Water Type: LAVA, WATER, DARK (To determine sound and particle effects)

  //We shold also add standard particles that can be used everywhere (Located and
  //loaded in globalparticles folder) such as these below.
  Particle Effect: REDBLOOD, SMOKE, HEALCLOUD
  */
}


//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
bool_t load_all_music_sounds()
{
  //ZF> This function loads all of the music sounds
  STRING songname;
  FILE *playlist;
  Uint8 cnt;

  //Open the playlist listing all music files
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.music_dir, CData.playlist_file );
  playlist = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( playlist == NULL )
  {
    log_warning( "Error opening playlist.txt\n" );
    return bfalse;
  }

  // Load the music data into memory
  if ( CData.musicvalid && !musicinmemory )
  {
    cnt = 0;
    while ( cnt < MAXPLAYLISTLENGTH && !feof( playlist ) )
    {
      fget_next_string( playlist, songname, sizeof( songname ) );
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.music_dir, songname );
      instrumenttosound[cnt] = Mix_LoadMUS( CStringTmp1 );
      cnt++;
    }
  }
  fs_fileClose( playlist );
  return btrue;
}

//------------------------------------------------------------------------------
void play_music( int songnumber, int fadetime, int loops )
{
  // ZF> This functions plays a specified track loaded into memory
  if ( songplaying != songnumber && CData.musicvalid )
  {
    Mix_FadeOutMusic( fadetime );
    Mix_PlayMusic( instrumenttosound[songnumber], loops );
    songplaying = songnumber;
  }
}

//------------------------------------------------------------------------------
void stop_music(int fadetime)
{
  //ZF> This function sets music track to pause
  if ( CData.musicvalid )
  {
    Mix_FadeOutMusic(fadetime);
  }
}
