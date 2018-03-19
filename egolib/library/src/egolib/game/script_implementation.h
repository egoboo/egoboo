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

/// @file egolib/game/script_implementation.h
/// @details turns script_functions.* into a bunch of stubs

#pragma once

#include "egolib/game/egoboo.h"
#include "egolib/AI/WaypointList.h"
#include "egolib/Entities/Forward.hpp"

/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct script_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct waypoint_list_t;

struct line_of_sight_info_t;

//--------------------------------------------------------------------------------------------
// waypoint_list_t
//--------------------------------------------------------------------------------------------


#define WAYTHRESH           (Info<int>::Grid::Size() >> 1)       ///< Threshold for reaching waypoint (GRID_FSIZE/2)

//--------------------------------------------------------------------------------------------
// wrap the BIT_FIELD macros, since lua doesn't recognize bitwise functions
//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_08BITS() macro
uint8_t BIT_FIELD_clip_to_08_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for Ego::Math::clipBits<16>() macro
uint16_t BIT_FIELD_clip_to_16_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_24BITS() macro
uint32_t BIT_FIELD_clip_to_24_bits( BIT_FIELD val );

/// @ingroup _bitwise_functions_
/// @details wrapper for CLIP_TO_32BITS() macro
uint32_t BIT_FIELD_clip_to_32_bits( BIT_FIELD val );

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
bool    BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_ALL_BITS() macro
bool    BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for HAS_NO_BITS() macro
bool    BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details wrapper for MISSING_BITS() macro
bool    BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test );

/// @ingroup _bitwise_functions_
/// @details adds all bits values to val
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details removes all bits values from val
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits );

/// @ingroup _bitwise_functions_
/// @details the values has all the given bits
bool    BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits );

//--------------------------------------------------------------------------------------------
// wrapped script functions
//--------------------------------------------------------------------------------------------

/// @author ZZ
/// @details This function tells the character where to move next
///
/// @lua AddWaypoint( tmpx = "x position", tmpy = "y position" )
bool AddWaypoint( waypoint_list_t& wplst, ObjectRef ichr, float pos_x, float pos_y );

/// @author ZF
/// @details Ported the A* path finding algorithm by birdsey and heavily modified it
/// This function adds enough waypoints to get from one point to another
bool FindPath( waypoint_list_t& wplst, Object * pchr, float dst_x, float dst_y, bool * used_astar_ptr );

/// @author ZZ
/// @details This function modifies tmpx and tmpy, depending on the setting of
/// tmpdistance and tmpturn.  It acts like one of those Compass thing
/// with the two little needle legs
///
/// @lua Compass( tmpturn = "rotation", tmpdistance = "radius" )
bool Compass( Ego::Vector2f& pos, int facing, float distance );

/// @author ZZ
/// @details This function sets the character's ai timer.  50 clicks per second.
/// Used in conjunction with IfTimeOut
///
/// @lua selftimer = UpdateTime( selftimer = "time", tmpargument = "delay" )
uint32_t UpdateTime( uint32_t time_val, int delay );

/// @author ZZ
/// @details This function breaks the tiles of a passage if there is a character standing
///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.
uint8_t BreakPassage( int mesh_fx_or, const uint16_t become, const int frames, const int starttile, const int passageID, int *ptilex, int *ptiley );

/// @author ZZ
/// @details This function appends a message to the end-module text
uint8_t AddEndMessage( Object * pchr, const int message_index, script_state_t * pstate );

/// @author ZZ
/// @details This function finds the next tile in the passage, x0 and y0
///    must be set first, and are set on a find.  Returns true or false
///    depending on if it finds one or not
Uint8 FindTileInPassage( const int x0, const int y0, const int tiletype, const int passageID, int *px1, int *py1 );

/// @author ZF
/// @details This function searches the nearby vincinity for a melee weapon the character can use
ObjectRef FindWeapon( Object * pchr, float max_distance, const IDSZ2& weap_idsz, bool find_ranged, bool use_line_of_sight );

/// @author ZZ
/// @details This function sets an object's lighting
bool FlashObject( Object * pchr, Uint8 value );

/// @details This function restocks the characters ammo, if it needs ammo and if
///    either its parent or type idsz match the given idsz.  This
///    function returns the amount of ammo given.
int RestockAmmo(const ObjectRef character, const IDSZ2& idsz);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @author ZZ
/// @details This function sticks a message_offset in the display queue and sets its timer
Uint8 _display_message( const ObjectRef ichr, const PRO_REF iprofile, const int message, script_state_t * pstate );
