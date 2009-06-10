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

#include "sound.h"
#include "camera.h"
#include "log.h"
#include "file_common.h"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

#include "egoboo_setup.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Sound using SDL_Mixer
static bool_t mixeron         = bfalse;


snd_config_t snd;

// music
bool_t      musicinmemory = bfalse;
Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];
Sint8       songplaying   = -1;

Mix_Chunk * g_wavelist[GSND_COUNT];

#define COIN1               0                       // Coins are the first particles loaded
#define COIN5               1
#define COIN25              2
#define COIN100             3
#define WEATHER4            4                       // Weather particles
#define WEATHER5            5                       // Weather particle finish
#define SPLASH              6                       // Water effects are next
#define RIPPLE              7
#define DEFEND              8                       // Defend particle

// text filenames for the global sounds
static const char * wavenames[GSND_COUNT] =
{
    "coinget",
    "defend",
    "weather1",
    "weather2",
    "coinfall",
    "lvlup",
    "pitfall"
};

static bool_t sound_atexit_registered = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t sdl_audio_initialize();
static bool_t sdl_mixer_initialize();
static void   sdl_mixer_quit(void);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// define a little stack for interrupting music sounds with other music

#define MUSIC_STACK_COUNT 20
static int music_stack_depth = 0;

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
        Mix_VolumeMusic( snd.musicvolume );
    }
}

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
    music_stack_push( musictracksloaded[0], 0 );

    // register the callback
    Mix_HookMusicFinished( music_stack_finished_callback );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t sdl_audio_initialize()
{
    bool_t retval = btrue;

    // make sure that SDL audio is turned on
    if ( 0 == SDL_WasInit(SDL_INIT_AUDIO) )
    {
        log_info( "Intializing SDL Audio... " );
        if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
        {
            log_message( "Failed!\n" );
            log_warning( "SDL error == \"%s\"\n", SDL_GetError() );

            retval = bfalse;
            snd.musicvalid = bfalse;
            snd.soundvalid = bfalse;
        }
        else
        {
            retval = btrue;
            log_message( "Succeess!\n" );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t sdl_mixer_initialize()
{
    //ZF> This intitializes the SDL_mixer services
    if ( !mixeron )
    {
        log_info( "Initializing SDL_mixer audio services version %d.%d.%d... ", SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL );
        if ( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, snd.buffersize ) < 0 )
        {
            mixeron = bfalse;
            log_message( "Failure!\n" );
            log_warning( "Unable to initialize audio: %s\n", Mix_GetError() );
        }
        else
        {
            Mix_VolumeMusic( snd.musicvolume );
            Mix_AllocateChannels( snd.maxsoundchannel );

            // initialize the music stack
            music_stack_init();

            mixeron = btrue;

            atexit( sdl_mixer_quit );
            sound_atexit_registered = btrue;

            log_message("Success!\n");
        }
    }

    return mixeron;
}

//--------------------------------------------------------------------------------------------
void sdl_mixer_quit(void)
{
    if ( mixeron && (0 != SDL_WasInit(SDL_INIT_AUDIO)) )
    {
        Mix_CloseAudio();
        mixeron = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This function enables the use of SDL_Audio and SDL_Mixer functions, returns btrue if success
bool_t sound_initialize()
{
    bool_t retval = bfalse;
    if ( sdl_audio_initialize() )
    {
        retval = sdl_mixer_initialize();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
Mix_Chunk * sound_load_chunk( const char * szFileName )
{
    STRING      full_file_name;
    Mix_Chunk * tmp_chunk;
    bool_t      file_exists = bfalse;

    if ( !mixeron ) return NULL;
    if ( NULL == szFileName || '\0' == szFileName[0] ) return NULL;

    // blank out the data
    tmp_chunk = NULL;

    // try a wav file
    snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "wav" );
    if ( fs_fileExists(full_file_name) )
    {
        file_exists = btrue;
        tmp_chunk = Mix_LoadWAV( full_file_name );
    }

    if ( NULL == tmp_chunk )
    {
        // try an ogg file
        snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "ogg" );
        if ( fs_fileExists(full_file_name) )
        {
            file_exists = btrue;
            tmp_chunk = Mix_LoadWAV(full_file_name);
        }
    }

    if ( cfg.dev_mode && file_exists && NULL == tmp_chunk )
    {
        // there is an error only if the file exists and can't be loaded
        log_warning( "Sound file not found/loaded %s.\n", szFileName );
    }

    return tmp_chunk;
}

//--------------------------------------------------------------------------------------------
Mix_Music * sound_load_music( const char * szFileName )
{
    STRING      full_file_name;
    Mix_Music * tmp_music;
    bool_t      file_exists = bfalse;

    if ( !mixeron ) return NULL;
    if ( NULL == szFileName || '\0' == szFileName[0] ) return NULL;

    // blank out the data
    tmp_music = NULL;

    // try a wav file
    snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "wav" );
    if ( fs_fileExists(full_file_name) )
    {
        file_exists = btrue;
        tmp_music = Mix_LoadMUS( full_file_name );
    }

    if (NULL == tmp_music)
    {
        // try an ogg file
        tmp_music = NULL;
        snprintf( full_file_name, sizeof(full_file_name), "%s.%s", szFileName, "ogg" );
        if ( fs_fileExists(full_file_name) )
        {
            file_exists = btrue;
            tmp_music = Mix_LoadMUS(full_file_name);
        }
    }

    if ( cfg.dev_mode && file_exists && NULL == tmp_music )
    {
        // there is an error only if the file exists and can't be loaded
        log_warning( "Music file not found/loaded %s.\n", szFileName );
    }

    return tmp_music;
}

//--------------------------------------------------------------------------------------------
bool_t sound_load( mix_ptr_t * pptr, const char * szFileName, mix_type_t type )
{

    if ( !mixeron ) return bfalse;
    if ( NULL == pptr ) return bfalse;

    // clear out the data
    pptr->ptr.unk = NULL;
    pptr->type    = MIX_UNKNOWN;

    if ( NULL == szFileName || '\0' == szFileName[0] ) return bfalse;

    switch ( type )
    {
        case MIX_MUS:
            pptr->ptr.mus = sound_load_music( szFileName );
            if (NULL != pptr->ptr.mus)
            {
                pptr->type = MIX_MUS;
            }
            break;

        case MIX_SND:
            pptr->ptr.snd = sound_load_chunk( szFileName );
            if (NULL != pptr->ptr.snd)
            {
                pptr->type = MIX_SND;
            }
            break;

        case MIX_UNKNOWN:
            /* do nothing */
            break;

        default:
            if ( cfg.dev_mode )
            {
                // there is an error only if the file exists and can't be loaded
                log_warning( "sound_load() - Mix type recognized %d.\n", type );
            }
            break;
    };

    return MIX_UNKNOWN != pptr->type;
}

//--------------------------------------------------------------------------------------------
int sound_play_mix( GLvector3 pos, mix_ptr_t * pptr )
{
    int retval = -1;
    if ( !snd.soundvalid || !mixeron )
    {
        return -1;
    }
    if ( NULL == pptr || MIX_UNKNOWN == pptr->type || NULL == pptr->ptr.unk )
    {
        if ( cfg.dev_mode )
        {
            if (cfg.dev_mode) log_warning( "Unable to load sound. (%s)\n", Mix_GetError() );
        }
        return -1;
    }

    retval = -1;
    if ( MIX_SND == pptr->type )
    {
        retval = sound_play_chunk( pos, pptr->ptr.snd );
    }
    else if ( MIX_MUS == pptr->type )
    {
        // !!!!this will override the music!!!!

        // add the old stream to the stack
        music_stack_push( musictracksloaded[songplaying], songplaying );

        // push on a new stream, play only once
        retval = Mix_PlayMusic( pptr->ptr.mus, 1 );

        // invalidate the song
        songplaying = INVALID_SOUND;

        // since music_stack_finished_callback() is registered using Mix_HookMusicFinished(),
        // it will resume when pptr->ptr.mus is finished playing
    }

    return retval;

}

//--------------------------------------------------------------------------------------------
void sound_restart()
{
    if ( mixeron )
    {
        Mix_CloseAudio();
        mixeron = bfalse;
    }

    // loose the info on the currently playing song
    //songplaying = -1;
    if ( snd.musicvalid || snd.soundvalid )
    {
        if ( -1 != Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, snd.buffersize ) )
        {
            mixeron = btrue;
            Mix_AllocateChannels( snd.maxsoundchannel );
            Mix_VolumeMusic( snd.musicvolume );
        }
        else
        {
            log_warning( "sound_restart() - Cannot get the sound module to restart. (%s)\n", Mix_GetError() );
        }
    }
}

//------------------------------------
// Mix_Chunk stuff -------------------
//------------------------------------
int sound_play_chunk_looped( GLvector3 pos, Mix_Chunk * pchunk, Sint8 loops )
{
    // This function plays a specified sound and returns which channel it's using
    int channel;
    GLvector3 diff;
    float dist2;
    int volume;

    const float reverb_dist2 = 200 * 200;

    if ( !snd.soundvalid || !mixeron || NULL == pchunk ) return INVALID_SOUND;

    // measure the distance in tiles
    diff = VSub( pos, PCamera->track_pos );
    dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    // adjust for the local_listening skill
    if ( local_listening ) dist2 *= 0.66f * 0.66f;

    volume  = 128 * reverb_dist2 / (reverb_dist2 + dist2);
    volume *= ( volume * snd.soundvolume ) / 100;

    // play the sound
    channel = INVALID_SOUND;
    if ( volume > 0 )
    {
        channel = Mix_PlayChannel( -1, pchunk, loops );
        if ( INVALID_SOUND == channel )
        {
            if (cfg.dev_mode) log_warning( "Unable to play sound. (%s)\n", Mix_GetError() );
        }
        else
        {
            float pan;
            float cosval;
            int leftvol;

            // determine the angle away from "forward"
            pan = ATAN2( diff.y, diff.x ) - PCamera->turn_z_rad;
            volume *= (2.0f + cos( pan )) / 3.0f;

            // determine the angle from the left ear
            pan += ( 1.5f * PI );

            // determine the panning
            cosval = cos(pan);
            cosval *= cosval;
            leftvol  = cosval * 128;

            // apply the volume adjustments
            Mix_Volume(channel, volume);
            Mix_SetPanning(channel, 127 + leftvol, 255 - leftvol);
        }
    }

    return channel;
}
//--------------------------------------------------------------------------------------------
void sound_stop_channel( int whichchannel )
{
    if ( mixeron && snd.soundvalid )
    {
        Mix_HaltChannel( whichchannel );
    }
}

//------------------------------------
// Mix_Music stuff -------------------
//------------------------------------
void sound_play_song( Sint8 songnumber, Uint16 fadetime, Sint8 loops )
{
    if ( !snd.musicvalid || !mixeron ) return;

    // This functions plays a specified track loaded into memory
    if ( songplaying != songnumber )
    {
        // Mix_FadeOutMusic(fadetime);      // Stops the game too

        if ( loops != 0 )
        {
            if ( INVALID_SOUND != songplaying )
            {
                music_stack_push( musictracksloaded[songplaying], songplaying );
            }
        }

        Mix_FadeInMusic( musictracksloaded[songnumber], loops, fadetime );

        songplaying = songnumber;
    }
}

//--------------------------------------------------------------------------------------------
void sound_stop_song()
{
    // This function sets music track to pause
    if ( mixeron && snd.musicvalid )
    {
        Mix_HaltMusic();
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_global_waves( const char * modname )
{
    // ZZ> This function loads the global waves
    STRING tmploadname;
    STRING wavename;
    int cnt;

    // Grab these sounds from the basicdat dir
    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_GETCOIN] );
    g_wavelist[GSND_GETCOIN] = sound_load_chunk( wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_DEFEND] );
    g_wavelist[GSND_DEFEND] = sound_load_chunk( wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_COINFALL] );
    g_wavelist[GSND_COINFALL] = sound_load_chunk( wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "%s", wavenames[GSND_LEVELUP] );
    g_wavelist[GSND_LEVELUP] = sound_load_chunk( wavename );

    snprintf( wavename, sizeof(wavename), "basicdat" SLASH_STR "%s", wavenames[GSND_PITFALL] );
    g_wavelist[GSND_PITFALL] = sound_load_chunk( wavename );

    /*
    // These new values todo should determine global sound and particle effects
    Weather Type: DROPS, RAIN, SNOW
    Water Type: LAVA, WATER, DARK
    */

    // try to grab these same sounds from the gamedat dir. This lets the local sounds override the
    // global sounds.
    make_newloadname( modname, "gamedat", tmploadname );

    for ( cnt = 0; cnt < GSND_COUNT; cnt++ )
    {
        Mix_Chunk * ptmp;

        // only overwrite with a valid sound file
        snprintf( wavename, sizeof(wavename), "%s" SLASH_STR "%s", tmploadname, wavenames[cnt] );

        ptmp = sound_load_chunk( wavename );
        if ( NULL == ptmp )
        {
            snprintf( wavename, sizeof(wavename), "%s" SLASH_STR "sound%d", tmploadname, cnt );
            ptmp = sound_load_chunk( wavename );
        }

        if (NULL != ptmp)
        {
            g_wavelist[cnt] = ptmp;
        };
    }

}

//--------------------------------------------------------------------------------------------
void load_all_music_sounds()
{
    // This function loads all of the music sounds
    char loadpath[128];
    char songname[128];
    FILE *playlist;
    Uint8 cnt;

    if ( musicinmemory || !snd.musicvalid ) return;

    // Open the playlist listing all music files
    playlist = fopen( "basicdat" SLASH_STR "music" SLASH_STR "playlist.txt", "r" );
    if ( playlist == NULL )
    {
        log_warning( "Error opening playlist.txt\n" );
        return;
    }

    // Load the music data into memory
    for ( cnt = 0; cnt < MAXPLAYLISTLENGTH && !feof( playlist ); cnt++ )
    {
        goto_colon( NULL, playlist, btrue );
        fscanf( playlist, "%s", songname );
        sprintf( loadpath, ( "basicdat" SLASH_STR "music" SLASH_STR "%s" ), songname );

        musictracksloaded[cnt] = Mix_LoadMUS( loadpath );
    }
    musicinmemory = btrue;

    fclose( playlist );
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t snd_config_init( snd_config_t * psnd )
{
    if ( NULL == psnd ) return bfalse;

    psnd->soundvalid        = bfalse;
    psnd->musicvalid        = bfalse;
    psnd->musicvolume       = 50;                            // The sound volume of music
    psnd->soundvolume       = 75;          // Volume of sounds played
    psnd->maxsoundchannel   = 16;      // Max number of sounds playing at the same time
    psnd->buffersize        = 2048;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t snd_config_synch( snd_config_t * psnd, egoboo_config_t * pcfg )
{
    if ( NULL == psnd && NULL == pcfg ) return bfalse;

    if ( NULL == pcfg )
    {
        return snd_config_init( psnd );
    }

    // coerce pcfg to have valid values
    pcfg->sound_channel_count = CLIP(pcfg->sound_channel_count, 8, 128);
    pcfg->sound_buffer_size      = CLIP(pcfg->sound_buffer_size, 512, 8196);

    if ( NULL != psnd )
    {
        psnd->soundvalid      = pcfg->sound_allowed;
        psnd->soundvolume     = pcfg->sound_volume;
        psnd->musicvalid      = pcfg->music_allowed;
        psnd->musicvolume     = pcfg->music_volume;
        psnd->maxsoundchannel = pcfg->sound_channel_count;
        psnd->buffersize      = pcfg->sound_buffer_size;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void sound_fade_all()
{
    if ( mixeron  )
    {
        Mix_FadeOutChannel( -1, 500 );     // Stop all sounds that are playing
    }
}

//--------------------------------------------------------------------------------------------
void fade_in_music( Mix_Music * music )
{
    if ( mixeron  )
    {
        Mix_FadeInMusic( music, -1, 500 );
    }
}
