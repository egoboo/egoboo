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

#pragma once

#include "egolib/geometry.h"
#include "egolib/bbox.h"
#include "egolib/Math/AABB.h"
#include "egolib/bv.h"
#include "egolib/Math/Cone.hpp"
#include "egolib/Math/Cube.hpp"
#include "egolib/Math/Matrix44.hpp"


/**
 * @brief
 *	A view frustum.
 *	A point is inside the frustum, if its distance from each plane of the frustum is non-negative.
 */
struct egolib_frustum_t
{
    enum Planes
    {
        RIGHT = 0,
        LEFT,
        BOTTOM,
        TOP,
        BACK,  ///< the back/far plane
        FRONT, ///< the front/near plane
        COUNT,

        // some aliases
        BEGIN = RIGHT,       ///< The index of the first plane.
        END = FRONT,         ///< The index of the last plane.
        SIDES_BEGIN = RIGHT, ///< The index of the first side (left, right, bottom, top) plane.
                             ///< The side planes have indices from SIDES_BEGIN to SIDES_END.
        SIDES_END = TOP      ///< The index of the last side plane.
    };
    typedef plane_base_t base_t[Planes::COUNT];
    // basic frustum data
    base_t _planes;

    // data for intersection optimization
    fvec3_t _origin;
    sphere_t _sphere;
    cone_t _cone;
	/**
	 * @brief
	 *	Get the relation of a point to this frustum.
	 * @param self
	 *	this frustum
	 * @param point
	 *	the point
	 * @param doEnds
	 *	if @a false, the far and the near plane are ignored
	 * @remark
	 *	If a point is behind one of the frustum planes (i.e. its distance to the plane is
	 *	negative), then the point is outside the frustum, otherwise it is inside the frustum.
	 */
	geometry_rv intersects_point(const fvec3_t& point, const bool doEnds) const;

	/**
	 * @brief
	 *	Get the relation of a sphere to this frustum.
	 * @param sphere
	 *	the sphere
	 * @param doEnds
	 *	if @a false, the far and the near plane are ignored
	 * @return
	 *	wether the sphere is inside the frustum, partially overlaps with the frustum or is outside the frustum.
	 *	<ul>
	 *		<li>geometry_error     - an error occured</li>
	 *		<li>geometry_outside   - the sphere is outside the frustum</li>
	 *		<li>geometry_intersect - the sphere and the frustum partially overlap</li>
	 *		<li>geometry_inside    - the sphere is completely inside the frustum</li>
	 *	</ul>
	 * @remark
	 *	If a the sphere is outside one plane farther than its radius, it is outside the frustum.
	 */
	geometry_rv intersects_sphere(const sphere_t& sphere, const bool doEnds) const;

	/**
	 * @brief
	 *	Get the relation of a cube to this frustum.
	 * @param self
	 *	this frustum
	 * @param center, size
	 * 	the center and the size of the cube
	 * @param doEnds
	 *	if @a false, the far and the near plane are ignored
	 * @return
	 *	wether the cube is inside the frustum, partially overlaps with the frustum or is outside the frustum.
	 *	<ul>
	 *		<li>geometry_error     - an error occured</li>
	 *		<li>geometry_outside   - the cube is outside the frustum</li>
	 *		<li>geometry_intersect - the cube and the frustum partially overlap</li>
	 *		<li>geometry_inside    - the cube is completely inside the frustum</li>
	 *	</ul>
	 * @todo
	 *	Replace <tt>const fvec3_t& position</tt> and <tt>const float size</tt> by <tt>const cube_t& cube</tt>.
	 */
	geometry_rv intersects_cube(const fvec3_t& center, const float size, const bool doEnds) const;

	/**
	 * @brief
	 *	Get the relation of an AABB to this frustum.
	 * @warning
	 *	This is not optimized.
	 * @return
	 *	wether the AABB is inside the frustum, partially overlaps with the frustum or is outside the frustum.
	 *	<ul>
	 *		<li>geometry_error     - an error occured</li>
	 *		<li>geometry_outside   - the AABB is outside the frustum</li>
	 *		<li>geometry_intersect - the AABB and the frustum partially overlap</li>
	 *		<li>geometry_inside    - the AABB is completely inside the frustum</li>
	 *	</ul>
	 */
	 geometry_rv intersects_aabb(const fvec3_t& corner1, const fvec3_t& corner2, const bool doEnds) const;

	/**
	 * @brief
	 *	Get the relation of a BV to this frustum.
	 * @return
	 *	wether the BV is inside the frustum, intersects with the frustum or is outside the frustum.
	 *	<ul>
	 *		<li>geometry_error     - an error occured</li>
	 *		<li>geometry_outside   - the BV volume is outside the frustum</li>
	 *		<li>geometry_intersect - the BV and the frustum partially overlap</li>
	 *		<li>geometry_inside    - the bounding volume is completely inside the frustum</li>
	 */
	geometry_rv intersects_bv(const bv_t *bv, bool doEnds) const;

	/// @todo Should return geometry_rv.
	bool intersects_oct(const oct_bb_t *oct, const bool doEnds) const;

    /**
    * @brief
    *	Call this every time the camera moves or the projection matrix changes to update the frustum
    * @param projection
    *	the projection matrix
    * @param view
    *	the view matrix
    */
    void calculate(const fmat_4x4_t& projection, const fmat_4x4_t& view);
protected:
    /// Call this every time the camera moves to update the frustum
    static void calculate(base_t planes, const fmat_4x4_t& projection, const fmat_4x4_t& view);

};

