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

#include "egoboo.h"
#include "Log.h"

//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
  if (( CData.musicvalid || CData.soundvalid ) && !mixeron )
  {
    log_info( "Initializing SDL_mixer audio services version %i.%i.%i... ", MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL);
    Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, CData.buffersize );
    Mix_VolumeMusic( CData.musicvolume );  //*1.28
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

int play_sound( float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, int whichobject, int soundnumber )
{
  // This function plays a specified sound

  if ( !CData.soundvalid ) return INVALID_CHANNEL;

  if ( loadedwave == NULL )
  {
    log_warning( "Sound file not correctly loaded (Not found?), Object \"%s\" is trying to play sound%i.wav\n", capclassname[chrmodel[whichobject]], soundnumber );
    return INVALID_CHANNEL;
  }

  channel = Mix_PlayChannel( -1, loadedwave, loops );

  if( INVALID_CHANNEL == channel )
  {
    log_warning( "All sound channels are currently in use. Sound is NOT playing. Object \"%s\" is trying to play sound%i.wav\n", capclassname[chrmodel[whichobject]], soundnumber );
  }
  else
  {
    sound_apply_mods( channel, intensity, pos, camtrackpos, camturn_lr);
  };

  return channel;
}


//------------------------------------------------------------------------------
void stop_sound( int whichchannel )
{
  //This function is used for stopping a looped sound, but can be used to stop
  //a particular sound too. If whichchannel is -1, all playing channels will fade out.
  if ( CData.soundvalid ) Mix_FadeOutChannel( whichchannel, 400 );
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
void load_all_music_sounds()
{
  //This function loads all of the music sounds
  STRING songname;
  FILE *playlist;
  int cnt;

  //Open the playlist listing all music files
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.music_dir, CData.playlist_file );
  playlist = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( playlist == NULL ) log_warning( "Error opening playlist.txt\n" );

  // Load the music data into memory
  if ( CData.musicvalid && !musicinmemory )
  {
    cnt = 0;
    while ( cnt < MAXPLAYLISTLENGHT && !feof( playlist ) )
    {
      fget_next_string( playlist, songname, sizeof( songname ) );
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.music_dir, songname );
      instrumenttosound[cnt] = Mix_LoadMUS( CStringTmp1 );
      cnt++;
    }
    musicinmemory = btrue;
  }
  fs_fileClose( playlist );
}

void play_music( int songnumber, int fadetime, int loops )
{
  //This functions plays a specified track loaded into memory
  if ( songplaying != songnumber && CData.musicvalid )
  {
    Mix_FadeOutMusic( fadetime );
    Mix_PlayMusic( instrumenttosound[songnumber], loops );
    songplaying = songnumber;
  }
}

void stop_music(int fadetime)
{
  //This function sets music track to pause
  if ( CData.musicvalid )
  {
    Mix_FadeOutMusic(fadetime);
  }
}
