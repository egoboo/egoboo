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

///
/// @file bbox.h
/// @brief A small "library" for dealing with various bounding boxes

#include "egoboo_math.h"

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
    float  size_big;     ///< For octagonal bumpers
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
typedef float * oct_vec_base_t;
typedef float oct_vec_t[OCT_COUNT];

#define OCT_VEC_INIT_VALS { 0,0,0,0,0 }

//--------------------------------------------------------------------------------------------

/// generic octagonal bounding box
/// to be used for the Level 1 character "bumper"
/// The best possible octagonal bounding volume. A generalization of the old octagonal bounding box
/// values in data.txt. Computed on the fly.
struct s_oct_bb
{
    bool_t    empty;
    oct_vec_t mins,  maxs;
};
typedef struct s_oct_bb oct_bb_t;

egoboo_rv  oct_bb_interpolate( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, float flip );

#define OCT_BB_INIT_VALS { btrue, OCT_VEC_INIT_VALS, OCT_VEC_INIT_VALS }

//--------------------------------------------------------------------------------------------
struct s_ego_aabb
{
    int    sub_used;
    float  weight;

    bool_t used;
    int    level;
    int    address;

    aabb_t  bb;
};

typedef struct s_ego_aabb ego_aabb_t;

//--------------------------------------------------------------------------------------------
struct s_aabb_lst
{
    int       count;
    ego_aabb_t * list;
};
typedef struct s_aabb_lst aabb_lst_t;

EGO_CONST aabb_lst_t * aabb_lst_ctor( aabb_lst_t * lst );
EGO_CONST aabb_lst_t * aabb_lst_dtor( aabb_lst_t * lst );
EGO_CONST aabb_lst_t * aabb_lst_renew( aabb_lst_t * lst );
EGO_CONST aabb_lst_t * aabb_lst_alloc( aabb_lst_t * lst, int count );

//--------------------------------------------------------------------------------------------
struct s_aabb_ary
{
    int         count;
    aabb_lst_t * list;
};
typedef struct s_aabb_ary aabb_ary_t;

EGO_CONST aabb_ary_t * bbox_ary_ctor( aabb_ary_t * ary );
EGO_CONST aabb_ary_t * bbox_ary_dtor( aabb_ary_t * ary );
EGO_CONST aabb_ary_t * bbox_ary_renew( aabb_ary_t * ary );
EGO_CONST aabb_ary_t * bbox_ary_alloc( aabb_ary_t * ary, int count );

//--------------------------------------------------------------------------------------------

/// @details A convex poly representation of an object volume
struct s_OVolume
{
    int      lod;             ///< the level of detail (LOD) of this volume
    bool_t   needs_shape;     ///< is the shape data valid?
    bool_t   needs_position;  ///< Is the position data valid?

    oct_bb_t oct;
};
typedef struct s_OVolume OVolume_t;

OVolume_t OVolume_merge( const OVolume_t * pv1, const OVolume_t * pv2 );
OVolume_t OVolume_intersect( const OVolume_t * pv1, const OVolume_t * pv2 );
//bool_t    OVolume_draw( OVolume_t * cv, bool_t draw_square, bool_t draw_diamond );
//bool_t    OVolume_shift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );
//bool_t    OVolume_unshift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );

bool_t    OVolume_refine( OVolume_t * pov, fvec3_t * pcenter, float * pvolume );

//--------------------------------------------------------------------------------------------
struct s_OVolume_Tree { OVolume_t leaf[8]; };
typedef struct s_OVolume_Tree OVolume_Tree_t;

//--------------------------------------------------------------------------------------------

/// @details A convex polygon representation of the collision of two objects
struct s_CVolume
{
    float            volume;
    fvec3_t          center;
    OVolume_t        ov;
    OVolume_Tree_t * tree;
};
typedef struct s_CVolume CVolume_t;

bool_t CVolume_ctor( CVolume_t * pcv, const OVolume_t * pva, const OVolume_t * pvb );
bool_t CVolume_refine( CVolume_t * pcv );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// type conversion routines

//bool_t bumper_to_oct_bb_1( const bumper_t src, const fvec3_t vel, oct_bb_t * pdst );

egoboo_rv oct_bb_downgrade( const oct_bb_t * psrc_bb, const bumper_t bump_stt, const bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb );

int    oct_bb_to_points( const oct_bb_t * pbmp, fvec4_t pos[], size_t pos_count );
void   points_to_oct_bb( oct_bb_t * pbmp, const fvec4_t pos[], const size_t pos_count );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_bbox_h
