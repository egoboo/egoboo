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

/// @file    geometry.h
/// @brief   functions for manipulating geometric primatives
/// @details The frustum code was inspired by Ben Humphrey (DigiBen at http://www.gametutorials.com), who was inspired by
///          Mark Morely (through the now vanished tutorial at http://www.markmorley.com/opengl/frustumculling.html)

#include "../egolib/typedef.h"
#include "../egolib/_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// external types
//--------------------------------------------------------------------------------------------

    struct s_aabb;

//--------------------------------------------------------------------------------------------
// internal types
//--------------------------------------------------------------------------------------------

    struct s_intersection_info;
    typedef struct s_intersection_info intersection_info_t;

    struct s_cube;
    typedef struct s_cube cube_t;

    struct s_sphere;
    typedef struct s_sphere sphere_t;

    struct s_cone;
    typedef struct s_cone cone_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    enum e_geometry_rv
    {
        geometry_error,
        geometry_outside,
        geometry_intersect,
        geometry_inside
    };

// this typedef must be after the enum definition of gcc has a fit
    typedef enum e_geometry_rv geometry_rv;

//--------------------------------------------------------------------------------------------
// a datatype for points
//--------------------------------------------------------------------------------------------

    typedef fvec3_base_t point_base_t;

//--------------------------------------------------------------------------------------------
// a datatype for planes
//--------------------------------------------------------------------------------------------

// the base type of the plane data
    typedef fvec4_base_t plane_base_t;

    bool_t plane_base_normalize( plane_base_t * plane );

//--------------------------------------------------------------------------------------------
// a datatype for spheres
//--------------------------------------------------------------------------------------------

    struct s_sphere
    {
        fvec3_t origin;
        float   radius;
    };

#define SPHERE_INIT_VALS                \
    {                                   \
        ZERO_VECT3, /*fvec3_t pos    */ \
        -1.0f       /*float   radius */ \
    }

    bool_t sphere_self_clear( sphere_t * );

//--------------------------------------------------------------------------------------------
// a datatype for cubes
//--------------------------------------------------------------------------------------------

    struct s_cube
    {
        fvec3_t pos;
        float   size;
    };

//--------------------------------------------------------------------------------------------
// a datatype for cones
//--------------------------------------------------------------------------------------------

    struct s_cone
    {
        fvec3_t origin;
        fvec3_t axis;

        // use these values to pre-calculate trig functions based off of the opening angle
        float   inv_sin;
        float   sin_2;
        float   cos_2;
    };

//--------------------------------------------------------------------------------------------
// a datatype for frustums
//--------------------------------------------------------------------------------------------

    enum e_frustum_planes
    {
        FRUST_PLANE_RIGHT = 0,
        FRUST_PLANE_LEFT,
        FRUST_PLANE_BOTTOM,
        FRUST_PLANE_TOP,
        FRUST_PLANE_BACK,
        FRUST_PLANE_FRONT,
        FRUST_PLANE_COUNT
    };

    typedef plane_base_t frustum_base_t[FRUST_PLANE_COUNT];

//--------------------------------------------------------------------------------------------
// intersection routines
//--------------------------------------------------------------------------------------------

    geometry_rv point_intersects_aabb( const point_base_t pos, const fvec3_base_t corner1, const fvec3_base_t corner2 );

    geometry_rv aabb_intersects_aabb( const struct s_aabb * lhs, const struct s_aabb * rhs );

    geometry_rv plane_intersects_aabb( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs );

    geometry_rv sphere_intersects_sphere( const sphere_t * lhs, const sphere_t * rhs );

    geometry_rv cone_intersects_point( const cone_t * lhs, const fvec3_base_t rhs );
    geometry_rv cone_intersects_sphere( const cone_t * lhs, const sphere_t * rhs );

    geometry_rv frustum_intersects_point( const frustum_base_t pf, const fvec3_base_t pos );
    geometry_rv frustum_intersects_sphere( const frustum_base_t pf, const fvec3_base_t pos, const float radius );
    geometry_rv frustum_intersects_cube( const frustum_base_t pf, const fvec3_base_t pos, const float size );
    geometry_rv frustum_intersects_aabb( const frustum_base_t pf, const fvec3_base_t corner1, const fvec3_base_t corner2 );

//--------------------------------------------------------------------------------------------
// misc routines
//--------------------------------------------------------------------------------------------

/// The distance between a point and a plane
    float plane_point_distance( const plane_base_t plane, const point_base_t pos );

/// find the parametric line where two planes intersect
    bool_t two_plane_intersection( fvec3_base_t dst_pos, fvec3_base_t dst_dir, const plane_base_t p0, const plane_base_t p1 );

/// find the point where 3 planes intersect
    bool_t three_plane_intersection( fvec3_base_t dst_pos, const plane_base_t p0, const plane_base_t p1, const plane_base_t p2 );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define geometry_h
