#pragma once

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

/// @file sound.h
/// @Sound handling using SDL_mixer

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#include <SDL_mixer.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_egoboo_config;
struct s_renderlist;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAX_WAVE              30           ///< Up to 30 wave/ogg per model
#define MAXPLAYLISTLENGTH     35           ///< Max number of different tracks loaded into memory
#define INVALID_SOUND         -1           ///< The index of this sound is not valid
#define INVALID_SOUND_CHANNEL -1           ///< SDL_mixer sound channel is invalid
#define MENU_SONG              0           ///< default music theme played when in the menu

/// frequency 44100 for 44.1KHz, which is CD audio rate.
/// @details Most games use 22050, because 44100 requires too much
/// CPU power on older computers.
#define MIX_HIGH_QUALITY   44100

/// Pre defined global particle sounds
typedef enum e_global_sounds
{
    GSND_GETCOIN = 0,
    GSND_DEFEND,
    GSND_WEATHER1,
    GSND_WEATHER2,
    GSND_COINFALL,
    GSND_LEVELUP,
    GSND_PITFALL,
    GSND_SHIELDBLOCK,
    GSND_COUNT
} GSND_GLOBAL;

/// what type of music data is used by mix_ptr_t
enum e_mix_type { MIX_UNKNOWN = 0, MIX_MUS, MIX_SND };
typedef enum e_mix_type mix_type_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define sound_play_chunk( pos, pchunk ) sound_play_chunk_looped( pos, pchunk, 0, (CHR_REF)MAX_CHR, &renderlist )

#define VALID_SND( ISND )       ( ISND >= 0 && ISND < MAX_WAVE )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// an anonymized "pointer" type in case we want to store data that is either a chunk or a music
struct s_mix_ptr
{
    mix_type_t type;

    union
    {
        void      * unk;
        Mix_Music * mus;
        Mix_Chunk * snd;
    } ptr;
};
typedef struct s_mix_ptr mix_ptr_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The global variables for the sound module
struct s_snd_config
{
    bool_t       soundvalid;           ///< Allow playing of sound?
    Uint8        soundvolume;          ///< Volume of sounds played

    bool_t       musicvalid;           ///< Allow music and loops?
    Uint8        musicvolume;          ///< The sound volume of music

    int          maxsoundchannel;      ///< Max number of sounds playing at the same time
    int          buffersize;           ///< Buffer size set in setup.txt
    bool_t       highquality;          ///< Allow CD quality frequency sounds?
};
typedef struct s_snd_config snd_config_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern snd_config_t snd;

extern Mix_Chunk * g_wavelist[GSND_COUNT];      ///< All sounds loaded into memory

extern bool_t      musicinmemory;                          ///< Is the music loaded in memory?
extern Sint8       songplaying;                            ///< Current song that is playing
extern Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];   ///< This is a specific music file loaded into memory

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The global functions for the sound module

bool_t sound_initialize();
void   sound_restart();

Mix_Chunk * sound_load_chunk_vfs( const char * szFileName );
Mix_Music * sound_load_music( const char * szFileName );
bool_t      sound_load( mix_ptr_t * pptr, const char * szFileName, mix_type_t type );

int     sound_play_mix( fvec3_t pos, struct s_mix_ptr * pptr );
int     sound_play_chunk_looped( fvec3_t pos, Mix_Chunk * pchunk, int loops, const CHR_REF object, struct s_renderlist * prlist );
void    sound_play_song( int songnumber, Uint16 fadetime, int loops );
void    sound_finish_song( Uint16 fadetime );
int     sound_play_chunk_full( Mix_Chunk * pchunk );

void    sound_fade_all();
void    fade_in_music( Mix_Music * music );

void    sound_stop_channel( int whichchannel );
void    sound_stop_song();

void    load_global_waves( void );
void    load_all_music_sounds_vfs();

bool_t snd_config_synch( snd_config_t * psnd, struct s_egoboo_config * pcfg );

bool_t looped_stop_object_sounds( const CHR_REF  ichr );
void   looped_update_all_sound( struct s_renderlist * prlist );

void   sound_finish_sound();
void   sound_free_chunk( Mix_Chunk * pchunk );

int get_current_song_playing();
bool_t LoopedList_remove( int channel );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _sound_h