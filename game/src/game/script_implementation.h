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

/// @file game/script_implementation.h
/// @details turns script_functions.* into a bunch of stubs

#include "game/egoboo_typedef.h"

#include "egolib/_math.h"

/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_chr;
struct s_script_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_waypoint_list;
typedef struct s_waypoint_list waypoint_list_t;

struct s_line_of_sight_info;
typedef struct s_line_of_sight_info line_of_sight_info_t;

//--------------------------------------------------------------------------------------------
// waypoint_list_t
//--------------------------------------------------------------------------------------------

#define MAXWAY              8                       ///< Waypoints
#define WAYTHRESH           (GRID_ISIZE >> 1)       ///< Threshold for reaching waypoint (GRID_FSIZE/2)

typedef float waypoint_t[3];

struct s_waypoint_list
{
    int          tail;         ///< Which waypoint
    int          head;         ///< Where to stick next
    waypoint_t   pos[MAXWAY];  ///< Waypoint
};

ego_bool waypoint_list_peek( waypoint_list_t * plst, waypoint_t wp );
ego_bool waypoint_list_push( waypoint_list_t * plst, int x, int y );
ego_bool waypoint_list_reset( waypoint_list_t * plst );
ego_bool waypoint_list_clear( waypoint_list_t * plst );
ego_bool waypoint_list_empty( waypoint_list_t * plst );
ego_bool waypoint_list_finished( waypoint_list_t * plst );
ego_bool waypoint_list_advance( waypoint_list_t * plst );

//--------------------------------------------------------------------------------------------
/// Data needed to specify a line-of-sight test
struct s_line_of_sight_info
{
    float x0, y0, z0;
    float x1, y1, z1;
    Uint32 stopped_by;

    CHR_REF collide_chr;
    Uint32  collide_fx;
    int     collide_x;
    int     collide_y;
};

ego_bool line_of_sight_blocked( line_of_sight_info_t * plos );
ego_bool line_of_sight_with_mesh( line_of_sight_info_t * plos );
ego_bool line_of_sight_with_characters( line_of_sight_info_t * plos );

//--------------------------------------------------------------------------------------------
// wrap generic bitwise conversion macros
//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
/// @details formal conversion of a boolean value to an integer (ego_false = 0, ego_true = 1)
BIT_FIELD bool_to_bit( ego_bool val );

/// @ingroup _bitwise_functions_
/// @details formal conversion of an integer to a boolean value ( 0 = ego_false, all other values are ego_true )
ego_bool bit_to_bool( BIT_FIELD val );

//--------------------------------------------------------------------------------------------
// wrap the BIT_FIELD macros, since lua doesn't recognize bitwise functions
//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_08BITS() macro
Uint8 BIT_FIELD_clip_to_08_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_16BITS() macro
Uint16 BIT_FIELD_clip_to_16_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_24BITS() macro
Uint32 BIT_FIELD_clip_to_24_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_32BITS() macro
Uint32 BIT_FIELD_clip_to_32_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for FILL_BIT_FIELD() macro
BIT_FIELD BIT_FIELD_fill( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for EMPTY_BIT_FIELD() macro
BIT_FIELD BIT_FIELD_empty( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for SET_BIT() macro
BIT_FIELD BIT_FIELD_set_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details wrapper for UNSET_BIT() macro
BIT_FIELD BIT_FIELD_unset_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_SOME_BITS() macro
ego_bool    BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_ALL_BITS() macro
ego_bool    BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_NO_BITS() macro
ego_bool    BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for MISSING_BITS() macro
ego_bool    BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details set the given bit
BIT_FIELD BIT_FIELD_set_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details clear the given bit
BIT_FIELD BIT_FIELD_clear_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details test the given bit
ego_bool    BIT_FIELD_test_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details adds all bits values to val
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details removes all bits values from val
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details the values has all the given bits
ego_bool    BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits );

//--------------------------------------------------------------------------------------------
// wrapped script functions
//--------------------------------------------------------------------------------------------

/// @author ZZ
/// @details This function tells the character where to move next
///
/// @lua AddWaypoint( tmpx = "x position", tmpy = "y position" )
ego_bool AddWaypoint( waypoint_list_t * plst, CHR_REF ichr, float pos_x, float pos_y );

/// @author ZF
/// @details Ported the A* path finding algorithm by birdsey and heavily modified it
/// This function adds enough waypoints to get from one point to another
ego_bool FindPath( waypoint_list_t * plst, struct s_chr * pchr, float dst_x, float dst_y, ego_bool * used_astar_ptr );

/// @author ZZ
/// @details This function modifies tmpx and tmpy, depending on the setting of
/// tmpdistance and tmpturn.  It acts like one of those Compass thing
/// with the two little needle legs
///
/// @lua Compass( tmpturn = "rotation", tmpdistance = "radius" )
ego_bool Compass( fvec2_base_t pos, int facing, float distance );

/// @author ZZ
/// @details This function returns the cost of the desired skin upgrade
//
/// @lua tmpx = GetTargetArmorPrice( tmpargument = "skin" )
int GetArmorPrice( struct s_chr * pchr, const int skin );

/// @author ZZ
/// @details This function sets the character's ai timer.  50 clicks per second.
/// Used in conjunction with IfTimeOut
///
/// @lua selftimer = UpdateTime( selftimer = "time", tmpargument = "delay" )
Uint32 UpdateTime( Uint32 time_val, int delay );

/// @author ZZ
/// @details This function breaks the tiles of a passage if there is a character standing
///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.
Uint8 BreakPassage( int mesh_fx_or, const Uint16 become, const int frames, const int starttile, const PASS_REF passage, int *ptilex, int *ptiley );

/// @author ZZ
/// @details This function appends a message to the end-module text
Uint8 AddEndMessage( struct s_chr * pchr, const int message_index, struct s_script_state * pstate );

/// @author ZZ
/// @details This function finds the next tile in the passage, x0 and y0
///    must be set first, and are set on a find.  Returns ego_true or ego_false
///    depending on if it finds one or not
Uint8 FindTileInPassage( const int x0, const int y0, const int tiletype, const PASS_REF passage, int *px1, int *py1 );

/// @author ZF
/// @details This function searches the nearby vincinity for a melee weapon the character can use
CHR_REF FindWeapon( struct s_chr * pchr, float max_distance, IDSZ weap_idsz, ego_bool find_ranged, ego_bool use_line_of_sight );

/// @author ZZ
/// @details This function sets an object's lighting
ego_bool FlashObject( struct s_chr * pchr, Uint8 value );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @author ZZ
/// @details This function sticks a message_offset in the display queue and sets its timer
Uint8 _display_message( const CHR_REF ichr, const PRO_REF iprofile, const int message, struct s_script_state * pstate );
