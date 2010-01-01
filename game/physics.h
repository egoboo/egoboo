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

#include "bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PLATTOLERANCE       50                     ///< Platform tolerance...

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Data for doing the physics in bump_all_objects()
/// @details should prevent you from being bumped into a wall
struct s_phys_data
{
    fvec3_t        apos_0, apos_1;
    fvec3_t        avel;

    float          bumpdampen;                    ///< "Mass" = weight / bumpdampen
    Uint32         weight;                        ///< Weight
    float          dampen;                        ///< Bounciness
};
typedef struct s_phys_data phys_data_t;

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

bool_t test_interaction_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform );
bool_t test_interaction_close_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t test_platform );

bool_t get_depth_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_1( oct_bb_t cv_a,   fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );
bool_t get_depth_close_2( oct_bb_t cv_a,   fvec3_t pos_a, oct_bb_t   cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth );

bool_t phys_estimate_chr_chr_normal( oct_vec_t opos_a, oct_vec_t opos_b, oct_vec_t odepth, float exponent, fvec3_base_t nrm );