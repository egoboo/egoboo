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

/// @file egolib/frustum.h
/// @brief integrating the basic frustum object into Egoboo algorithms

#include "egolib/geometry.h"
#include "egolib/bbox.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

    struct s_oct_bb;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

    struct s_ego_aabb;
    typedef struct s_ego_aabb ego_aabb_t;

    struct s_egolib_frustum;
    typedef struct s_egolib_frustum egolib_frustum_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_ego_aabb
    {
        sphere_t sphere;
        aabb_t   data;
    };

#define EGO_AABB_INIT_VALS                 \
    {                                          \
        SPHERE_INIT_VALS, /*sphere_t sphere */ \
        AABB_INIT_VALS    /*aabb_t   data   */ \
    }

    ego_aabb_t * ego_aabb_ctor( ego_aabb_t * );
    ego_aabb_t * ego_aabb_dtor( ego_aabb_t * );
    bool ego_aabb_self_clear( ego_aabb_t * );
    bool ego_aabb_is_clear( const ego_aabb_t * pdst );

    bool ego_aabb_self_union( ego_aabb_t * pdst, const ego_aabb_t * psrc );
    bool ego_aabb_lhs_contains_rhs( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr );
    bool ego_aabb_overlap( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr );

    bool ego_aabb_copy( ego_aabb_t * pdst, const ego_aabb_t * psrc );
    bool ego_aabb_from_oct_bb( ego_aabb_t * dst, const struct s_oct_bb * src );

    bool ego_aabb_validate( ego_aabb_t * rhs );
    bool ego_aabb_test( const ego_aabb_t * rhs );

//--------------------------------------------------------------------------------------------
    struct s_egolib_frustum
    {
        // basic frustum data
        frustum_base_t data;

        // data for intersection optimization
        fvec3_t   origin;
        sphere_t  sphere;
        cone_t    cone;
    };

/// Call this every time the camera moves to update the frustum
    egolib_rv egolib_frustum_calculate( egolib_frustum_t * pfrust, const float proj[], const float modl[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Call this every time the camera moves to update the frustum
    geometry_rv egolib_frustum_intersects_ego_aabb( const egolib_frustum_t * pfrust, const ego_aabb_t * paabb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_frustum_h

