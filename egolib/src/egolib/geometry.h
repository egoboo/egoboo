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

/// @file    egolib/geometry.h
/// @brief   functions for manipulating geometric primitives
/// @details The frustum code was inspired by Ben Humphrey (DigiBen at http://www.gametutorials.com), who was inspired by
///          Mark Morely (through the now vanished tutorial at http://www.markmorley.com/opengl/frustumculling.html)

#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"
#include "egolib/math/Sphere.h"

// Forward declaration.
struct aabb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    enum e_geometry_rv
    {
        geometry_error,
        geometry_outside,
        geometry_intersect,
        geometry_inside
    };

// this typedef must be after the enum definition or gcc has a fit
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

    bool plane_base_normalize( plane_base_t * plane );

//--------------------------------------------------------------------------------------------
// a datatype for cones
//--------------------------------------------------------------------------------------------

    struct cone_t
    {
        fvec3_t origin;
        fvec3_t axis;

        // use these values to pre-calculate trig functions based off of the opening angle
        float   inv_sin;
        float   sin_2;
        float   cos_2;

        cone_t() :
            origin(0, 0, 0),
            axis(0, 0, 0),
            inv_sin(0.0f),
            sin_2(0.0f),
            cos_2(0.0f)
        {
            //ctor
        }
		/**
		 * @brief
		 *	Assign this cone the values of another cone.
		 * @param other
		 *	the other cone
		 * @post
		 *	This cone was assigned the values of the other cone.
		 */
		void assign(const cone_t& other)
		{
			origin = other.origin;
			axis = other.axis;
			inv_sin = other.inv_sin;
			sin_2 = other.sin_2;
			cos_2 = other.cos_2;
		}
		/**
		 * @brief
		 *	Assign this cone the values of another cone.
		 * @param other
		 *	the other cone
		 * @return
		 *	this cone
		 * @post
		 *	This cone was assigned the values of the other cone.
		 */
		cone_t& operator=(const cone_t& other)
		{
			assign(other);
			return *this;
		}
    };

//--------------------------------------------------------------------------------------------
// a datatype for frustums
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// intersection routines
//--------------------------------------------------------------------------------------------

    geometry_rv point_intersects_aabb( const point_base_t pos, const fvec3_base_t corner1, const fvec3_base_t corner2 );

    geometry_rv aabb_intersects_aabb(const aabb_t& lhs, const aabb_t& rhs);

	geometry_rv plane_intersects_aabb_min(const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs);
	geometry_rv plane_intersects_aabb_max(const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs);
/**
 * @brief
 *	Get if a plane and an AABB intersect.
 * @param plane
 *	the plane
 * @param mins, maxs
 *	the AABB
 * @todo
 *	Replace <tt>const fvec3_base_t mins</tt> and <tt>const fvec3_base_t maxs</tt> by <tt>const aabb_t& aabb</tt>.
 * @todo
 *	Rename <tt>plane</tt> to <tt>lhs</tt> and <tt>aabb</tt> to <tt>rhs</tt>.
 */
geometry_rv plane_intersects_aabb(const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs);

/**
 * @brief
 *	Get if two spheres intersect.
 * @param lhs
 *	a sphere
 * @param rhs
 *	the other sphere
 */
geometry_rv sphere_intersects_sphere(const sphere_t *lhs, const sphere_t *rhs);

/**
 * @brief
 *	Get if a cone and a point intersect.
 * @param lhs
 *	a cone
 * @param rhs
 *	a point
 */
geometry_rv cone_intersects_point(const cone_t * lhs, const fvec3_t& rhs);
geometry_rv cone_intersects_sphere(const cone_t * lhs, const sphere_t * rhs);


//--------------------------------------------------------------------------------------------
// misc routines
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	Get the plane normal vector of this plane.
 * @param self
 *	this plane
 * @return
 *	the normal vector of this plane
 * @remark
 *	The normal vector of a plane is a unit vector.
 */
fvec3_t plane_get_normal(const plane_base_t self);

/// The distance between a point and a plane
float plane_point_distance( const plane_base_t plane, const point_base_t pos );

/// find the parametric line where two planes intersect
bool two_plane_intersection( fvec3_t& dst_pos, fvec3_t& dst_dir, const plane_base_t p0, const plane_base_t p1 );

/// find the point where 3 planes intersect
bool three_plane_intersection( fvec3_t& dst_pos, const plane_base_t p0, const plane_base_t p1, const plane_base_t p2 );