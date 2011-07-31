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

/// @file script_implementation.h
/// @details turns script_functions.* into a bunch of stubs

#include "egoboo_typedef.h"

#include <egolib/_math.h>

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

bool_t waypoint_list_peek( waypoint_list_t * plst, waypoint_t wp );
bool_t waypoint_list_push( waypoint_list_t * plst, int x, int y );
bool_t waypoint_list_reset( waypoint_list_t * plst );
bool_t waypoint_list_clear( waypoint_list_t * plst );
bool_t waypoint_list_empty( waypoint_list_t * plst );
bool_t waypoint_list_finished( waypoint_list_t * plst );
bool_t waypoint_list_advance( waypoint_list_t * plst );

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

bool_t line_of_sight_do( line_of_sight_info_t * plos );
bool_t line_of_sight_with_mesh( line_of_sight_info_t * plos );
bool_t line_of_sight_with_characters( line_of_sight_info_t * plos );

//--------------------------------------------------------------------------------------------
// wrap generic bitwise conversion macros
//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
/// @details formal conversion of a boolean value to an integer (bfalse = 0, btrue = 1)
BIT_FIELD bool_to_bit( bool_t val );

/// @ingroup _bitwise_functions_
/// @details formal conversion of an integer to a boolean value ( 0 = bfalse, all other values are btrue )
bool_t bit_to_bool( BIT_FIELD val );

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
bool_t    BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_ALL_BITS() macro
bool_t    BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_NO_BITS() macro
bool_t    BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for MISSING_BITS() macro
bool_t    BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details set the given bit
BIT_FIELD BIT_FIELD_set_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details clear the given bit
BIT_FIELD BIT_FIELD_clear_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details test the given bit
bool_t    BIT_FIELD_test_one_bit( BIT_FIELD val, size_t which );

/// @ingroup _bitwise_functions_
/// @details adds all bits values to val
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details removes all bits values from val
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details the values has all the given bits
bool_t    BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits );

//--------------------------------------------------------------------------------------------
// wrapped script functions
//--------------------------------------------------------------------------------------------

/// @details ZZ@> This function tells the character where to move next
///
/// @lua AddWaypoint( tmpx = "x position", tmpy = "y position" )
bool_t AddWaypoint( waypoint_list_t * plst, CHR_REF ichr, float pos_x, float pos_y );

/// @details ZF@> Ported the A* path finding algorithm by birdsey and heavily modified it
/// This function adds enough waypoints to get from one point to another
bool_t FindPath( waypoint_list_t * plst, struct s_chr * pchr, float dst_x, float dst_y, bool_t * used_astar_ptr );

/// @details ZZ@> This function modifies tmpx and tmpy, depending on the setting of
/// tmpdistance and tmpturn.  It acts like one of those Compass thing
/// with the two little needle legs
///
/// @lua Compass( tmpturn = "rotation", tmpdistance = "radius" )
bool_t Compass( fvec2_base_t pos, int facing, float distance );

/// @details ZZ@> This function returns the cost of the desired skin upgrade
//
/// @lua tmpx = GetTargetArmorPrice( tmpargument = "skin" )
int GetArmorPrice( struct s_chr * pchr, int skin );

/// @details ZZ@> This function sets the character's ai timer.  50 clicks per second.
/// Used in conjunction with IfTimeOut
///
/// @lua selftimer = UpdateTime( selftimer = "time", tmpargument = "delay" )
Uint32 UpdateTime( Uint32 time_val, int delay );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @details ZZ@> This function breaks the tiles of a passage if there is a character standing
///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.
Uint8 _break_passage( int mesh_fx_or, int become, int frames, int starttile, const PASS_REF passage, int *ptilex, int *ptiley );

/// @details ZZ@> This function appends a message to the end-module text
Uint8 _append_end_text( struct s_chr * pchr, const int message_index, struct s_script_state * pstate );

/// @details ZZ@> This function finds the next tile in the passage, x0 and y0
///    must be set first, and are set on a find.  Returns btrue or bfalse
///    depending on if it finds one or not
Uint8 _find_grid_in_passage( const int x0, const int y0, const int tiletype, const PASS_REF passage, int *px1, int *py1 );

/// @details ZZ@> This function sticks a message_offset in the display queue and sets its timer
Uint8 _display_message( const CHR_REF ichr, const PRO_REF iprofile, const int message, struct s_script_state * pstate );
