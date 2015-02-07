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

/// @file game/egoboo.h
///
/// @details Disgusting, hairy, way too monolithic header file for the whole darn
///          project.  In severe need of cleaning up.  Venture here with extreme
///          caution, and bring one of those canaries with you to make sure you
///          don't run out of oxygen.

#pragma once

/* Typedefs for various platforms */
#include "game/egoboo_typedef.h"

/// The following magic allows this include to work in multiple files
#if defined(DECLARE_GLOBALS)
#    define EXTERN
#    define EQ(x) = x
#else
#    define EXTERN extern
#    define EQ(x)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define VERSION "2.9.0"                             ///< Version of the game

#define NOSPARKLE           255                     ///< Dont sparkle icons
#define SPELLBOOK           127                     ///< The spellbook model

/// Messaging stuff
#define DAMAGERAISE         25                      ///< Tolerance for damage tiles

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f

#define TARGET_UPS                      50.0f
#define UPDATE_SKIP                     (TICKS_PER_SEC/TARGET_UPS)    ///< 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATE_SKIP)    ///< 1000 tics per sec / 20 ticks per frame = 50 fps

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Timers

#define STABILIZED_KEEP  0.65f
#define STABILIZED_COVER (1.0f - STABILIZED_KEEP)

EXTERN Sint32          game_fps_clock             EQ( 0 );             ///< The number of ticks this second

EXTERN Uint32          game_fps_loops             EQ( 0 );             ///< The number of frames drawn this second

EXTERN Sint32          gfx_clear_loops       EQ( 0 );             ///< The number of times the screen has been cleared

EXTERN Sint32          menu_fps_clock        EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          menu_fps_loops        EQ( 0 );             ///< The number of frames drawn this second

EXTERN Sint32          game_ups_clock             EQ( 0 );             ///< The number of ticks this second
EXTERN Uint32          game_ups_loops             EQ( 0 );             ///< The number of frames drawn this second
EXTERN float           stabilized_game_ups        EQ( TARGET_UPS );
EXTERN float           stabilized_game_ups_sum    EQ( STABILIZED_COVER * TARGET_UPS );
EXTERN float           stabilized_game_ups_weight EQ( STABILIZED_COVER );

/// Timers
EXTERN Uint32          outofsync   EQ( 0 );

EXTERN bool          pickedmodule_ready EQ( false ); ///< Is there a new picked module?
EXTERN int             pickedmodule_index EQ( -1 );     ///< The module index number
EXTERN STRING          pickedmodule_path;               ///< The picked module's full path name
EXTERN STRING          pickedmodule_name;               ///< The picked module's short name
EXTERN STRING          pickedmodule_write_path;         ///< The picked module's path name relative to the userdata directory

//HUD
EXTERN bool          timeron        EQ( false );        ///< Game timer displayed?
EXTERN Uint32          timervalue     EQ( 0 );             ///< Timer time ( 50ths of a second )
EXTERN int             wraptolerance  EQ( 80 );            ///< Status bar
EXTERN bool          fpson          EQ( true );         ///< Show FPS?

/// EWWWW. GLOBALS ARE EVIL.

#define INVISIBLE           20                      ///< The character can't be detected

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct local_stats_t
{
    bool  noplayers;          ///< Are there any local players?
    int     player_count;

    float   grog_level;
    float   daze_level;
    float   seeinvis_level;
    float   seeinvis_mag;
    float   seedark_level;
    float   seedark_mag;
    float   seekurse_level;
    float   listening_level;    ///< Players with listen skill?

    bool  allpladead;         ///< Have players died?
    int     revivetimer;        ///< Cooldown to respawn

    //ESP
    TEAM_REF sense_enemies_team;
    IDSZ     sense_enemies_idsz;
};

EXTERN local_stats_t local_stats;

//---------------------------------------------------------------------------------------------------------------------

#include "egolib/egolib.h"

/// A process that controls the master loop of the program.
struct ego_process_t
{
    process_t base;

    double frameDuration;
    int    menuResult;

    bool was_active;
    bool escape_requested, escape_latch;

    egolib_timer_t loop_timer;

    bool free_running_latch_requested;
    bool free_running_latch;

	/// @brief The number of command-line arguments.
	int argc;
	/// @brief The command-line arguments.
	char **argv;
};

void ego_init_SDL_base();

EXTERN bool single_frame_mode EQ( false );
EXTERN bool single_frame_keyready EQ( true );
EXTERN bool single_frame_requested EQ( false );
EXTERN bool single_update_requested EQ( false );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern ego_process_t * EProc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint32 egoboo_get_ticks();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define  _egoboo_h_
