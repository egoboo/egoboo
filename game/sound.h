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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - sound.h
 * Sound handling using SDL_mixer
 */

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#include <SDL_mixer.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_egoboo_config;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAX_WAVE           30             // Up to 30 wave/ogg per model
#define VOLUMERATIO        7             // Volume ratio
#define MAXPLAYLISTLENGTH 35             // Max number of different tracks loaded into memory

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// BB > enumerated "speech" sounds, so that we COULD ge the scripts to classify which
// sound to use for the "ouch", the "too much baggage", etc.
// also some left-over sounds from the RTS days, but they might be useful if an NPC
// uses messages to control his minions.

// for example:
// necromancer sends message to all minions "attack blah"
// zombie minion responds with "moooooaaaaannn" automatically because that is the sound
// registered as his SPEECH_ATTACK sound.
// It seems to have a lot of potential to me. It *could* be done completely in the scripts,
// but the idea of having registered sounds for certain actions makes a lot of sense to me! :)

enum e_sound_types
{
    SOUND_FOOTFALL = 0,
    SOUND_JUMP,
    SOUND_SPAWN,
    SOUND_DEATH,

    // old "RTS" stuff
    SPEECH_MOVE,
    SPEECH_MOVEALT,
    SPEECH_ATTACK,
    SPEECH_ASSIST,
    SPEECH_TERRAIN,
    SPEECH_SELECT,

    SOUND_COUNT,

    SPEECH_BEGIN = SPEECH_MOVE,
    SPEECH_END   = SPEECH_SELECT
};

typedef enum e_global_sounds
{
    GSND_GETCOIN = 0,
    GSND_DEFEND,
    GSND_WEATHER1,
    GSND_WEATHER2,
    GSND_COINFALL,
    GSND_LEVELUP,
    GSND_PITFALL,
    GSND_COUNT
} GSND_GLOBAL;

#define INVALID_SOUND -1

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// an anonymized "pointer" type in case we want to store data that is either a
// chunk or a music

enum e_mix_type { MIX_UNKNOWN = 0, MIX_MUS, MIX_SND };
typedef enum e_mix_type mix_type_t;

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
// The global variables for the sound module

struct s_snd_config
{
    bool_t       soundvalid;           // Allow playing of sound?
    Uint8        soundvolume;          // Volume of sounds played

    bool_t       musicvalid;                             // Allow music and loops?
    Uint8        musicvolume;                            // The sound volume of music

    Uint16       maxsoundchannel;      // Max number of sounds playing at the same time
    Uint16       buffersize;           // Buffer size set in setup.txt
};
typedef struct s_snd_config snd_config_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern snd_config_t snd;

extern Mix_Chunk * g_wavelist[GSND_COUNT];      // All sounds loaded into memory

extern bool_t      musicinmemory;                          // Is the music loaded in memory?
extern Sint8       songplaying;                            // Current song that is playing
extern Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];   // This is a specific music file loaded into memory

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The global functions for the sound module

bool_t sound_initialize();
void   sound_restart();

Mix_Chunk * sound_load_chunk( const char * szFileName );
Mix_Music * sound_load_music( const char * szFileName );
bool_t sound_load( mix_ptr_t * pptr, const char * szFileName, mix_type_t type );

int     sound_play_mix( GLvector3 pos, struct s_mix_ptr * pptr );
int     sound_play_chunk_looped( GLvector3 pos, Mix_Chunk * pchunk, Sint8 loops );
#define sound_play_chunk( pos, pchunk ) sound_play_chunk_looped( pos, pchunk, 0 )
void    sound_play_song( Sint8 songnumber, Uint16 fadetime, Sint8 loops );
void    sound_finish_song( Uint16 fadetime );

void    sound_fade_all();
void    fade_in_music( Mix_Music * music );

void    sound_stop_channel( int whichchannel );
void    sound_stop_song();

void   load_global_waves( const char *modname );
void   load_all_music_sounds();

bool_t snd_config_synch( snd_config_t * psnd, struct s_egoboo_config * pcfg );

