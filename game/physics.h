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
struct s_chr;
struct s_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PLATTOLERANCE       50                     ///< Platform tolerance...

enum
{
    PHYS_PLATFORM_NONE = 0,
    PHYS_PLATFORM_OBJ1 = ( 1 << 0 ),
    PHYS_PLATFORM_OBJ2 = ( 1 << 1 )
};

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
extern float   hillslide;                   ///< Extra downhill force
extern float   airfriction;                 ///< 0.9868 is approximately real world air friction
extern float   waterfriction;               ///< Water resistance
extern float   slippyfriction;              ///< Friction on tiles that are marked with MPDFX_SLIPPY
extern float   noslipfriction;              ///< Friction on normal tiles
extern float   gravity;                     ///< Gravitational accel
extern float   platstick;                   ///< Friction between characters and platforms
extern fvec3_t windspeed;                   ///< The game's windspeed

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_expand_oct_bb( oct_bb_t src, fvec3_t vel, float tmin, float tmax, oct_bb_t * pdst );
bool_t phys_expand_chr_bb( struct s_chr * pchr, float tmin, float tmax, oct_bb_t * pdst );
bool_t phys_expand_prt_bb( struct s_prt * pprt, float tmin, float tmax, oct_bb_t * pdst );

bool_t phys_estimate_chr_chr_normal( oct_vec_t opos_a, oct_vec_t opos_b, oct_vec_t odepth, float exponent, fvec3_base_t nrm );
bool_t phys_intersect_oct_bb( oct_bb_t src1, fvec3_t pos1, fvec3_t vel1, oct_bb_t src2, fvec3_t pos2, fvec3_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax );

