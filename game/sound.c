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
#include "game.h"
#include "char.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include "egoboo.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LOOPED_COUNT 256

struct s_looped_sound_data
{
    int         channel;
    Mix_Chunk * chunk;
    Uint16      object;
};
typedef struct s_looped_sound_data looped_sound_data_t;

DEFINE_LIST_STATIC(looped_sound_data_t, LoopedList, LOOPED_COUNT );

DECLARE_LIST( static, looped_sound_data_t, LoopedList );

void   LoopedList_init();
void   LoopedList_clear();
bool_t LoopedList_free_one( int index );
int    LoopedList_get_free();

bool_t LoopedList_validate();
int    LoopedList_add( Mix_Chunk * sound, int loops, Uint16 object );
bool_t LoopedList_remove( int channel );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Sound using SDL_Mixer
static bool_t mixeron         = bfalse;

snd_config_t snd;

// music
bool_t      musicinmemory = bfalse;
Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];
Sint8       songplaying   = INVALID_SOUND;

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

int    _calculate_volume( GLvector3 diff );
bool_t _update_channel_volume( int channel, int volume, GLvector3 diff );
bool_t _update_stereo_channel( int channel, GLvector3 diff );

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
bool_t music_stack_push(Mix_Music * mus, int song)
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
bool_t music_stack_pop(Mix_Music ** mus, int * song)
{
    if (NULL == mus || NULL == song) return bfalse;

    // fail if music isn't loaded
    if ( !musicinmemory )
    {
        *mus = NULL;
        *song = -1;
        return bfalse;
    }

    // set the default to be song 0
    *song = 0;
    *mus  = musictracksloaded[*song];

    // pop the stack, if possible
    if (music_stack_depth > 0)
    {
        music_stack_depth--;

        *mus  = music_stack[music_stack_depth].mus;
        *song = music_stack[music_stack_depth].number;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void music_stack_init()
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
    // ZF> This intitializes the SDL_mixer services
    if ( !mixeron && ( snd.musicvalid || snd.soundvalid ) )
    {
        log_info( "Initializing SDL_mixer audio services version %d.%d.%d... ", SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL );
        if ( Mix_OpenAudio( cfg.sound_highquality ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, snd.buffersize ) < 0 )
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

    if ( retval )
    {
        LoopedList_init();
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
    if ( INVALID_CSTR(szFileName) ) return NULL;

    // blank out the data
    tmp_chunk = NULL;

    // try a wav file
    snprintf( full_file_name, SDL_arraysize(full_file_name), "%s.%s", szFileName, "wav" );
    if ( vfs_exists(full_file_name) )
    {
        file_exists = btrue;
        tmp_chunk = Mix_LoadWAV( vfs_resolveReadFilename(full_file_name) );
    }

    if ( NULL == tmp_chunk )
    {
        // try an ogg file
        snprintf( full_file_name, SDL_arraysize(full_file_name), "%s.%s", szFileName, "ogg" );
        if ( vfs_exists(full_file_name) )
        {
            file_exists = btrue;
            tmp_chunk = Mix_LoadWAV( vfs_resolveReadFilename(full_file_name) );
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
    if ( INVALID_CSTR(szFileName) ) return NULL;

    // blank out the data
    tmp_music = NULL;

    // try a wav file
    snprintf( full_file_name, SDL_arraysize(full_file_name), "%s.%s", szFileName, "wav" );
    if ( vfs_exists(full_file_name) )
    {
        file_exists = btrue;
        tmp_music = Mix_LoadMUS( full_file_name );
    }

    if (NULL == tmp_music)
    {
        // try an ogg file
        tmp_music = NULL;
        snprintf( full_file_name, SDL_arraysize(full_file_name), "%s.%s", szFileName, "ogg" );
        if ( vfs_exists(full_file_name) )
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

    if ( INVALID_CSTR(szFileName) ) return bfalse;

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
    if ( snd.musicvalid || snd.soundvalid )
    {
        if ( -1 != Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, snd.buffersize ) )
        {
            mixeron = btrue;
            Mix_AllocateChannels( snd.maxsoundchannel );
            Mix_VolumeMusic( snd.musicvolume );

            // initialize the music stack
            music_stack_init();

            if ( !sound_atexit_registered )
            {
                atexit( sdl_mixer_quit );
                sound_atexit_registered = btrue;
            }
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

int _calculate_volume( GLvector3 diff )
{
    // BB> make this code its own function

    float dist2;
    int volume;
    float render_size;

    // approximate the radius of the area that the camera sees
    render_size = renderlist.all_count * (TILE_SIZE / 2 * TILE_SIZE / 2) / 4;

    dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    // adjust for the local_listening skill
    if ( local_listening ) dist2 *= 0.66f * 0.66f;

    volume  = 255 * render_size / (render_size + dist2);
    volume  = ( volume * snd.soundvolume ) / 100;

    return volume;
}

bool_t _update_channel_volume( int channel, int volume, GLvector3 diff )
{
    float pan;
    float cosval;
    int leftvol, rightvol;

    // determine the angle away from "forward"
    pan = ATAN2( diff.y, diff.x ) - PCamera->turn_z_rad;
    volume *= (2.0f + cos( pan )) / 3.0f;

    // determine the angle from the left ear
    pan += 1.5f * PI;

    // determine the panning
    cosval = cos(pan);
    cosval *= cosval;

    leftvol  = cosval * 128;
    rightvol = 128 - leftvol;

    leftvol  = ((127 + leftvol ) * volume) >> 8;
    rightvol = ((127 + rightvol) * volume) >> 8;

    // apply the volume adjustments
    Mix_SetPanning( channel, leftvol, rightvol );

    return btrue;
}

int sound_play_chunk_looped( GLvector3 pos, Mix_Chunk * pchunk, Sint8 loops, Uint16 object )
{
    // This function plays a specified sound and returns which channel it's using
    int channel = INVALID_SOUND;
    GLvector3 diff;
    int volume;

    if ( !snd.soundvalid || !mixeron || NULL == pchunk ) return INVALID_SOUND;

    // only play sound effects if the game is running
    if ( !process_instance_running( PROC_PBASE(GProc) ) )  return INVALID_SOUND;

    // measure the distance in tiles
    diff = VSub( pos, PCamera->track_pos );
    volume = _calculate_volume( diff );

    // play the sound
    if ( volume > 0 )
    {
        // this function handles loops == 0 properly
        channel = LoopedList_add( pchunk, loops, object );

        if ( INVALID_SOUND == channel )
        {
            if (cfg.dev_mode)
            {
                log_warning( "Unable to play sound. (%s)\n", Mix_GetError() );
            }
        }
        else
        {
            //Set left/right panning
            _update_channel_volume( channel, volume, diff );
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
void sound_finish_song( Uint16 fadetime )
{
    Mix_Music * mus;
    int         song;

    if ( !snd.musicvalid || !mixeron || songplaying == MENU_SONG ) return;

    if ( !musicinmemory )
    {
        Mix_HaltMusic();
        return;
    }

    // set the defaults
    mus  = musictracksloaded[MENU_SONG];
    song = MENU_SONG;

    // try to grab the last song playing
    music_stack_pop(&mus, &song);

    if ( INVALID_SOUND == song )
    {
        // some wierd error
        Mix_HaltMusic();
    }
    else
    {
        if ( INVALID_SOUND != songplaying )
        {
            Mix_FadeOutMusic( fadetime );
        }

        // play the music
        Mix_FadeInMusic( mus, -1, fadetime );

        // set the volume
        Mix_VolumeMusic( snd.musicvolume );

        songplaying = song;
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
    snprintf( wavename, SDL_arraysize(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_GETCOIN] );
    g_wavelist[GSND_GETCOIN] = sound_load_chunk( wavename );

    snprintf( wavename, SDL_arraysize(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_DEFEND] );
    g_wavelist[GSND_DEFEND] = sound_load_chunk( wavename );

    snprintf( wavename, SDL_arraysize(wavename), "basicdat" SLASH_STR "globalparticles" SLASH_STR "%s", wavenames[GSND_COINFALL] );
    g_wavelist[GSND_COINFALL] = sound_load_chunk( wavename );

    snprintf( wavename, SDL_arraysize(wavename), "basicdat" SLASH_STR "%s", wavenames[GSND_LEVELUP] );
    g_wavelist[GSND_LEVELUP] = sound_load_chunk( wavename );

    snprintf( wavename, SDL_arraysize(wavename), "basicdat" SLASH_STR "%s", wavenames[GSND_PITFALL] );
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
        snprintf( wavename, SDL_arraysize(wavename), "%s" SLASH_STR "%s", tmploadname, wavenames[cnt] );

        ptmp = sound_load_chunk( wavename );
        if ( NULL == ptmp )
        {
            snprintf( wavename, SDL_arraysize(wavename), "%s" SLASH_STR "sound%d", tmploadname, cnt );
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
    STRING loadpath;
    STRING songname;
    vfs_FILE *playlist;
    Uint8 cnt;

    if ( musicinmemory || !snd.musicvalid ) return;

    // Open the playlist listing all music files
    playlist = vfs_openRead( "basicdat" SLASH_STR "music" SLASH_STR "playlist.txt" );
    if ( playlist == NULL )
    {
        log_warning( "Error opening playlist.txt\n" );
        return;
    }

    // Load the music data into memory
    for ( cnt = 0; cnt < MAXPLAYLISTLENGTH && !vfs_eof( playlist ); cnt++ )
    {
        if ( goto_colon( NULL, playlist, btrue ) )
        {
            fget_string( playlist, songname, SDL_arraysize(songname) );

            snprintf( loadpath, SDL_arraysize( loadpath), ( "basicdat" SLASH_STR "music" SLASH_STR "%s" ), songname );
            musictracksloaded[cnt] = Mix_LoadMUS( loadpath );
        }
    }
    musicinmemory = btrue;

    vfs_close( playlist );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t snd_config_init( snd_config_t * psnd )
{
    // Initialize the sound settings and set all values to default
    if ( NULL == psnd ) return bfalse;

    psnd->soundvalid        = bfalse;
    psnd->musicvalid        = bfalse;
    psnd->musicvolume       = 50;                            // The sound volume of music
    psnd->soundvolume       = 75;          // Volume of sounds played
    psnd->maxsoundchannel   = 16;      // Max number of sounds playing at the same time
    psnd->buffersize        = 2048;
    psnd->highquality       = bfalse;

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
        psnd->highquality     = pcfg->sound_highquality;
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void   LoopedList_init()
{
    // BB> setup the looped sound list
    int cnt;

    for (cnt = 0; cnt < LOOPED_COUNT; cnt++)
    {
        // clear out all of the data
        memset(LoopedList.lst + cnt, 0, sizeof(looped_sound_data_t) );

        LoopedList.lst[cnt].channel = -1;
        LoopedList.lst[cnt].chunk   = NULL;
        LoopedList.lst[cnt].object  = MAX_CHR;

        LoopedList.used_ref[cnt] = LOOPED_COUNT;
        LoopedList.free_ref[cnt] = cnt;
    }

    LoopedList.used_count = 0;
    LoopedList.free_count = LOOPED_COUNT;
}

//--------------------------------------------------------------------------------------------
bool_t LoopedList_validate()
{
    // BB> do the free and used indices have valid values?

    bool_t retval;

    retval = btrue;
    if ( LOOPED_COUNT != LoopedList.free_count + LoopedList.used_count )
    {
        // punt!
        LoopedList_clear();
        retval = bfalse;
    }

    return retval;
};

//--------------------------------------------------------------------------------------------
bool_t LoopedList_free_one( int index )
{
    // BB> free a looped sound only if it is actually being used
    Uint32 cnt;

    if ( !LoopedList_validate() ) return bfalse;

    // is the index actually free?
    for ( cnt = 0; cnt < LoopedList.used_count; cnt++ )
    {
        if ( index == LoopedList.used_ref[cnt] ) break;
    }

    // was anything found?
    if ( cnt >= LoopedList.used_count ) return bfalse;

    // swap the value with the one on the top of the stack
    SWAP(int, LoopedList.used_ref[cnt], LoopedList.used_ref[LoopedList.used_count-1] );
    LoopedList.used_count--;

    // push the value onto the free stack
    LoopedList.free_ref[LoopedList.free_count] = index;
    LoopedList.free_count++;

    // clear out the data
    LoopedList.lst[index].channel = INVALID_SOUND;
    LoopedList.lst[index].chunk   = NULL;
    LoopedList.lst[index].object  = MAX_CHR;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int LoopedList_get_free()
{
    int index;

    if ( !LoopedList_validate() ) return bfalse;

    LoopedList.free_count--;
    index = LoopedList.free_ref[LoopedList.free_count];

    // push the value onto the used stack
    LoopedList.used_ref[LoopedList.used_count] = index;
    LoopedList.used_count++;

    return index;
}

//--------------------------------------------------------------------------------------------
void LoopedList_clear()
{
    // BB> shut off all the looped sounds

    int cnt;

    for (cnt = 0; cnt < LOOPED_COUNT; cnt++)
    {
        if ( INVALID_SOUND != LoopedList.lst[cnt].channel )
        {
            Mix_FadeOutChannel( LoopedList.lst[cnt].channel, 500 );

            // clear out the data
            LoopedList.lst[cnt].channel = INVALID_SOUND;
            LoopedList.lst[cnt].chunk   = NULL;
            LoopedList.lst[cnt].object  = MAX_CHR;
        }
    }

    LoopedList_init();
}

//--------------------------------------------------------------------------------------------
int LoopedList_add( Mix_Chunk * sound, int loops, Uint16 object )
{
    // BB> add a looped sound to the list

    int channel;

    if ( NULL == sound )
    {
        // not a valid sound
        return INVALID_SOUND;
    }
    else if ( 0 == loops )
    {
        // not looped
        channel = Mix_PlayChannel( -1, sound, 0 );
    }
    else
    {
        // valid, looped sound

        int index;

        if ( LoopedList.used_count >= LOOPED_COUNT ) return -1;
        if ( !LoopedList_validate() ) return -1;

        index = LoopedList_get_free();

        if ( LOOPED_COUNT == index )
        {
            // there are no free indices, just play the sound once
            channel = Mix_PlayChannel( -1, sound, 0 );
        }
        else
        {
            // set up the LoopedList entry at the empty index

            channel = Mix_PlayChannel( -1, sound, loops );
            if ( INVALID_SOUND != channel )
            {
                LoopedList.lst[index].chunk   = sound;
                LoopedList.lst[index].channel = channel;
                LoopedList.lst[index].object  = object;
            }
            else
            {
                LoopedList_free_one( index );
            }
        }
    }

    return channel;
}

//--------------------------------------------------------------------------------------------
bool_t LoopedList_remove( int channel )
{
    // BB> remove a looped sound from the used list

    Uint32 cnt;
    bool_t retval;

    if ( 0 == LoopedList.used_count ) return bfalse;

    if ( !LoopedList_validate() ) return bfalse;

    retval = bfalse;
    for (cnt = 0; cnt < LoopedList.used_count; cnt++)
    {
        int index = LoopedList.used_ref[cnt];

        if ( channel == LoopedList.lst[index].channel )
        {
            retval = LoopedList_free_one(cnt);
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t _update_stereo_channel( int channel, GLvector3 diff )
{
    // BB> This updates the stereo image of a looped sound

    int       volume;

    if ( INVALID_SOUND == channel ) return bfalse;

    volume = _calculate_volume( diff );
    return _update_channel_volume( channel, volume, diff );
}

//--------------------------------------------------------------------------------------------
void looped_update_all_sound()
{
    Uint32 cnt;

    for (cnt = 0; cnt < LoopedList.used_count; cnt++)
    {
        GLvector3 diff;
        int       index;
        looped_sound_data_t * plooped;

        index = LoopedList.used_ref[cnt];

        if ( index < 0 || index >= LOOPED_COUNT ) continue;
        if ( -1 == LoopedList.lst[index].channel   ) continue;
        plooped = LoopedList.lst + index;

        if ( INVALID_CHR( plooped->object ) )
        {
            // not a valid object
            GLvector3 diff = VECT3(0, 0, 0);

            _update_stereo_channel( plooped->channel, diff );
        }
        else
        {
            // make the sound stick to the object
            diff = VSub( ChrList.lst[plooped->object].pos, PCamera->track_pos );

            _update_stereo_channel( plooped->channel, diff );
        }
    }

}

//--------------------------------------------------------------------------------------------
bool_t looped_stop_object_sounds( Uint16 ichr )
{
    // BB> free any looped sound(s) being made by a certain character
    int freed;
    Uint32 cnt;
    bool_t found;

    if ( MAX_CHR == ichr ) return bfalse;

    // we have to do this a funny way, because it is hard to guarantee how the
    // "delete"/"free" function LoopedList_free_one() will free an entry, and because
    // each object could have multiple looped sounds

    freed = 0;
    found = btrue;
    while ( found && LoopedList.used_count > 0 )
    {
        found = bfalse;
        for (cnt = 0; cnt < LoopedList.used_count; cnt++)
        {
            int index = LoopedList.used_ref[cnt];

            if ( LoopedList.lst[cnt].object == ichr )
            {
                if ( LoopedList_free_one( index ) )
                {
                    freed++;
                    found = btrue;
                    break;
                }
            }
        }
    }

    return freed > 0;
}

//--------------------------------------------------------------------------------------------
void sound_finish_sound()
{
    Mix_FadeOutChannel( -1, 500 );     // Stop all in-game sounds that are playing
    sound_finish_song( 500 );          // Fade out the existing song and pop the music stack
}


//--------------------------------------------------------------------------------------------
void sound_free_chunk( Mix_Chunk * pchunk )
{
	if( mixeron )
	{
		Mix_FreeChunk(pchunk);
	}
}