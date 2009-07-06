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

/* Egoboo - egoboo.h
 * Disgusting, hairy, way too monolithic header file for the whole darn
 * project.  In severe need of cleaning up.  Venture here with extreme
 * caution, and bring one of those canaries with you to make sure you
 * don't run out of oxygen.
 */

/* Typedefs for various platforms */
#include "egoboo_typedef.h"
#include "egoboo_math.h"  /* vector and matrix math */
#include "egoboo_config.h"

#include "proto.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_opengl.h>

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) = x;
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.7.4"                         // Version of the game

// My lil' random number table
#define MAXRAND 4096
#define RANDIE randie[randindex];  randindex = (randindex+1)&(MAXRAND-1)

EXTERN Uint16  randindex EQ( 0 );
EXTERN Uint16  randie[MAXRAND];

enum e_game_difficulty
{
    GAME_EASY  = 0,
    GAME_NORMAL,
    GAME_HARD
};


#define EXPKEEP 0.85f                                // Experience to keep when respawning

#define MAXINVENTORY        7
#define MAXIMPORTPERPLAYER  (MAXINVENTORY + 2)
#define MAXIMPORT           (4*MAXIMPORTPERPLAYER)          // Number of subdirs in IMPORT directory

#define NOSPARKLE           255
#define ANYTIME             0xFF          // Code for respawnvalid...

#define SIZETIME            50                      // Time it takes to resize a character

#define NOSKINOVERRIDE      -1                      // For import

#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define EDGE                128                     // Camera bounds/edge of the map

#define NOHIDE              127                     // Don't hide

// Stats
#define MANARETURNSHIFT     22
#define LOWSTAT             256                     // Worst...
#define PERFECTSTAT         (60*256)                // Maximum stat without magic effects
#define PERFECTBIG          (100*256)               // Perfect life or mana...
#define HIGHSTAT            (100*256)                // Absolute max adding enchantments as well

enum e_damage_fx
{
    DAMFX_NONE           = 0,                       // Damage effects
    DAMFX_ARMO           = (1 << 1),                // Armor piercing
    DAMFX_NBLOC          = (1 << 2),                // Cannot be blocked by shield
    DAMFX_ARRO           = (1 << 3),                // Only hurts the one it's attached to
    DAMFX_TURN           = (1 << 4),                // Turn to attached direction
    DAMFX_TIME           = (1 << 5)
};

#define HURTDAMAGE           256                     // Minimum damage for hurt animation

#define ULTRABLUDY           2                       // This makes any damage draw blud

#define SPELLBOOK           127                     // The spellbook model

// Geneder stuff
#define GENFEMALE           0                       // Gender
#define GENMALE             1
#define GENOTHER            2
#define GENRANDOM           3

// Messaging stuff
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     4096
#define MESSAGESIZE         80
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define TABADD              (1<<5)
#define TABAND              (~(TABADD-1))                      // Tab size

#define GRABSIZE            90.0f                      // Grab tolerance
#define SEEINVISIBLE        128                        // Cutoff for invisible characters


enum e_damage_type
{
    DAMAGE_SLASH = 0,
    DAMAGE_CRUSH,
    DAMAGE_POKE,
    DAMAGE_HOLY,                             // (Most invert Holy damage )
    DAMAGE_EVIL,
    DAMAGE_FIRE,
    DAMAGE_ICE,
    DAMAGE_ZAP,
    DAMAGE_COUNT                             // Damage types
};
#define DAMAGE_NONE          255

#define DAMAGEMANA          16                      // 000x0000 Deals damage to mana
#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )

#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time
#define DROPXYVEL           8
#define DROPZVEL            7
#define JUMPATTACKVEL       -2
#define WATERJUMP           12


enum e_idsz_type
{
    IDSZ_PARENT = 0,                             // Parent index
    IDSZ_TYPE,                                   // Self index
    IDSZ_SKILL,                                  // Skill index
    IDSZ_SPECIAL,                                // Special index
    IDSZ_HATE,                                   // Hate index
    IDSZ_VULNERABILITY,                          // Vulnerability index
    IDSZ_COUNT                                   // ID strings per character
};

// XP stuff
enum e_xp_type
{
    XP_FINDSECRET = 0,                          // Finding a secret
    XP_WINQUEST,                                // Beating a module or a subquest
    XP_USEDUNKOWN,                              // Used an unknown item
    XP_KILLENEMY,                               // Killed an enemy
    XP_KILLSLEEPY,                              // Killed a sleeping enemy
    XP_KILLHATED,                               // Killed a hated enemy
    XP_TEAMKILL,                                // Team has killed an enemy
    XP_TALKGOOD,                                // Talk good, er...  I mean well
    XP_COUNT                                    // Number of ways to get experience
};
#define XPDIRECT            255                     // No modification


// Z velocity stuff
#define JUMPDELAY           20                      // Time between jumps


#define RAISE       12 //25                               // Helps correct z level
#define SHADOWRAISE 5
#define DAMAGERAISE 25


/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000
#define UPDATES_PER_SEC                 50
#define UPDATE_SKIP                     ((float)TICKS_PER_SEC/(float)UPDATES_PER_SEC)
#define ONESECOND                       TICKS_PER_SEC


#define NORTH 16384                                 // Character facings
#define SOUTH 49152
#define EAST 32768
#define WEST 0
#define RANDOM rand() % 65535
#define FRONT 0                                     // Attack directions
#define BEHIND 32768
#define LEFT 49152
#define RIGHT 16384

#define MAXXP 200000                                // Maximum experience
#define MAXMONEY 9999                               // Maximum money

//------------------------------------
// Character defines
//------------------------------------
#define MAXSKIN   4

// Display
EXTERN Uint8                   timeron  EQ( bfalse );          // Game timer displayed?
EXTERN Uint32                  timervalue  EQ( 0 );           // Timer time ( 50ths of a second )
EXTERN bool_t                  fpson EQ(btrue);
EXTERN char                    szfpstext[]  EQ( "000 FPS" );

// Timers
EXTERN Sint32          clock_stt;                   // GetTickCount at start
EXTERN Sint32          clock_all  EQ( 0 );             // The total number of ticks so far
EXTERN Sint32          clock_lst  EQ( 0 );             // The last total of ticks so far
EXTERN Sint32          clock_wld  EQ( 0 );             // The sync clock
EXTERN Sint32          clock_fps  EQ( 0 );             // The number of ticks this second
EXTERN Uint32          update_wld  EQ( 0 );            // The number of times the game has been updated
EXTERN Uint32          frame_all  EQ( 0 );             // The total number of frames drawn so far
EXTERN Uint32          frame_fps  EQ( 0 );             // The number of frames drawn this second
EXTERN Uint32          clock_stat  EQ( 0 );            // For stat regeneration
EXTERN Uint32          clock_pit  EQ( 0 );             // For pit kills
EXTERN Uint32          outofsync  EQ( 0 );
EXTERN Uint8           parseerror  EQ( bfalse );


EXTERN bool_t          soundon  EQ( btrue );              // Is the sound alive?

EXTERN bool_t          pickedmodule_ready;              // Is there a new picked module?
EXTERN char            pickedmodule_name[64];           // The module load name
EXTERN int             pickedmodule_index;                // The module index number

//Respawning
EXTERN bool_t                   local_allpladead;            // Has everyone died?
EXTERN Uint16                   revivetimer EQ(0);

// Imports
EXTERN int                     numimport;                     // Number of imports from this machine
EXTERN Uint32                  local_control[16];             // Input bits for each imported player
EXTERN short                   local_slot[16];                // For local imports

// Setup values
EXTERN Uint8                   messageon      EQ( btrue );         // Messages?
EXTERN int                     maxmessage     EQ( MAXMESSAGE );
EXTERN int                     wraptolerance  EQ( 80 );            // Status bar
EXTERN bool_t                  wateron        EQ( btrue );         // Water overlays?

// EWWWW. GLOBALS ARE EVIL.

// KEYBOARD
EXTERN bool_t console_mode EQ( bfalse );                   // Input text from keyboard?
EXTERN bool_t console_done EQ( bfalse );                   // Input text from keyboard finished?


extern float           light_a, light_d, light_x, light_y, light_z;
EXTERN float           hillslide  EQ( 1.00f );
EXTERN float           slippyfriction  EQ( 1.00f );                            // Friction
EXTERN float           airfriction  EQ( 0.91f );                               // 0.9868 is approximately real world air friction
EXTERN float           waterfriction  EQ( 0.80f );
EXTERN float           noslipfriction  EQ( 0.91f );
EXTERN float           gravity  EQ( -1.0f );                                   // Gravitational accel

#define INVISIBLE           20                      // The character can't be detected

EXTERN bool_t                    local_seeinvisible   EQ( bfalse );
EXTERN bool_t                    local_seekurse       EQ( bfalse );
EXTERN Uint16                    local_senseenemies   EQ( MAX_CHR );
EXTERN IDSZ                      local_senseenemiesID EQ( bfalse );
EXTERN bool_t                    local_listening      EQ( bfalse );  // Players with listen skill?
EXTERN bool_t                    local_noplayers;					 // Are there any local players?

//------------------------------------
// Model stuff
//------------------------------------
EXTERN int             globalicon_count;                              // Number of icons



EXTERN Uint16  skintoicon[MAX_TEXTURE];                  // Skin to icon

EXTERN Uint16  bookicon_count    EQ(0);
EXTERN Uint16  bookicon[MAXSKIN];                      // The first book icon

EXTERN const char *globalparsename  EQ( NULL ); // The SCRIPT.TXT filename

// phisics info

EXTERN float           platstick  EQ( 0.1f );


#define CLOSETOLERANCE      2                       // For closing doors

enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

#define  _EGOBOO_H_
