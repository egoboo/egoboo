#pragma once

#include "egoboo_types.h"

#include <SDL_mixer.h>

#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define MAXWAVE         16                            // Up to 16 waves per model
#define VOLMIN          -4000                         // Minumum Volume level
#define VOLUMERATIO     7                             // Volume ratio

//Sound using SDL_Mixer
extern bool_t      mixeron;                          //Is the SDL_Mixer loaded?
extern Mix_Chunk  *globalwave[MAXWAVE];               //All sounds loaded into memory
//extern Mix_Chunk  *wave;                             //Used for playing one selected sound file
//extern int         channel;                           //Which channel the current sound is using

#define INVALID_SOUND   (-1)
#define INVALID_CHANNEL (-1)

#define FIX_SOUND(XX) ((((XX)<0) || ((XX)>=MAXWAVE)) ? INVALID_SOUND : (XX))

typedef enum global_sound_t
{
  GSOUND_COINGET = 0,              // 0 - Pick up coin
  GSOUND_DEFEND,                   // 1 - Defend clank
  GSOUND_WEATHER,                  // 2 - Weather Effect
  GSOUND_SPLASH,                   // 3 - Hit Water tile (Splash)
  GSOUND_COINFALL,                 // 4 - Coin falls on ground
  GSOUND_LEVELUP,				           // 5 - Level up sound
  GSOUND_COUNT = MAXWAVE
};

//Music using SDL_Mixer
#define MAXPLAYLISTLENGTH 25      //Max number of different tracks loaded into memory

extern bool_t     musicinmemory;                        //Is the music loaded in memory?
extern Mix_Music *instrumenttosound[MAXPLAYLISTLENGTH]; //This is a specific music file loaded into memory
extern int        songplaying;                          //Current song that is playing
