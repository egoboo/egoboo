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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/sound.c
/// @brief Sound code in Egoboo is implemented using SDL_mixer.
/// @details

#include "game/sound.h"
#include "game/camera_system.h"
#include "game/game.h"
#include "game/graphic.h"
#include "game/char.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_looped_sound_data;
typedef struct s_looped_sound_data looped_sound_data_t;

struct s_music_stack_element;
typedef struct s_music_stack_element music_stack_element_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LOOPED_COUNT 256

#define INVALID_LOOPED_IDX LOOPED_COUNT
#define INVALID_LOOPED_REF (( LOOP_REF )INVALID_LOOPED_IDX)

#define VALID_LOOPED_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<LOOPED_COUNT) )

#define MUSIC_STACK_COUNT 20

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data needed to store and manipulate a looped sound
struct s_looped_sound_data
{
    int               channel;
    const Mix_Chunk * chunk;
    CHR_REF           object;
};
looped_sound_data_t * looped_sound_data__init( looped_sound_data_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// define a little stack for interrupting music sounds with other music

/// The data needed to store a dsingle music track on the music_stack[]
struct s_music_stack_element
{
    Mix_Music * mus;
    int         number;
};

static bool                music_stack_pop( Mix_Music ** mus, int * song );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST_STATIC( looped_sound_data_t, LoopedList, LOOPED_COUNT );

static void   LoopedList_init();
static void   LoopedList_clear();

static bool LoopedList_free_one( size_t index );
static size_t LoopedList_get_free_ref();

static bool LoopedList_validate();
static size_t LoopedList_add( const Mix_Chunk * sound, int loops, const CHR_REF  object );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool sdl_audio_initialize();
static bool sdl_mixer_initialize();
static void   sdl_mixer_quit();

static int    _calculate_volume( const fvec3_base_t cam_pos, const fvec3_base_t cam_center, const fvec3_base_t diff, const float fov_rad );
static bool _update_stereo_channel( int channel, const fvec3_base_t cam_pos, const fvec3_base_t cam_center, const fvec3_base_t diff, const float pan );
static bool _update_channel_volume( int channel, const int volume, const float pan );
static bool _calculate_average_camera_stereo( const fvec3_base_t snd_pos, fvec3_base_t cam_pos, fvec3_base_t cam_center, fvec3_base_t diff, float * pan_ptr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Sound using SDL_Mixer
static bool mixeron         = false;

// text filenames for the global sounds
static const char * wavenames[GSND_COUNT] =
{
    "coinget",
    "defend",
    "weather1",
    "weather2",
    "coinfall",
    "lvlup",
    "pitfall",
    "shieldblock"
};

static bool sound_atexit_registered = false;

static int                   music_stack_depth = 0;
static music_stack_element_t music_stack[MUSIC_STACK_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

snd_config_t snd;

// music
bool      musicinmemory = false;
Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];
int         songplaying   = INVALID_SOUND;

Mix_Chunk * g_wavelist[GSND_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool sound_system_config_init( snd_config_t * psnd );
static void music_stack_init();
static bool music_stack_push( Mix_Music * mus, int song );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( looped_sound_data_t, LoopedList, LOOPED_COUNT );

//--------------------------------------------------------------------------------------------
// music_stack
//--------------------------------------------------------------------------------------------
static void music_stack_finished_callback()
{
    // this function is only called when a music function is finished playing
    // pop the saved music off the stack

    // unfortunately, it seems that SDL_mixer does not support saving the position of
    // the music stream, so the music track will restart from the beginning

    Mix_Music * mus;
    int         song;

    // grab the next song
    if ( music_stack_pop( &mus, &song ) )
    {
        // play the music
        Mix_PlayMusic( mus, 0 );

        songplaying = song;

        // set the volume
        Mix_VolumeMusic( snd.musicvolume );
    }
}

//--------------------------------------------------------------------------------------------
bool music_stack_push( Mix_Music * mus, int song )
{
    if ( music_stack_depth >= MUSIC_STACK_COUNT - 1 )
    {
        music_stack_depth = MUSIC_STACK_COUNT - 1;
        return false;
    }

    music_stack[music_stack_depth].mus    = mus;
    music_stack[music_stack_depth].number = song;

    music_stack_depth++;

    return true;
}

//--------------------------------------------------------------------------------------------
bool music_stack_pop( Mix_Music ** mus, int * song )
{
    if ( NULL == mus || NULL == song ) return false;

    // fail if music isn't loaded
    if ( !musicinmemory )
    {
        *mus = NULL;
        *song = INVALID_SOUND;
        return false;
    }

    // set the default to be song 0
    *song = 0;
    *mus  = musictracksloaded[*song];

    // pop the stack, if possible
    if ( music_stack_depth > 0 )
    {
        music_stack_depth--;

        *mus  = music_stack[music_stack_depth].mus;
        *song = music_stack[music_stack_depth].number;
    }

    return true;
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
// SDL
//--------------------------------------------------------------------------------------------
bool sdl_audio_initialize()
{
    bool retval = true;

    // make sure that SDL audio is turned on
    if ( 0 == SDL_WasInit( SDL_INIT_AUDIO ) )
    {
        log_info( "Intializing SDL Audio... " );
        if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
        {
            log_message( "Failed!\n" );
            log_warning( "SDL error == \"%s\"\n", SDL_GetError() );

            retval = false;
            snd.musicvalid = false;
            snd.soundvalid = false;
        }
        else
        {
            retval = true;
            log_message( "Success!\n" );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool sdl_mixer_initialize()
{
    /// @author ZF
    /// @details This intitializes the SDL_mixer services

    if ( !mixeron && ( snd.musicvalid || snd.soundvalid ) )
    {
        const SDL_version* link_version = Mix_Linked_Version();
        log_info( "Initializing SDL_mixer audio services version %d.%d.%d... ", link_version->major, link_version->minor, link_version->patch );
        if ( Mix_OpenAudio( cfg.sound_highquality_base ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, snd.buffersize ) < 0 )
        {
            mixeron = false;
            log_message( "Failure!\n" );
            log_warning( "Unable to initialize audio: %s\n", Mix_GetError() );
        }
        else
        {
            Mix_VolumeMusic( snd.musicvolume );
            Mix_AllocateChannels( snd.maxsoundchannel );

            // initialize the music stack
            music_stack_init();

            mixeron = true;

            atexit( sdl_mixer_quit );
            sound_atexit_registered = true;

            log_message( "Success!\n" );
        }
    }

    return mixeron;
}

//--------------------------------------------------------------------------------------------
void sdl_mixer_quit()
{
    if ( mixeron && ( 0 != SDL_WasInit( SDL_INIT_AUDIO ) ) )
    {
        Mix_CloseAudio();
        mixeron = false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This function enables the use of SDL_Audio and SDL_Mixer functions, returns true if success
bool sound_system_initialize()
{
    bool retval = false;
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
void sound_system_restart()
{
    //if ( mixeron )
    {
        Mix_CloseAudio();
        mixeron = false;
    }

    // loose the info on the currently playing song
    if ( snd.musicvalid || snd.soundvalid )
    {
        if ( -1 != Mix_OpenAudio( cfg.sound_highquality_base ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, snd.buffersize ) )
        {
            mixeron = true;
            Mix_AllocateChannels( snd.maxsoundchannel );
            Mix_VolumeMusic( snd.musicvolume );

            // initialize the music stack
            music_stack_init();

            if ( !sound_atexit_registered )
            {
                atexit( sdl_mixer_quit );
                sound_atexit_registered = true;
            }
        }
        else
        {
            log_warning( "sound_system_restart() - Cannot get the sound module to restart. (%s)\n", Mix_GetError() );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool sound_system_config_init( snd_config_t * psnd )
{
    // Initialize the sound settings and set all values to default
    if ( NULL == psnd ) return false;

    psnd->soundvalid        = false;
    psnd->musicvalid        = false;
    psnd->musicvolume       = 50;                            // The sound volume of music
    psnd->soundvolume       = 75;          // Volume of sounds played
    psnd->maxsoundchannel   = 16;      // Max number of sounds playing at the same time
    psnd->buffersize        = 2048;
    psnd->highquality       = false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool sound_system_download_from_config( snd_config_t * psnd, egoboo_config_t * pcfg )
{
    if ( NULL == psnd && NULL == pcfg ) return false;

    if ( NULL == pcfg )
    {
        return sound_system_config_init( psnd );
    }

    // coerce pcfg to have valid values
    pcfg->sound_channel_count = CLIP( pcfg->sound_channel_count, (Uint16)8, (Uint16)128 );
    pcfg->sound_buffer_size   = CLIP( pcfg->sound_buffer_size, (Uint16)512, (Uint16)8196 );

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

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Mix_Chunk * sound_load_chunk_vfs( const char * szFileName )
{
    STRING      full_file_name;
    Mix_Chunk * tmp_chunk;
    bool      file_exists = false;

    if ( !mixeron ) return NULL;
    if ( INVALID_CSTR( szFileName ) ) return NULL;

    // blank out the data
    tmp_chunk = NULL;

    // try an ogg file
    snprintf( full_file_name, SDL_arraysize( full_file_name ), "%s.%s", szFileName, "ogg" );
    if ( vfs_exists( full_file_name ) )
    {
        file_exists = true;
        tmp_chunk = Mix_LoadWAV( vfs_resolveReadFilename( full_file_name ) );
    }

    if ( NULL == tmp_chunk )
    {
        // try a wav file
        snprintf( full_file_name, SDL_arraysize( full_file_name ), "%s.%s", szFileName, "wav" );
        if ( vfs_exists( full_file_name ) )
        {
            file_exists = true;
            tmp_chunk = Mix_LoadWAV( vfs_resolveReadFilename( full_file_name ) );
        }
    }

    if ( file_exists && NULL == tmp_chunk )
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
    bool      file_exists = false;

    if ( !mixeron ) return NULL;
    if ( INVALID_CSTR( szFileName ) ) return NULL;

    // blank out the data
    tmp_music = NULL;

    // try a wav file
    snprintf( full_file_name, SDL_arraysize( full_file_name ), "%s.%s", szFileName, "wav" );
    if ( vfs_exists( full_file_name ) )
    {
        file_exists = true;
        tmp_music = Mix_LoadMUS( full_file_name );
    }

    if ( NULL == tmp_music )
    {
        // try an ogg file
        tmp_music = NULL;
        snprintf( full_file_name, SDL_arraysize( full_file_name ), "%s.%s", szFileName, "ogg" );
        if ( vfs_exists( full_file_name ) )
        {
            file_exists = true;
            tmp_music = Mix_LoadMUS( full_file_name );
        }
    }

    if ( file_exists && NULL == tmp_music )
    {
        // there is an error only if the file exists and can't be loaded
        log_warning( "Music file not found/loaded %s.\n", szFileName );
    }

    return tmp_music;
}

//--------------------------------------------------------------------------------------------
bool sound_load( mix_ptr_t * pptr, const char * szFileName, mix_type_t type )
{

    if ( !mixeron ) return false;
    if ( NULL == pptr ) return false;

    // clear out the data
    pptr->ptr.unk = NULL;
    pptr->type    = MIX_UNKNOWN;

    if ( INVALID_CSTR( szFileName ) ) return false;

    switch ( type )
    {
        case MIX_MUS:
            pptr->ptr.mus = sound_load_music( szFileName );
            if ( NULL != pptr->ptr.mus )
            {
                pptr->type = MIX_MUS;
            }
            break;

        case MIX_SND:
            pptr->ptr.snd = sound_load_chunk_vfs( szFileName );
            if ( NULL != pptr->ptr.snd )
            {
                pptr->type = MIX_SND;
            }
            break;

        case MIX_UNKNOWN:
            /* do nothing */
            break;

        default:
            // there is an error only if the file exists and can't be loaded
            log_debug( "sound_load() - Mix type recognized %d.\n", type );
            break;
    };

    return MIX_UNKNOWN != pptr->type;
}

//--------------------------------------------------------------------------------------------
int sound_play_mix( fvec3_base_t pos, mix_ptr_t * pptr )
{
    int retval = INVALID_SOUND_CHANNEL;
    if ( !snd.soundvalid || !mixeron )
    {
        return INVALID_SOUND_CHANNEL;
    }
    if ( NULL == pptr || MIX_UNKNOWN == pptr->type || NULL == pptr->ptr.unk )
    {
        log_debug( "Unable to load sound. (%s)\n", Mix_GetError() );
        return INVALID_SOUND_CHANNEL;
    }

    retval = INVALID_SOUND_CHANNEL;
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
void sound_fade_all()
{
    if ( mixeron )
    {
        Mix_FadeOutChannel( -1, 500 );     // Stop all sounds that are playing
    }
}

//--------------------------------------------------------------------------------------------
void fade_in_music( Mix_Music * music )
{
    if ( mixeron )
    {
        Mix_FadeInMusic( music, -1, 500 );
    }
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
    if ( mixeron )
    {
        Mix_FreeChunk( pchunk );
    }
}

//--------------------------------------------------------------------------------------------
int get_current_song_playing()
{
    /// @author ZF
    /// @details This gives read access to the private variable 'songplaying'

    return songplaying;
}

//--------------------------------------------------------------------------------------------
// chunk stuff
//--------------------------------------------------------------------------------------------
int sound_play_chunk_looped( const fvec3_base_t snd_pos, const Mix_Chunk * pchunk, const int loops, const CHR_REF owner )
{
    /// @author ZF
    /// @details This function plays a specified sound and returns which channel it's using
    int channel = INVALID_SOUND_CHANNEL;

    fvec3_t cam_pos, cam_center, diff;
    float pan;
    int volume;

    if ( !snd.soundvalid || !mixeron || NULL == pchunk ) return INVALID_SOUND_CHANNEL;

    // only play sound effects if the game is running
    if ( !process_running( PROC_PBASE( GProc ) ) )  return INVALID_SOUND_CHANNEL;

    // measure the distance in tiles
    _calculate_average_camera_stereo( snd_pos, cam_pos.v, cam_center.v, diff.v, &pan );

    volume = _calculate_volume( cam_pos.v, cam_center.v, diff.v, DEG_TO_RAD( camera_t::DEFAULT_FOV ) );

    // play the sound
    if ( volume > 0 )
    {
        // play the sound
        channel = Mix_PlayChannel( -1, ( Mix_Chunk * )pchunk, loops );

        if ( INVALID_SOUND_CHANNEL == channel )
        {
            /// @note ZF@> disabled this warning because this happens really often
            //log_debug( "Unable to play sound. (%s)\n", Mix_GetError() );
        }
        else
        {
            if ( 0 != loops )
            {
                // add the sound to the LoopedList
                LoopedList_add( pchunk, channel, owner );
            }

            //Set left/right panning
            _update_channel_volume( channel, volume, pan );
        }
    }

    return channel;
}

//--------------------------------------------------------------------------------------------
int sound_play_chunk_full( Mix_Chunk * pchunk )
{
    /// @author ZF
    /// @details This function plays a specified sound at full possible volume and returns which channel it's using
    int channel = INVALID_SOUND_CHANNEL;

    if ( !snd.soundvalid || !mixeron || NULL == pchunk ) return INVALID_SOUND_CHANNEL;

    // only play sound effects if the game is running
    if ( !process_running( PROC_PBASE( GProc ) ) )  return INVALID_SOUND_CHANNEL;

    // play the sound
    channel = Mix_PlayChannel( -1, pchunk, 0 );

    // we are still limited by the global sound volume
    Mix_Volume( channel, ( 128*snd.soundvolume ) / 100 );

    return channel;
}

//--------------------------------------------------------------------------------------------
void sound_stop_channel( int whichchannel )
{
    /// @author ZF
    /// @details Stops a sound effect playing in the specified channel
    if ( mixeron && snd.soundvalid )
    {
        Mix_HaltChannel( whichchannel );
    }
}

//--------------------------------------------------------------------------------------------
// song stuff
//--------------------------------------------------------------------------------------------
void sound_play_song( int songnumber, Uint16 fadetime, int loops )
{
    /// @author ZF
    /// @details This functions plays a specified track loaded into memory
    if ( !snd.musicvalid || !mixeron ) return;

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
    music_stack_pop( &mus, &song );

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
    /// @author ZF
    /// @details This function sets music track to pause
    if ( mixeron && snd.musicvalid )
    {
        Mix_HaltMusic();
    }
}

//--------------------------------------------------------------------------------------------
void sound_load_global_waves_vfs()
{
    /// @author ZZ
    /// @details This function loads the global waves

    STRING wavename;
    int cnt;

    // Grab these sounds from the basicdat dir
    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_GETCOIN] );
    g_wavelist[GSND_GETCOIN] = sound_load_chunk_vfs( wavename );

    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_DEFEND] );
    g_wavelist[GSND_DEFEND] = sound_load_chunk_vfs( wavename );

    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_COINFALL] );
    g_wavelist[GSND_COINFALL] = sound_load_chunk_vfs( wavename );

    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_LEVELUP] );
    g_wavelist[GSND_LEVELUP] = sound_load_chunk_vfs( wavename );

    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_PITFALL] );
    g_wavelist[GSND_PITFALL] = sound_load_chunk_vfs( wavename );

    snprintf( wavename, SDL_arraysize( wavename ), "mp_data/%s", wavenames[GSND_SHIELDBLOCK] );
    g_wavelist[GSND_SHIELDBLOCK] = sound_load_chunk_vfs( wavename );

    for ( cnt = 0; cnt < GSND_COUNT; cnt++ )
    {
        Mix_Chunk * ptmp;

        snprintf( wavename, SDL_arraysize( wavename ), "mp_data/sound%d", cnt );
        ptmp = sound_load_chunk_vfs( wavename );

        // only overwrite with a valid sound file
        if ( NULL != ptmp )
        {
            g_wavelist[cnt] = ptmp;
        }
    }
}

//--------------------------------------------------------------------------------------------
void sound_load_all_music_sounds_vfs()
{
    /// @author ZF
    /// @details This function loads all of the music sounds
    STRING loadpath;
    STRING songname;
    vfs_FILE *playlist;
    Uint8 cnt;

    if ( musicinmemory || !snd.musicvalid ) return;

    // Open the playlist listing all music files
    snprintf( loadpath, SDL_arraysize( loadpath ), "mp_data/music/playlist.txt" );
    playlist = vfs_openRead( loadpath );
    if ( NULL == playlist )
    {
        log_warning( "Error reading music list. (%s)\n", loadpath );
        return;
    }

    // Load the music data into memory
    for ( cnt = 0; cnt < MAXPLAYLISTLENGTH && !vfs_eof( playlist ); cnt++ )
    {
        if ( goto_colon_vfs( NULL, playlist, true ) )
        {
            vfs_get_string( playlist, songname, SDL_arraysize( songname ) );

            snprintf( loadpath, SDL_arraysize( loadpath ), ( "mp_data/music/%s" ), songname );
            musictracksloaded[cnt] = Mix_LoadMUS( vfs_resolveReadFilename( loadpath ) );
        }
    }
    musicinmemory = true;

    //Special xmas theme, override the default menu theme song
    if ( check_time( SEASON_CHRISTMAS ) )
    {
        snprintf( loadpath, SDL_arraysize( loadpath ), ( "mp_data/music/special/xmas.ogg" ) );
        musictracksloaded[MENU_SONG] = Mix_LoadMUS( vfs_resolveReadFilename( loadpath ) );
    }
    else if ( check_time( SEASON_HALLOWEEN ) )
    {
        snprintf( loadpath, SDL_arraysize( loadpath ), ( "mp_data/music/special/halloween.ogg" ) );
        musictracksloaded[MENU_SONG] = Mix_LoadMUS( vfs_resolveReadFilename( loadpath ) );
    }

    // A small helper for us developers
    if ( cnt == MAXPLAYLISTLENGTH )
    {
        log_debug( "Play list is full. Consider increasing MAXPLAYLISTLENGTH (currently %i).", MAXPLAYLISTLENGTH );
    }

    vfs_close( playlist );
}

//--------------------------------------------------------------------------------------------
// LoopedList
//--------------------------------------------------------------------------------------------
void   LoopedList_init()
{
    /// @author BB
    /// @details setup the looped sound list
    LOOP_REF cnt;
    size_t tnc;

    for ( cnt = 0; cnt < LOOPED_COUNT; cnt++ )
    {
        // clear out all of the data
        looped_sound_data__init( LoopedList.lst + cnt );

        tnc = REF_TO_INT( cnt );
        LoopedList.used_ref[tnc] = INVALID_LOOPED_IDX;
        LoopedList.free_ref[tnc] = tnc;
    }

    LoopedList.used_count = 0;
    LoopedList.free_count = LOOPED_COUNT;
}

//--------------------------------------------------------------------------------------------
bool LoopedList_validate()
{
    /// @author BB
    /// @details do the free and used indices have valid values?

    bool retval;

    retval = true;
    if ( LOOPED_COUNT != LoopedList.free_count + LoopedList.used_count )
    {
        // punt!
        LoopedList_clear();
        retval = false;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool LoopedList_free_one( size_t index )
{
    /// @author BB
    /// @details free a looped sound only if it is actually being used
    int   cnt;
    LOOP_REF ref;

    if ( !LoopedList_validate() ) return false;

    // is the index actually free?
    for ( cnt = 0; cnt < LoopedList.used_count; cnt++ )
    {
        if ( index == LoopedList.used_ref[cnt] ) break;
    }

    // was anything found?
    if ( cnt >= LoopedList.used_count ) return false;

    // swap the value with the one on the top of the stack
    SWAP( size_t, LoopedList.used_ref[cnt], LoopedList.used_ref[LoopedList.used_count-1] );

    LoopedList.used_count--;
    LoopedList.update_guid++;

    LoopedList_push_free( index );

    // clear out the data
    ref = ( LOOP_REF )index;
    LoopedList.lst[ref].channel = INVALID_SOUND_CHANNEL;
    LoopedList.lst[ref].chunk   = NULL;
    LoopedList.lst[ref].object  = INVALID_CHR_REF;

    return true;
}

//--------------------------------------------------------------------------------------------
size_t LoopedList_pop_free( const int idx )
{
    size_t index = INVALID_LOOPED_IDX;
    size_t loops = 0;

    while ( LoopedList.free_count > 0 )
    {
        LoopedList.free_count--;
        LoopedList.update_guid++;

        index = LoopedList.free_ref[LoopedList.free_count];
        if ( VALID_LOOPED_RANGE( index ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %d loops.\n", __FUNCTION__, loops );
    }

    return index;
}

//--------------------------------------------------------------------------------------------
bool LoopedList_push_free( const REF_T index )
{
    if ( LoopedList.free_count >= LOOPED_COUNT ) return false;

    LoopedList.free_ref[LoopedList.free_count] = index;

    LoopedList.free_count++;
    LoopedList.update_guid++;

    return true;
}

//--------------------------------------------------------------------------------------------
bool LoopedList_push_used( const REF_T index )
{
    if ( LoopedList.used_count >= LOOPED_COUNT ) return false;

    LoopedList.used_ref[LoopedList.used_count] = index;

    LoopedList.used_count++;
    LoopedList.update_guid++;

    return true;
}

//--------------------------------------------------------------------------------------------
size_t LoopedList_pop_used( const int idx )
{
    size_t index = INVALID_LOOPED_IDX;
    size_t loops = 0;

    while ( LoopedList.used_count > 0 )
    {
        LoopedList.used_count--;
        LoopedList.update_guid++;

        index = LoopedList.used_ref[LoopedList.used_count];
        if ( VALID_LOOPED_RANGE( index ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the used stack. %d loops.\n", __FUNCTION__, loops );
    }

    return index;
}

//--------------------------------------------------------------------------------------------
size_t LoopedList_get_free_ref()
{
    size_t index;

    if ( !LoopedList_validate() ) return false;

    // try to get a free value
    index = LoopedList_pop_free( -1 );

    // if the index is an error value, return an error value
    if ( !VALID_LOOPED_RANGE( index ) ) return index;

    // try to push the value onto the used stack
    if ( !LoopedList_push_used( index ) )
    {
        // put it back on the free stack
        LoopedList_push_free( index );

        // return an error value
        index = INVALID_LOOPED_IDX;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
void LoopedList_clear()
{
    /// @author BB
    /// @details shut off all the looped sounds

    LOOP_REF cnt;

    for ( cnt = 0; cnt < LOOPED_COUNT; cnt++ )
    {
        if ( INVALID_SOUND_CHANNEL != LoopedList.lst[cnt].channel )
        {
            Mix_FadeOutChannel( LoopedList.lst[cnt].channel, 500 );

            // clear out the data
            LoopedList.lst[cnt].channel = INVALID_SOUND_CHANNEL;
            LoopedList.lst[cnt].chunk   = NULL;
            LoopedList.lst[cnt].object  = INVALID_CHR_REF;
        }
    }

    LoopedList_init();
}

//--------------------------------------------------------------------------------------------
size_t LoopedList_add( const Mix_Chunk * sound, int channel, const CHR_REF  ichr )
{
    /// @author BB
    /// @details add a looped sound to the list

    size_t index;

    if ( NULL == sound || INVALID_SOUND_CHANNEL == channel || !INGAME_CHR( ichr ) )
    {
        return INVALID_LOOPED_IDX;
    }

    if ( LoopedList.used_count >= LOOPED_COUNT )
    {
        return INVALID_LOOPED_IDX;
    }

    if ( !LoopedList_validate() )
    {
        return INVALID_LOOPED_IDX;
    }

    index = LoopedList_get_free_ref();
    if ( VALID_LOOPED_RANGE( index ) )
    {
        // set up the LoopedList entry at the empty index
        LOOP_REF ref = ( LOOP_REF )index;

        LoopedList.lst[ref].chunk   = sound;
        LoopedList.lst[ref].channel = channel;
        LoopedList.lst[ref].object  = ichr;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
bool LoopedList_remove( int channel )
{
    /// @author BB
    /// @details remove a looped sound from the used list

    int cnt;
    bool retval;

    if ( 0 == LoopedList.used_count ) return false;

    if ( !LoopedList_validate() ) return false;

    retval = false;
    for ( cnt = 0; cnt < LoopedList.used_count; cnt++ )
    {
        size_t index = LoopedList.used_ref[cnt];

        if ( VALID_LOOPED_RANGE( index ) )
        {
            LOOP_REF ref = ( LOOP_REF ) index;

            if ( channel == LoopedList.lst[ref].channel )
            {
                retval = LoopedList_free_one( cnt );
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// looped
//--------------------------------------------------------------------------------------------
void looped_update_all_sound()
{
    int cnt;

    for ( cnt = 0; cnt < LoopedList.used_count; cnt++ )
    {
        size_t    index;
        LOOP_REF   ref;
        looped_sound_data_t * plooped;

        index = LoopedList.used_ref[cnt];
        if ( !VALID_LOOPED_RANGE( index ) ) continue;

        ref = ( LOOP_REF )index;

        if ( INVALID_SOUND_CHANNEL == LoopedList.lst[ref].channel ) continue;
        plooped = LoopedList.lst + ref;

        if ( !INGAME_CHR( plooped->object ) )
        {
            // not a valid object
            fvec3_t   diff_tmp   = VECT3( 0, 0, 0 );
            fvec3_t   cam_pos_tmp = VECT3( 0, 0, 0 );
            fvec3_t   cam_center_tmp = VECT3( 0, 0, 0 );

            _update_stereo_channel( plooped->channel, cam_pos_tmp.v, cam_center_tmp.v, diff_tmp.v, 0.0f );
        }
        else
        {
            float pan;
            fvec3_t cam_pos, cam_center, diff;

            // make the sound stick to the object

            // get the stereo parameters for multiple cameras (yuck!)
            _calculate_average_camera_stereo( ChrList.lst[plooped->object].pos.v, cam_pos.v, cam_center.v, diff.v, &pan );

            // update the stereo channels
            _update_stereo_channel( plooped->channel, cam_pos.v, cam_center.v, diff.v, pan );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool looped_stop_object_sounds( const CHR_REF  ichr )
{
    /// @author BB
    /// @details free any looped sound(s) being made by a certain character
    int freed;
    int cnt;
    bool found;

    if ( !ALLOCATED_CHR( ichr ) ) return false;

    // we have to do this a funny way, because it is hard to guarantee how the
    // "delete"/"free" function LoopedList_free_one() will free an entry, and because
    // each object could have multiple looped sounds

    freed = 0;
    found = true;
    while ( found && LoopedList.used_count > 0 )
    {
        found = false;
        for ( cnt = 0; cnt < LoopedList.used_count; cnt++ )
        {
            LOOP_REF ref;

            size_t index = LoopedList.used_ref[cnt];
            if ( !VALID_LOOPED_RANGE( index ) ) continue;

            ref = ( LOOP_REF )index;

            if ( ichr == LoopedList.lst[ref].object )
            {
                int channel = LoopedList.lst[ref].channel;

                if ( LoopedList_free_one( index ) )
                {
                    freed++;
                    found = true;
                    sound_stop_channel( channel );
                    break;
                }
            }
        }
    }

    return freed > 0;
}

//--------------------------------------------------------------------------------------------
// helper functions
//--------------------------------------------------------------------------------------------

int _calculate_volume( const fvec3_base_t cam_pos, const fvec3_base_t cam_center, const fvec3_base_t diff, float const fov_rad )
{
    /// @author BB
    /// @details This calculates the volume a sound should have depending on
    ///  the distance from the camera

    float lookat_dist2, diff_dist2;
    int volume;
    float render_radius2, tan_val;

    // approximate the radius of the area that the camera sees
    lookat_dist2 = fvec3_dist_2( cam_center, cam_pos ); // * fvec3_length_2( eye_vec );
    diff_dist2   = fvec3_length_2( diff );

    tan_val = TAN( fov_rad * 0.5f );
    render_radius2 = lookat_dist2 * tan_val * tan_val;

    // adjust for the listen skill
    if ( local_stats.listening_level ) diff_dist2 *= 0.66f * 0.66f;

    if ( 0.0f == render_radius2 + diff_dist2 )
    {
        volume = 255;
    }
    else
    {
        volume  = 255 * render_radius2 / ( render_radius2 + diff_dist2 );
    }

    volume  = ( volume * snd.soundvolume ) / 100;

    return CLIP( volume, 0, 255 );
}

//--------------------------------------------------------------------------------------------
bool _update_channel_volume( int channel, const int volume, const float pan )
{
    float loc_pan = pan, loc_volume = volume;
    float cosval;
    int leftvol, rightvol;

    // determine the angle away from "forward"
    loc_volume *= ( 2.0f + COS( loc_pan ) ) / 3.0f;

    // determine the angle from the left ear
    loc_pan += 1.5f * PI;

    // determine the panning
    cosval = cos( loc_pan );
    cosval *= cosval;

    leftvol  = cosval * 128;
    rightvol = 128 - leftvol;

    leftvol  = (( 127 + leftvol ) * volume ) >> 8;
    rightvol = (( 127 + rightvol ) * volume ) >> 8;

    // apply the volume adjustments
    Mix_SetPanning( channel, leftvol, rightvol );

    return true;
}

//--------------------------------------------------------------------------------------------
bool _calculate_average_camera_stereo( const fvec3_base_t pos, fvec3_base_t cam_pos, fvec3_base_t cam_center, fvec3_base_t diff, float * pan_ptr )
{
    int cam_count;
    fvec2_t pan_diff;
    ext_camera_iterator_t * it;
    ext_camera_list_t * pclst;

    if ( NULL == pos ) return false;

    // initialize the values
    fvec3_self_clear( cam_pos );
    fvec3_self_clear( cam_center );
    fvec3_self_clear( diff );
    fvec2_self_clear( pan_diff.v );

    // get the camera list
    pclst = camera_system_get_list();

    // iterate over all cameras
    cam_count = 0;
    for ( it = camera_list_iterator_begin( pclst ); NULL != it; it = camera_list_iterator_next( it ) )
    {
        fvec3_t tmp_diff;

        camera_t * pcam = camera_list_iterator_get_camera( it );
        if ( NULL == pcam ) continue;

        // how many cameras?
        cam_count++;

        // find the difference relative to this camera
        fvec3_sub( tmp_diff.v, pos, pcam->center.v );

        // sum up the differences
        fvec3_self_sum( diff, tmp_diff.v );

        // sum up the center positions
        fvec3_self_sum( cam_pos, pcam->pos.v );
        fvec3_self_sum( cam_center, pcam->center.v );

        // if pan is required...
        if ( NULL != pan_ptr )
        {
            float diff2, inv_diff_len;
            float cam_sin, cam_cos;
            fvec2_t norm_diff, cam_diff, tmp;

            // calculate the camera trig functions
            cam_sin = SIN( pcam->turn_z_rad );
            cam_cos = COS( pcam->turn_z_rad );

            // get the distance squared
            diff2 = fvec2_length_2( tmp_diff.v );

            if ( diff2 > 0.0f )
            {
                float inv_diff2 = 1.0f / diff2;

                // fall to half in one grid unit
                float pan_wt    = 1.0f / ( 1.0f + diff2 / GRID_FSIZE / GRID_FSIZE );

                // normalize the difference vector in 2d
                inv_diff_len = std::sqrt( inv_diff2 );
                fvec2_scale( norm_diff.v, tmp_diff.v, inv_diff_len );

                // measure the diff relative to the camera direction
                cam_diff.x = norm_diff.x * cam_cos + norm_diff.y * cam_sin;
                cam_diff.y = norm_diff.y * cam_cos - norm_diff.x * cam_sin;

                // weight the pan based on the volume at the destination
                fvec2_scale( tmp.v, cam_diff.v, pan_wt );

                // sum this vector
                fvec2_self_sum( pan_diff.v, tmp.v );
            }
        }
    }
    it = camera_list_iterator_end( it );

    // get the average
    if ( cam_count > 1 )
    {
        fvec3_self_scale( diff, 1.0f / cam_count );
        fvec3_self_scale( cam_pos, 1.0f / cam_count );
        fvec3_self_scale( cam_center, 1.0f / cam_count );
    }

    // find the net direction
    if ( NULL != pan_ptr )
    {
        *pan_ptr = ATAN2( pan_diff.y, pan_diff.x );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool _update_stereo_channel( int channel, const fvec3_base_t cam_pos, const fvec3_base_t cam_center, const fvec3_base_t diff, const float pan )
{
    /// @author BB
    /// @details This updates the stereo image of a looped sound

    int       volume;

    if ( INVALID_SOUND_CHANNEL == channel ) return false;

    volume = _calculate_volume( cam_pos, cam_center, diff, DEG_TO_RAD( camera_t::DEFAULT_FOV ) );

    return _update_channel_volume( channel, volume, pan );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
looped_sound_data_t * looped_sound_data__init( looped_sound_data_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->channel = INVALID_SOUND_CHANNEL;
    ptr->chunk   = NULL;
    ptr->object  = INVALID_CHR_REF;

    return ptr;
}