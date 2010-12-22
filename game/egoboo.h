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

/// The following magic allows this include to work in multiple files
#if defined(DECLARE_GLOBALS)
#    define EXTERN
#    define EQ(x) = x
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.8.1"                            ///< Version of the game

#define NOSPARKLE           255                     ///< Dont sparkle icons
#define SPELLBOOK           127                     ///< The spellbook model

/// Messaging stuff
#define MAX_MESSAGE         8                       ///< Number of messages
#define MAXTOTALMESSAGE     4096
#define MESSAGESIZE         80
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)

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
// Timers
//------------------------------------
// Display
EXTERN Uint8           timeron     EQ( bfalse );          ///< Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           ///< Timer time ( 50ths of a second )

// fps stuff
EXTERN Sint32          game_fps_clock             EQ( 0 );             ///< The number of ticks this second
EXTERN float           stabilized_menu_fps        EQ( TARGET_FPS );
EXTERN float           stabilized_menu_fps_sum    EQ( 0.1f * TARGET_FPS );
EXTERN float           stabilized_menu_fps_weight EQ( 0.1f );

EXTERN Uint32          game_fps_loops             EQ( 0 );             ///< The number of frames drawn this second
EXTERN float           stabilized_game_fps        EQ( TARGET_FPS );
EXTERN float           stabilized_game_fps_sum    EQ( 0.1f * TARGET_FPS );
EXTERN float           stabilized_game_fps_weight EQ( 0.1f );

EXTERN float           stabilized_fps             EQ( TARGET_FPS );

EXTERN Sint32          menu_fps_clock        EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          menu_fps_loops        EQ( 0 );             ///< The number of frames drawn this second

EXTERN float           est_max_fps           EQ( TARGET_FPS );
EXTERN float           est_render_time       EQ( 1.0f / TARGET_FPS );

EXTERN float           est_update_time       EQ( 1.0f / TARGET_UPS );
EXTERN float           est_max_ups           EQ( TARGET_UPS );

EXTERN float           est_gfx_time          EQ( 1.0f / TARGET_FPS );
EXTERN float           est_max_gfx           EQ( TARGET_FPS );

EXTERN float           est_single_update_time  EQ( 1.0f / TARGET_UPS );
EXTERN float           est_single_ups          EQ( TARGET_UPS );

EXTERN float           est_update_game_time  EQ( 1.0f / TARGET_UPS );
EXTERN float           est_max_game_ups      EQ( TARGET_UPS );

EXTERN Sint32          ups_clock             EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             ///< The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGET_UPS );
EXTERN float           stabilized_ups_sum    EQ( 0.1f * TARGET_UPS );
EXTERN float           stabilized_ups_weight EQ( 0.1f );

/// Timers
EXTERN Sint32          ticks_last  EQ( 0 );
EXTERN Sint32          ticks_now   EQ( 0 );
EXTERN Sint32          clock_stt   EQ( 0 );             ///< GetTickCount at start
EXTERN Sint32          clock_all   EQ( 0 );             ///< The total number of ticks so far
EXTERN Sint32          clock_wld   EQ( 0 );             ///< The sync clock
EXTERN Uint32          update_wld  EQ( 0 );             ///< The number of times the game has been updated
EXTERN Uint32          game_frame_all   EQ( 0 );             ///< The total number of frames drawn so far
EXTERN Uint32          clock_enc_stat  EQ( 0 );         ///< For character stat regeneration
EXTERN Uint32          clock_chr_stat  EQ( 0 );         ///< For enchant stat regeneration
EXTERN Uint32          clock_pit   EQ( 0 );             ///< For pit kills
EXTERN Uint32          outofsync   EQ( 0 );
EXTERN Uint32          true_update EQ( 0 );
EXTERN Uint32          true_frame  EQ( 0 );
EXTERN int             update_lag  EQ( 0 );
EXTERN bool_t          soundon  EQ( btrue );              ///< Is the sound alive?

EXTERN bool_t          pickedmodule_ready EQ( bfalse ); ///< Is there a new picked module?
EXTERN int             pickedmodule_index EQ( -1 );     ///< The module index number
EXTERN STRING          pickedmodule_path;               ///< The picked module's full path name
EXTERN STRING          pickedmodule_name;               ///< The picked module's short name
EXTERN STRING          pickedmodule_write_path;         ///< The picked module's path name relative to the userdata directory

/// Setup values
EXTERN Uint8                   messageon      EQ( btrue );         ///< Messages?
EXTERN int                     maxmessage     EQ( MAX_MESSAGE );
EXTERN int                     wraptolerance  EQ( 80 );            ///< Status bar
EXTERN bool_t                  wateron        EQ( btrue );         ///< Water overlays?
EXTERN bool_t                  fpson          EQ( btrue );         ///< Show FPS?

/// EWWWW. GLOBALS ARE EVIL.

/// KEYBOARD
EXTERN bool_t console_mode EQ( bfalse );                   ///< Input text from keyboard?
EXTERN bool_t console_done EQ( bfalse );                   ///< Input text from keyboard finished?

#define INVISIBLE           20                      ///< The character can't be detected

struct s_local_stats 
{
    bool_t  noplayers;          ///< Are there any local players?

    float   grog_level;
    float   daze_level;
    float   seeinvis_level;
    float   seeinvis_mag;
    float   seedark_level;
    float   seedark_mag;
    float   seekurse_level;
    float   listening_level;    ///< Players with listen skill?

    bool_t  allpladead;         ///< Have players died?
    int     revivetimer;        ///< Cooldown to respawn

    //ESP
    TEAM_REF sense_enemies_team;
    IDSZ     sense_enemies_idsz;
};
typedef struct s_local_stats local_stats_t;

EXTERN local_stats_t local_stats;

//---------------------------------------------------------------------------------------------------------------------

#include "egoboo_process.h"

/// a process that controls the master loop of the program
struct s_ego_process
{
    process_t base;

    double frameDuration;
    int    menuResult;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    ticks_next, ticks_now;

    char * argv0;
};
typedef struct s_ego_process ego_process_t;

extern ego_process_t * EProc;

void ego_init_SDL_base( void );

EXTERN bool_t screenshot_requested EQ( bfalse );

EXTERN bool_t single_frame_mode EQ( bfalse );
EXTERN bool_t single_frame_keyready EQ( btrue );
EXTERN bool_t single_frame_requested EQ( bfalse );
EXTERN bool_t single_update_requested EQ( bfalse );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void egoboo_clear_vfs_paths( void );
void egoboo_setup_vfs_paths( void );

Uint32 egoboo_get_ticks( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define  _EGOBOO_H_
