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

/// @file  egolib/bbox.h
/// @brief A small "library" for dealing with various bounding boxes.

#pragma once

#include "egolib/vec.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------

/// Level 0 character "bumper"
/// The simplest collision volume, equivalent to the old-style collision data
/// stored in data.txt
    struct bumper_t
    {
        float  size;        ///< Size of bumpers
        float  size_big;    ///< For octagonal bumpers
        float  height;      ///< Distance from head to toe
    };

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
    struct oct_bb_t
    {
        bool  empty;
        oct_vec_t mins,
		          maxs;
    };

    egolib_rv  oct_bb_interpolate( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, float flip );

#define OCT_BB_INIT_VALS { true, OCT_VEC_INIT_VALS, OCT_VEC_INIT_VALS }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @details A convex poly representation of an object volume
    struct OVolume_t
    {
        int        lod;             ///< the level of detail (LOD) of this volume
        bool   needs_shape;     ///< is the shape data valid?
        bool   needs_position;  ///< Is the position data valid?

        oct_bb_t   oct;
    };

    OVolume_t * OVolume__ctor( OVolume_t * );
    OVolume_t OVolume_merge( const OVolume_t * pv1, const OVolume_t * pv2 );
    OVolume_t OVolume_intersect( const OVolume_t * pv1, const OVolume_t * pv2 );
//bool    OVolume_draw( OVolume_t * cv, bool draw_square, bool draw_diamond );
//bool    OVolume_shift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );
//bool    OVolume_unshift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );

    bool    OVolume_refine( OVolume_t * pov, fvec3_t * pcenter, float * pvolume );

//--------------------------------------------------------------------------------------------

    struct OVolume_Tree_t { OVolume_t leaf[8]; };

//--------------------------------------------------------------------------------------------

/// @details A convex polygon representation of the collision of two objects
    struct CVolume_t
    {
        float            volume;
        fvec3_t          center;
        OVolume_t        ov;
        OVolume_Tree_t  *tree;
    };

    CVolume_t * CVolume__blank( CVolume_t * );
    bool CVolume_ctor( CVolume_t * , const OVolume_t * pva, const OVolume_t * pvb );
    bool CVolume_refine( CVolume_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// type conversion routines

//bool bumper_to_oct_bb_1( const bumper_t src, const fvec3_t vel, oct_bb_t * pdst );

    egolib_rv oct_bb_downgrade( const oct_bb_t * psrc_bb, const bumper_t bump_stt, const bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb );

    int    oct_bb_to_points( const oct_bb_t * pbmp, fvec4_t pos[], size_t pos_count );
    void   points_to_oct_bb( oct_bb_t * pbmp, const fvec4_t pos[], const size_t pos_count );

//--------------------------------------------------------------------------------------------
//inline
//--------------------------------------------------------------------------------------------

bool oct_vec_ctor(oct_vec_t ovec, const fvec3_t& pos);
bool oct_vec_add_fvec3(const oct_vec_t osrc, const fvec3_t& fvec, oct_vec_t odst);
bool oct_vec_self_add_fvec3(oct_vec_t osrc, const fvec3_t& fvec);

bool oct_vec_self_clear( oct_vec_t * ovec );

oct_bb_t *oct_bb_ctor( oct_bb_t * pobb );
egolib_rv oct_bb_set_bumper( oct_bb_t * pobb, const bumper_t src );
egolib_rv oct_bb_copy( oct_bb_t * pdst, const oct_bb_t * psrc );
egolib_rv oct_bb_validate( oct_bb_t * pobb );
bool oct_bb_empty_raw( const oct_bb_t * pbb );
bool oct_bb_empty( const oct_bb_t * pbb );
egolib_rv  oct_bb_set_ovec( oct_bb_t * pobb, const oct_vec_t ovec );
oct_bb_t * oct_bb_ctor_index( oct_bb_t * pobb, int index );
egolib_rv oct_bb_copy_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
egolib_rv oct_bb_validate_index( oct_bb_t * pobb, int index );
bool oct_bb_empty_index_raw( const oct_bb_t * pbb, int index );
bool oct_bb_empty_index( const oct_bb_t * pbb, int index );
egolib_rv oct_bb_union_index( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst, int index );
egolib_rv oct_bb_intersection_index( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, int index );
egolib_rv oct_bb_self_union_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
egolib_rv oct_bb_self_intersection_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
egolib_rv oct_bb_union( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst );
egolib_rv oct_bb_intersection( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst );
egolib_rv oct_bb_self_union( oct_bb_t * pdst, const oct_bb_t * psrc );
egolib_rv oct_bb_self_intersection( oct_bb_t * pdst, const oct_bb_t * psrc );

/**
 * @brief
 *	Translate this bounding box.
 * @param src
 *	the source bounding box
 * @param t
 *	the translation vector
 * @param dst
 *	the target bounding box
 */
egolib_rv oct_bb_add_fvec3(const oct_bb_t *src, const fvec3_t& t, oct_bb_t *dst);

/**
 * @brief
 *	Translate this bounding box.
 * @param dst
 * 	the bounding box
 * @param t
 *	the translation vector
 */
egolib_rv oct_bb_self_add_fvec3(oct_bb_t *dst, const fvec3_t& t);

egolib_rv oct_bb_add_ovec( const oct_bb_t * psrc, const oct_vec_t ovec, oct_bb_t * pdst );
egolib_rv oct_bb_self_add_ovec( oct_bb_t * pdst, const oct_vec_t ovec );
egolib_rv oct_bb_self_sum_ovec( oct_bb_t * pdst, const oct_vec_t ovec );
egolib_rv oct_bb_self_grow( oct_bb_t * pdst, const oct_vec_t ovec );
bool oct_bb_point_inside( const oct_bb_t * pobb, const oct_vec_t ovec );
bool oct_bb_lhs_contains_rhs( const oct_bb_t * plhs, const oct_bb_t * prhs );
bool oct_bb_get_mids( const oct_bb_t * pbb, oct_vec_t mids );