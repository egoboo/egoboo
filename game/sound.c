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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - sound.c
 * Sound code in Egoboo is implemented using SDL_mixer.
 */

#include "egoboo.h"
#include "log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t load_sound( mix_ptr_t * pptr, const char * szFileName )
{
    STRING      full_file_name;
    Mix_Chunk * tmp_chunk;
    Mix_Music * tmp_music;

    if ( NULL == pptr ) return bfalse;
    if ( NULL == szFileName || '\0' == szFileName[0] ) return bfalse;

    // blank out the data
    pptr->ptr.unk = NULL;
    pptr->type    = MIX_UNKNOWN;

    // try a wav file
    snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "wav" );
    tmp_chunk = Mix_LoadWAV( full_file_name );
    if (NULL != tmp_chunk)
    {
        pptr->ptr.snd = tmp_chunk;
        pptr->type    = MIX_SND;
    }
    else
    {
        // try an ogg file
        snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "ogg" );
        tmp_music = Mix_LoadMUS(full_file_name);
        if (NULL != tmp_music)
        {
            pptr->ptr.mus = tmp_music;
            pptr->type    = MIX_MUS;
        }
    }

    return NULL != pptr->ptr.unk;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static const char * wavenames[MAXGLOBALSOUNDS] = 
{
    "coinget",
    "defend",
    "weather1",
    "weather2",
    "coinfall",
    "lvlup",
    "pitfall"
};

void load_global_waves( char * modname )
{
    // ZZ> This function loads the global waves
    char tmploadname[256];
    char newloadname[256];
    STRING wavename;
    Uint8 cnt = 0;

    if (!soundvalid) return;

    // Grab these sounds from the basicdat dir
    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[SND_GETCOIN] );
    load_sound( globalwave + SND_GETCOIN, wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[SND_DEFEND] );
    load_sound( globalwave + SND_DEFEND,  wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[SND_COINFALL] );
    load_sound( globalwave + SND_COINFALL, wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "%s", wavenames[SND_LEVELUP] );
    load_sound( globalwave + SND_LEVELUP,  wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "%s", wavenames[SND_PITFALL] );
    load_sound( globalwave + SND_PITFALL,  wavename );

    /*
    // These new values todo should determine sound and particle effects
    Weather Type: DROPS, RAIN, SNOW
    Water Type: LAVA, WATER, DARK
    */

    // try to grab these same sounds from the gamedat dir. This lets the local sounds override the 
    // global sounds.
    make_newloadname( modname, "gamedat", tmploadname );

    for( cnt = 0; cnt<MAXGLOBALSOUNDS; cnt++)
    {
        mix_ptr_t tmp_ptr;

        snprintf( wavename, sizeof(wavename), "%s" SLASH_STR "%s", tmploadname, wavenames[cnt] );

        // only overwrite with a valid sound file
        if( load_sound( &tmp_ptr, wavename ) )
        {
            memcpy( globalwave + cnt, &tmp_ptr, sizeof(mix_ptr_t) );
        }
    }

}

//--------------------------------------------------------------------------------------------
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
int play_sound( float xpos, float ypos, mix_ptr_t * pptr )
{
    int distance, volume, pan;

    if( !soundvalid ) 
    {
        return -1;
    }

    if( NULL == pptr || MIX_UNKNOWN == pptr->type || NULL == pptr->ptr.unk ) 
    {
        log_warning( "Sound file not correctly loaded (Not found?).\n" );
        return -1;
    }

    // This function plays a specified sound
    distance = SQRT( POW( ABS( camtrackx - xpos ), 2 ) + POW( ABS( camtracky - ypos ), 2 ) ); // Ugly, but just the distance formula

    if ( listening ) distance *= 0.66f;

    volume = ( ( distance / VOLUMERATIO ) * ( 1 + ( soundvolume / 100 ) ) ); // adjust volume with ratio and sound volume
    pan = 57.3f * ( ( 1.5f * PI ) - ATAN2( camy - ypos, camx - xpos ) - camturnleftright ); // Convert the camera angle to the nearest integer degree

    if ( pan < 0 ) pan += 360;

    if ( pan > 360 ) pan -= 360;

    if ( volume < 255 )
    {
        if( MIX_MUS == pptr->type )
        {
            channel = -1;
            channel = Mix_PlayMusic( pptr->ptr.mus, 1 );
        }
        else if ( MIX_SND == pptr->type )
        {
            channel = Mix_PlayChannel( -1, pptr->ptr.snd, 0 );
        }

        if ( channel != -1 && MIX_SND == pptr->type )
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
        else 
        {
            log_warning( "All sound channels are currently in use. Sound is NOT playing.\n" );
        }
    }


    return channel;
}

//--------------------------------------------------------------------------------------------
// TODO:
void stop_sound( int whichchannel )
{
    if ( soundvalid ) Mix_HaltChannel( whichchannel );
}

//--------------------------------------------------------------------------------------------
// Music Stuff-------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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
        

        for ( cnt = 0; cnt < MAXPLAYLISTLENGTH && !feof( playlist ); cnt++ )
        {
            Mix_Music * tmp_music;

            goto_colon_yesno( playlist );
            fscanf( playlist, "%s", songname );
            sprintf( loadpath, ( "basicdat" SLASH_STR "music" SLASH_STR "%s" ), songname );

            musictracksloaded[cnt].ptr.unk = NULL;
            musictracksloaded[cnt].type    = MIX_UNKNOWN;

            tmp_music = Mix_LoadMUS( loadpath );
            if(NULL != tmp_music)
            {
                musictracksloaded[cnt].ptr.mus = tmp_music;
                musictracksloaded[cnt].type    = MIX_MUS;
            }
        }

        musicinmemory = btrue;
    }

    fclose( playlist );
}

//--------------------------------------------------------------------------------------------
void play_music( Sint8 songnumber, Uint16 fadetime, Sint8 loops )
{
    // This functions plays a specified track loaded into memory
    if ( songplaying != songnumber && musicvalid )
    {
        // Mix_FadeOutMusic(fadetime);      // Stops the game too

        if( MIX_MUS == musictracksloaded[songnumber].type )
        {
            Mix_PlayMusic( musictracksloaded[songnumber].ptr.mus, loops );
        }

        songplaying = songnumber;
    }
}

//--------------------------------------------------------------------------------------------
void stop_music()
{
    // This function sets music track to pause
    if ( musicvalid )
    {
        Mix_HaltMusic();
    }
}
