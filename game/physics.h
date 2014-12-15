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

/// @file game/physics.h

#include "game/egoboo_typedef.h"

#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_chr;
struct s_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_orientation;
typedef struct s_orientation orientation_t;

struct s_apos;
typedef struct s_apos apos_t;

struct s_phys_data;
typedef struct s_phys_data phys_data_t;

struct s_breadcrumb;
typedef struct s_breadcrumb breadcrumb_t;

struct s_breadcrumb_list;
typedef struct s_breadcrumb_list breadcrumb_list_t;

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
struct s_orientation
{
    FACING_T       facing_z;                        ///< Character's z-rotation 0 to 0xFFFF
    FACING_T       map_twist_facing_y;                    ///< Character's y-rotation 0 to 0xFFFF
    FACING_T       map_twist_facing_x;                    ///< Character's x-rotation 0 to 0xFFFF
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_apos
{
    fvec3_t mins;
    fvec3_t maxs;
    fvec3_t sum;
};

ego_bool apos_self_union( apos_t * lhs, apos_t * rhs );
ego_bool apos_self_union_fvec3( apos_t * lhs, const fvec3_base_t rhs );
ego_bool apos_self_union_index( apos_t * lhs, const float val, const int index );
ego_bool apos_evaluate( const apos_t * src, fvec3_base_t dst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data for doing the physics in bump_all_objects()
/// @details should prevent you from being bumped into a wall
struct s_phys_data
{
    apos_t         aplat, acoll;
    fvec3_t        avel;

    float          bumpdampen;                    ///< "Mass" = weight / bumpdampen
    Uint32         weight;                        ///< Weight
    float          dampen;                        ///< Bounciness
};

phys_data_t * phys_data_clear( phys_data_t * pphys );
phys_data_t * phys_data_ctor( phys_data_t * pphys );

phys_data_t * phys_data_sum_aplat( phys_data_t * pphys, const fvec3_base_t vec );
phys_data_t * phys_data_sum_acoll( phys_data_t * pphys, const fvec3_base_t vec );
phys_data_t * phys_data_sum_avel( phys_data_t * pphys, const fvec3_base_t vec );

phys_data_t * phys_data_sum_aplat_index( phys_data_t * pphys, const float val, const int index );
phys_data_t * phys_data_sum_acoll_index( phys_data_t * pphys, const float val, const int index );
phys_data_t * phys_data_sum_avel_index( phys_data_t * pphys, const float val, const int index );

//--------------------------------------------------------------------------------------------
struct s_breadcrumb
{
    ego_bool         valid;                    /// is this position valid
    fvec3_t        pos;                      ///< A stored safe position
    Uint32         grid;                     ///< the grid index of this position
    float          radius;                   ///< the size of the object at this position
    float          bits;                     ///< the collision buts of the object at this position
    Uint32         time;                     ///< the time when the breadcrumb was created
    Uint32         id;                       ///< an id for differentiating the timing of several events at the same "time"
};

breadcrumb_t * breadcrumb_init_chr( breadcrumb_t * bc, struct s_chr * pchr );
breadcrumb_t * breadcrumb_init_prt( breadcrumb_t * bc, struct s_prt * pprt );

int            breadcrumb_cmp( const void * lhs, const void * rhs );

//--------------------------------------------------------------------------------------------
#define MAX_BREADCRUMB 32

struct s_breadcrumb_list
{
    ego_bool       on;
    int          count;
    breadcrumb_t lst[MAX_BREADCRUMB];
};

void           breadcrumb_list_validate( breadcrumb_list_t * lst );
ego_bool         breadcrumb_list_add( breadcrumb_list_t * lst, breadcrumb_t * pnew );
breadcrumb_t * breadcrumb_list_last_valid( breadcrumb_list_t * lst );
breadcrumb_t * breadcrumb_list_alloc( breadcrumb_list_t * lst );
breadcrumb_t * breadcrumb_list_oldest_grid( breadcrumb_list_t * lst, Uint32 match_grid );
breadcrumb_t * breadcrumb_list_oldest( breadcrumb_list_t * lst );
breadcrumb_t * breadcrumb_list_newest( breadcrumb_list_t * lst );
void           breadcrumb_list_compact( breadcrumb_list_t * lst );
ego_bool         breadcrumb_list_empty( const breadcrumb_list_t * lst );
ego_bool         breadcrumb_list_full( const breadcrumb_list_t *  lst );

//--------------------------------------------------------------------------------------------
// the global physics/friction values

#define STANDARD_GRAVITY -1.0f              ///< The ordinary amount of gravity

extern float   hillslide;                   ///< Extra downhill force
extern float   airfriction;                 ///< 0.9868 is approximately real world air friction
extern float   waterfriction;               ///< Water resistance
extern float   slippyfriction;              ///< Friction on tiles that are marked with MAPFX_SLIPPY
extern float   noslipfriction;              ///< Friction on normal tiles
extern float   gravity;                     ///< Gravitational accel
extern float   platstick;                   ///< Friction between characters and platforms
extern fvec3_t windspeed;                   ///< The game's windspeed
extern fvec3_t waterspeed;                  ///< The game's waterspeed

extern const float air_friction;            ///< gives the same terminal velocity in terms of the size of the game characters
extern const float ice_friction;            ///< estimte if the friction on ice

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bool phys_expand_oct_bb( const oct_bb_t * src, const fvec3_base_t vel, const float tmin, const float tmax, oct_bb_t * pdst );
ego_bool phys_expand_chr_bb( struct s_chr * pchr, float tmin, float tmax, oct_bb_t * pdst );
ego_bool phys_expand_prt_bb( struct s_prt * pprt, float tmin, float tmax, oct_bb_t * pdst );

ego_bool phys_estimate_collision_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_base_t nrm, float * tmin );
ego_bool phys_estimate_pressure_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_base_t nrm, float * depth );

ego_bool phys_intersect_oct_bb( const oct_bb_t * src1, const fvec3_base_t pos1, const fvec3_base_t vel1, const oct_bb_t * src2, const fvec3_base_t pos2, const fvec3_base_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax );

ego_bool get_chr_mass( struct s_chr * pchr, float * wt );
ego_bool get_prt_mass( struct s_prt * pprt, struct s_chr * pchr, float * wt );
void   get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b );

