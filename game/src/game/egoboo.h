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

#define NOSPARKLE           255                     ///< Dont sparkle icons
#define SPELLBOOK           127                     ///< The spellbook model

/// Messaging stuff
#define DAMAGERAISE    25    ///< Tolerance for damage tiles

#define ONESECOND      50    ///< How many game loop updates represent 1 second (50 UPS = 1 second)

#define WRAP_TOLERANCE 90    ///< Status bar

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Timers

#define STABILIZED_KEEP  0.65f
#define STABILIZED_COVER (1.0f - STABILIZED_KEEP)

EXTERN Sint32 gfx_clear_loops EQ(0);            ///< The number of times the screen has been cleared

/// Timers
EXTERN Uint32 outofsync EQ(0);

//HUD
EXTERN bool timeron EQ(false);  ///< Game timer displayed?
EXTERN Uint32 timervalue EQ(0); ///< Timer time ( 50ths of a second )

/// EWWWW. GLOBALS ARE EVIL.

#define INVISIBLE           20                      ///< The character can't be detected

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct local_stats_t
{
    bool noplayers;          ///< Are there any local players?
    int player_count;

    float grog_level;
    float daze_level;
    float seeinvis_level;
    float seeinvis_mag;
    float seedark_level;
    float seedark_mag;
    float seekurse_level;

    bool allpladead;         ///< Have players died?
    int revivetimer;        ///< Cooldown to respawn

    //ESP
    TEAM_REF sense_enemies_team;
    IDSZ sense_enemies_idsz;
};

EXTERN local_stats_t local_stats;

/**
 * @brief
 * @param sync_from_file
 *  see remarks
 * @remark
 *  If @a fromfile is @a true, the values from <tt>"setup.txt"</tt> are downloaded
 *  into the egoboo_config_t data structure, otherwise not. Next, the data from the program
 *  variables are downloaded into the egoboo_config_t data structure and the program variables
 *  are uploaded into the egoboo_config_data_t data structure. Finally, if @a tofile
 *  is @a true, the values from the egoboo_config_data_t are uploaded into <tt>"setup.txt"</tt>.
 */
bool config_synch(egoboo_config_t *pcfg, bool fromfile, bool tofile);

//---------------------------------------------------------------------------------------------------------------------

#include "egolib/egolib.h"
