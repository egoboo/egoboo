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

/// @file physics.h

#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PLATTOLERANCE       50                     ///< Platform tolerance...

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// axis aligned bounding box
struct s_aabb
{
    float mins[3];
    float maxs[3];
};
typedef struct s_aabb aabb_t;

//--------------------------------------------------------------------------------------------
/// Level 0 character "bumper"
/// The simplest collision volume, equivalent to the old-style collision data
/// stored in data.txt
struct s_bumper
{
    float  size;        ///< Size of bumpers
    float  sizebig;     ///< For octagonal bumpers
    float  height;      ///< Distance from head to toe
};
typedef struct s_bumper bumper_t;

//--------------------------------------------------------------------------------------------
/// The various axes for the octagonal bounding box
enum e_octagonal_axes
{
    OCT_X, OCT_Y, OCT_XY, OCT_YX, OCT_Z, OCT_COUNT
};

/// a "vector" that measures distances based on the axes of an octagonal bounding box
typedef float oct_vec_t[OCT_COUNT];

//--------------------------------------------------------------------------------------------
/// generic octagonal bounding box
/// to be used for the Level 1 character "bumper"
/// The best possible octagonal bounding volume. A generalization of the old octagonal bounding box
/// values in data.txt. Computed on the fly.
struct s_oct_bb
{
    oct_vec_t mins,  maxs;
};
typedef struct s_oct_bb oct_bb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the global physics/friction values
extern float hillslide;                   ///< Extra downhill force
extern float airfriction;                 ///< 0.9868 is approximately real world air friction
extern float waterfriction;               ///< Water resistance
extern float slippyfriction;              ///< Friction on tiles that are marked with MPDFX_SLIPPY
extern float noslipfriction;              ///< Friction on normal tiles
extern float gravity;                     ///< Gravitational accel

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int    oct_bb_to_points( oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count );
void   points_to_oct_bb( oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count );
bool_t bumper_to_oct_bb( bumper_t src, oct_bb_t * pdst );
bool_t vec_to_oct_vec( fvec3_t pos, oct_vec_t ovec );

void oct_bb_downgrade( oct_bb_t * psrc, bumper_t bump_base, bumper_t * p_bump, oct_bb_t * pdst );

/// @notes
/// The test functions are designed to detect an interaction with the "least possible" computation.
/// Don't spoil the optimization by calling a test_interaction* function and then a get_depth* function
///
/// The numbers attached to these functions signify the level of precision that is used in calculating the
/// collision. The smallest number, 0, indicates the least precise collision and is equivalent to the "old" egoboo method.
///
/// The _close_ keyword indicates that you are checking for something like whether a character is able to
/// stand on a platform or something where it will tend to fall off if it starts to step off the edge.
///
/// Use the test_platform flag if you want to test whether the objects are close enough for some platform interaction.
/// You could determine whether this should be set to btrue by determining whether either of the objects was a platform
/// and whether the other object could use the platform.
///
/// If you definitely are going to need the depth info, make sure to use the get_depth* functions with the break_out
/// flag set to bfalse. Setting break_out to btrue will make the function faster in the case that there is no collision,
/// but it will leave some of the "depth vector" uncalculated, which might leave it with uninitialized data.

bool_t test_interaction_0      ( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_1      ( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_2      ( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t test_platform );

bool_t get_depth_0      ( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_1      ( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_2      ( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );

