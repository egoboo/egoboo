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

/// @file egoboo.h
///
/// @details Disgusting, hairy, way too monolithic header file for the whole darn
/// project.  In severe need of cleaning up.  Venture here with extreme
/// caution, and bring one of those canaries with you to make sure you
/// don't run out of oxygen.

/* Typedefs for various platforms */
#include "egoboo_typedef.h"
#include "egoboo_math.h"  /* vector and matrix math */
#include "egoboo_config.h"

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

/// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) = x
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.7.8"                         ///< Version of the game

#define MAXINVENTORY        6
#define MAXIMPORTOBJECTS    (MAXINVENTORY + 2)      ///< left hand + right hand + MAXINVENTORY
#define MAXIMPORTPERPLAYER  (1 + MAXIMPORTOBJECTS)  ///< player + MAXIMPORTOBJECTS

#define NOSPARKLE           255
#define RESPAWN_ANYTIME     0xFF          ///< Code for respawnvalid...

#define NOHIDE              127                     ///< Don't hide

#define SPELLBOOK           127                     ///< The spellbook model

/// Messaging stuff
#define MAX_MESSAGE          8                       ///< Number of messages
#define MAXTOTALMESSAGE     4096
#define MESSAGESIZE         80
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)

#define GRABSIZE            90.0f                      ///< Grab tolerance
#define SEEINVISIBLE        128                        ///< Cutoff for invisible characters

#define RAISE               12                  ///< Helps correct z level
#define SHADOWRAISE         5
#define DAMAGERAISE         25                  ///< Tolerance for damage tiles

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f

#define TARGET_FPS                      30.0f
#define FRAME_SKIP                      (TICKS_PER_SEC/TARGET_FPS)    ///< 1000 tics per sec / 50 fps = 20 ticks per frame

#define EMULATE_UPS                     50.0f
#define TARGET_UPS                      50.0f
#define UPDATE_SCALE                    (EMULATE_UPS/(stabilized_ups_sum/stabilized_ups_weight))
#define UPDATE_SKIP                     (TICKS_PER_SEC/TARGET_UPS)    ///< 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATE_SKIP)    ///< 1000 tics per sec / 20 ticks per frame = 50 fps

//------------------------------------
/// Character defines
//------------------------------------

//Dismounting
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  (TICKS_PER_SEC*0.5f)          ///< time delay for full object-object interaction (approximately 0.5 second)

//------------------------------------
/// Timers
//------------------------------------

/// Display
EXTERN Uint8           timeron     EQ( bfalse );          ///< Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           ///< Timer time ( 50ths of a second )

EXTERN bool_t          fpson        EQ(btrue);

/// fps stuff
EXTERN float           est_max_fps           EQ( TARGET_FPS );
EXTERN float           est_gfx_time          EQ( 1.0f );
EXTERN Sint32          fps_clock             EQ(0);               ///< The number of ticks this second
EXTERN Uint32          fps_loops             EQ(0);               ///< The number of frames drawn this second
EXTERN float           stabilized_fps        EQ( TARGET_FPS );
EXTERN float           stabilized_fps_sum    EQ( 0 );
EXTERN float           stabilized_fps_weight EQ( 0 );

EXTERN float           est_update_time       EQ( 1.0f / TARGET_UPS );
EXTERN float           est_max_ups           EQ( TARGET_UPS );
EXTERN Sint32          ups_clock             EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             ///< The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGET_UPS );
EXTERN float           stabilized_ups_sum    EQ( 0 );
EXTERN float           stabilized_ups_weight EQ( 0 );

/// Timers
EXTERN Sint32          ticks_last  EQ( 0 );
EXTERN Sint32          ticks_now   EQ( 0 );
EXTERN Sint32          clock_stt   EQ( 0 );             ///< GetTickCount at start
EXTERN Sint32          clock_all   EQ( 0 );             ///< The total number of ticks so far
EXTERN Sint32          clock_lst   EQ( 0 );             ///< The last total of ticks so far
EXTERN Sint32          clock_wld   EQ( 0 );             ///< The sync clock
EXTERN Sint32          clock_fps   EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          update_wld  EQ( 0 );             ///< The number of times the game has been updated
EXTERN Uint32          frame_all   EQ( 0 );             ///< The total number of frames drawn so far
EXTERN Uint32          frame_fps   EQ( 0 );             ///< The number of frames drawn this second
EXTERN Uint32          clock_enc_stat  EQ( 0 );         ///< For character stat regeneration
EXTERN Uint32          clock_chr_stat  EQ( 0 );         ///< For enchant stat regeneration
EXTERN Uint32          clock_pit   EQ( 0 );             ///< For pit kills
EXTERN Uint32          outofsync   EQ( 0 );
EXTERN Uint32          true_update EQ( 0 );
EXTERN int             update_lag  EQ( 0 );
EXTERN bool_t          soundon  EQ( btrue );              ///< Is the sound alive?

EXTERN bool_t          pickedmodule_ready;              ///< Is there a new picked module?
EXTERN char            pickedmodule_name[64];           ///< The module load name
EXTERN int             pickedmodule_index;                ///< The module index number

/// Respawning
EXTERN bool_t                   local_allpladead;            ///< Has everyone died?
EXTERN Uint16                   revivetimer EQ(0);

/// Imports
EXTERN int                     local_import_count;                     ///< Number of imports from this machine
EXTERN Uint32                  local_import_control[16];             ///< Input bits for each imported player
EXTERN int                     local_import_slot[16];                ///< For local imports

/// Setup values
EXTERN Uint8                   messageon      EQ( btrue );         ///< Messages?
EXTERN int                     maxmessage     EQ( MAX_MESSAGE );
EXTERN int                     wraptolerance  EQ( 80 );            ///< Status bar
EXTERN bool_t                  wateron        EQ( btrue );         ///< Water overlays?

/// EWWWW. GLOBALS ARE EVIL.

/// KEYBOARD
EXTERN bool_t console_mode EQ( bfalse );                   ///< Input text from keyboard?
EXTERN bool_t console_done EQ( bfalse );                   ///< Input text from keyboard finished?

extern float           light_a, light_d, light_x, light_y, light_z;
EXTERN float           hillslide  EQ( 1.00f );
EXTERN float           slippyfriction  EQ( 1.00f );                            ///< Friction
EXTERN float           airfriction  EQ( 0.91f );                               ///< 0.9868 is approximately real world air friction
EXTERN float           waterfriction  EQ( 0.80f );
EXTERN float           noslipfriction  EQ( 0.91f );
EXTERN float           gravity  EQ( -1.0f );                                   ///< Gravitational accel

#define INVISIBLE           20                      ///< The character can't be detected

EXTERN bool_t                    local_seeinvisible      EQ( bfalse );
EXTERN int                       local_seedark           EQ( 0 );
EXTERN bool_t                    local_seekurse          EQ( bfalse );
EXTERN bool_t                    local_listening         EQ( bfalse );  ///< Players with listen skill?
EXTERN bool_t                    local_noplayers;                    ///< Are there any local players?

//------------------------------------
/// Model stuff
//------------------------------------

EXTERN const char *globalparsename  EQ( NULL ); ///< The SCRIPT.TXT filename

EXTERN float           platstick  EQ( 0.1f );   ///< phisics info

/// The possible pre-defined orders
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

