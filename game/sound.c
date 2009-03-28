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
// text filenames for the global sounds
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// define a little stack for interrupting music sounds with other music

#define MUSIC_STACK_COUNT 20
static int         music_stack_depth = 0;
struct s_music_stack_element
{
    Mix_Music * mus;
    int         number;
};

typedef struct s_music_stack_element music_stack_element_t;

static music_stack_element_t music_stack[MUSIC_STACK_COUNT];

static bool_t music_stack_pop(Mix_Music ** mus, int * song);

//--------------------------------------------------------------------------------------------
static void music_stack_finished_callback(void)
{
    // this function is only called when a music function is finished playing
    // pop the saved music off the stack

    // unfortunately, it seems that SDL_mixer does not support saving the position of
    // the music stream, so the music track will restart from the beginning

    Mix_Music * mus;
    int         song;

    // grab the next song
    if ( music_stack_pop(&mus, &song) )
    {
        // play the music
        Mix_PlayMusic( mus, 0 );

        songplaying = song;

        // set the volume
        Mix_VolumeMusic( musicvolume );
    }
};

//--------------------------------------------------------------------------------------------
static bool_t music_stack_push(Mix_Music * mus, int song)
{
    if ( music_stack_depth >= MUSIC_STACK_COUNT - 1 )
    {
        music_stack_depth = MUSIC_STACK_COUNT - 1;
        return bfalse;
    }

    music_stack[music_stack_depth].mus    = mus;
    music_stack[music_stack_depth].number = song;

    music_stack_depth++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t music_stack_pop(Mix_Music ** mus, int * song)
{
    if (NULL == mus || NULL == song) return bfalse;

    if (music_stack_depth > 0)
    {
        music_stack_depth--;
    }

    *mus  = music_stack[music_stack_depth].mus;
    *song = music_stack[music_stack_depth].number;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static void music_stack_init()
{
    // push on the default music value
    music_stack_push( musictracksloaded[0].ptr.mus, 0 );

    // register the callback
    Mix_HookMusicFinished( music_stack_finished_callback );
};

//--------------------------------------------------------------------------------------------
// This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
    // make sure that SDL audio is turned on
    if ( 0 == SDL_WasInit(SDL_INIT_AUDIO) )
    {
        log_info( "Intializing SDL Audio... " );
        if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
        {
            log_message( "Failed!\n" );
            log_warning( "SDL error == \"%s\"\n", SDL_GetError() );

            musicvalid = bfalse;
            soundvalid = bfalse;
            return bfalse;
        }
        else
        {
            log_message( "Succeess!\n" );
        }
    }

    if ( ( musicvalid || soundvalid ) && !mixeron )
    {
        log_info( "Initializing SDL_mixer audio services version %d.%d.%d... ", SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL );

        if ( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize ) < 0 )
        {
            mixeron = bfalse;
            log_message( "Failure!\n" );
            log_warning( "Unable to initialize audio: %s\n", Mix_GetError() );
        }
        else
        {
            Mix_VolumeMusic( musicvolume );
            Mix_AllocateChannels( maxsoundchannel );

            // initialize the music stack
            music_stack_init();

            mixeron = btrue;

            log_message("Success!\n");
        }
    }

    return mixeron;
}

//--------------------------------------------------------------------------------------------
bool_t load_sound( mix_ptr_t * pptr, const char * szFileName )
{
    STRING      full_file_name;
    Mix_Chunk * tmp_chunk;
    Mix_Music * tmp_music;
    bool_t      file_exists = bfalse;

    if ( !mixeron ) return bfalse;
    if ( NULL == pptr ) return bfalse;
    if ( NULL == szFileName || '\0' == szFileName[0] ) return bfalse;

    // blank out the data
    pptr->ptr.unk = NULL;
    pptr->type    = MIX_UNKNOWN;

    // try a wav file
    tmp_chunk = NULL;
    snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "wav" );
    if( fs_fileExists(full_file_name) )
    {
        file_exists = btrue;
        tmp_chunk = Mix_LoadWAV( full_file_name );
    }

    if (NULL != tmp_chunk)
    {
        pptr->ptr.snd = tmp_chunk;
        pptr->type    = MIX_SND;
    }
    else
    {
        // try an ogg file
        tmp_music = NULL;
        snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "ogg" );
        if( fs_fileExists(full_file_name) )
        {
            file_exists = btrue;
            tmp_music = Mix_LoadMUS(full_file_name);
        }
        if (NULL != tmp_music)
        {
            pptr->ptr.mus = tmp_music;
            pptr->type    = MIX_MUS;
        }
    }

    if ( gDevMode && file_exists && NULL == pptr->ptr.unk ) 
    {
        // there is an error only if the file exists and can't be loaded
        log_warning( "Sound file not found/loaded %s.\n", szFileName );
    }

    return NULL != pptr->ptr.unk;
}




//--------------------------------------------------------------------------------------------
void load_global_waves( char * modname )
{
    // ZZ> This function loads the global waves
    STRING tmploadname;
    STRING wavename;
    int cnt;

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

    for ( cnt = 0; cnt < MAXGLOBALSOUNDS; cnt++ )
    {
        mix_ptr_t tmp_ptr;

        snprintf( wavename, sizeof(wavename), "%s" SLASH_STR "%s", tmploadname, wavenames[cnt] );

        // only overwrite with a valid sound file
        if ( load_sound( &tmp_ptr, wavename ) )
        {
            memcpy( globalwave + cnt, &tmp_ptr, sizeof(mix_ptr_t) );
        }
    }

}

//--------------------------------------------------------------------------------------------
int play_mix( float xpos, float ypos, mix_ptr_t * pptr )
{
    int retval = -1;

    if ( !soundvalid || !mixeron )
    {
        return -1;
    }

    if ( NULL == pptr || MIX_UNKNOWN == pptr->type || NULL == pptr->ptr.unk )
    {
        if ( gDevMode )
        {
            //log_warning( "Sound file not correctly loaded (Not found?).\n" );
        }
        return -1;
    }

    retval = -1;
    if ( MIX_SND == pptr->type )
    {
        retval = play_sound( xpos, ypos, pptr->ptr.snd );
    }
    else if ( MIX_MUS == pptr->type )
    {
        // !!!!this will override the music!!!!

        // add the old stream to the stack
        music_stack_push( musictracksloaded[songplaying].ptr.mus, songplaying );

        // push on a new stream, play only once
        retval = Mix_PlayMusic( pptr->ptr.mus, 1 );

        // invalidate the song
        songplaying = -1;

        // since music_stack_finished_callback() is registered using Mix_HookMusicFinished(),
        // it will resume when pptr->ptr.mus is finished playing
    }

    return retval;

}

//------------------------------------
// SOUND-------------------------------
//------------------------------------
int play_sound( float xpos, float ypos, Mix_Chunk * pchunk )
{
    // This function plays a specified sound

    int dist, volume, pan;

    if ( !mixeron || NULL == pchunk ) return -1;

    // measure the distance in tiles
    dist = SQRT( POW( ABS( camtrackx - xpos ), 2 ) + POW( ABS( camtracky - ypos ), 2 ) ); // Ugly, but just the dist formula
    dist >>= 7;

    // adjust for the local_listening skill
    if ( local_listening ) dist *= 0.66f;

    // adjust for the soundvolume
    dist *= VOLUMERATIO * 2;
    volume = 255 - dist;
    dist   = 255 - ( volume * soundvolume ) / 100;

    // determine the angle
    pan    = 57.3f * ( ( 1.5f * PI ) - ATAN2( camy - ypos, camx - xpos ) - camturnleftright ); // Convert the camera angle to the nearest integer degree

    // play the sound
    if ( dist <= 255 )
    {
        channel = Mix_PlayChannel( -1, pchunk, 0 );

        if ( -1 == channel )
        {
            // log_warning( "All sound channels are currently in use. Sound is NOT playing.\n" );
        }
        else
        {
            Mix_SetPosition( channel, pan, dist );
        }
    }

    return channel;
}

//--------------------------------------------------------------------------------------------
// TODO:
void stop_sound( int whichchannel )
{
    if ( mixeron && soundvalid )
    {
        Mix_HaltChannel( whichchannel );
    }
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
            if (NULL != tmp_music)
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
    if ( !mixeron ) return;

    // This functions plays a specified track loaded into memory
    if ( songplaying != songnumber && musicvalid )
    {
        // Mix_FadeOutMusic(fadetime);      // Stops the game too

        if ( MIX_MUS == musictracksloaded[songnumber].type )
        {
            if ( loops != 0 )
            {
                if ( -1 != songplaying )
                {
                    music_stack_push( musictracksloaded[songplaying].ptr.mus, songplaying );
                }
            }

            Mix_FadeInMusic( musictracksloaded[songnumber].ptr.mus, loops, fadetime );

            songplaying = songnumber;
        }
    }
}

//--------------------------------------------------------------------------------------------
void stop_music()
{
    // This function sets music track to pause
    if ( mixeron && musicvalid )
    {
        Mix_HaltMusic();
    }
}
