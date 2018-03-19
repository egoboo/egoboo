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
#include "egolib/Math/_Include.hpp"

namespace Ego::Graphics {

#pragma push_macro("far")
#undef far
#pragma push_macro("FAR")
#undef FAR
#pragma push_macro("near")
#undef near
#pragma push_macro("NEAR")
#undef NEAR

/**
 * @brief
 *	A view frustum.
 *	A point is inside the frustum, if its distance from each plane of the frustum is non-negative.
 */
struct Frustum
{
    enum Planes
    {
        RIGHT = 0,
        LEFT,
        BOTTOM,
        TOP,
        BACK,  ///< the back/far plane
        FAR = BACK,
        FRONT, ///< the front/near plane
        NEAR = FRONT,
        COUNT,

        // some aliases
        BEGIN = RIGHT,       ///< The index of the first plane.
        END = FRONT,         ///< The index of the last plane.
        SIDES_BEGIN = RIGHT, ///< The index of the first side (left, right, bottom, top) plane.
                             ///< The side planes have indices from SIDES_BEGIN to SIDES_END.
        SIDES_END = TOP      ///< The index of the last side plane.
    };
    // basic frustum data
	Plane3f _planes[Planes::COUNT];

    // data for intersection optimization
    Point3f _origin;
    Sphere3f _sphere;

public:

    /**
     * @brief
     *  Construct this frustum.
     */
    Frustum();

	/**
     * @brief
     *  Destruct this frustum.
     */
    virtual ~Frustum();

	/**
	 * @brief
	 *  Get the relation of a point to this frustum.
	 * @param self
	 *  this frustum
	 * @param point
	 *  the point
	 * @param doEnds
	 *  if @a false, the far and the near plane are ignored
	 * @remark
	 *  If a point is behind one of the frustum planes (i.e. its distance to the plane is
	 *  negative), then the point is outside the frustum, otherwise it is inside the frustum.
	 */
	Math::Relation intersects(const Point3f& point, const bool doEnds) const;

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
	Math::Relation intersects(const Sphere3f& sphere, const bool doEnds) const;

    /**
     * @brief
     *  Get the relation of a cube to this frustum.
     * @param self
     *  this frustum
     * @param center, size
     *  the center and the size of the cube
     * @param doEnds
     *  if @a false, the far and the near plane are ignored
     * @return
     *  wether the cube is inside the frustum, partially overlaps with the frustum or is outside the frustum.
     *  <ul>
     *      <li>geometry_error     - an error occured</li>
     *      <li>geometry_outside   - the cube is outside the frustum</li>
     *      <li>geometry_intersect - the cube and the frustum partially overlap</li>
     *      <li>geometry_inside    - the cube is completely inside the frustum</li>
     *  </ul>
     * @todo
     *  Replace <tt>const Vector3f& position</tt> and <tt>const float size</tt> by <tt>const Cube3f& cube</tt>.
     */
	Math::Relation intersects(const AxisAlignedCube3f& cube, const bool doEnds) const;

    /**
     * @brief
     *  Get the relation of an AABB to this frustum.
     * @warning
     *  This is not optimized.
     * @return
     *  wether the AABB is inside the frustum, partially overlaps with the frustum or is outside the frustum.
     *  <ul>
     *      <li>geometry_error     - an error occured</li>
     *      <li>geometry_outside   - the AABB is outside the frustum</li>
     *      <li>geometry_intersect - the AABB and the frustum partially overlap</li>
     *      <li>geometry_inside    - the AABB is completely inside the frustum</li>
     *	</ul>
     */
	Math::Relation intersects_aab(const Point3f& corner1, const Point3f& corner2, bool doEnds) const;
	Math::Relation intersects(const AxisAlignedBox3f& aabb, bool doEnds) const;

	/// @todo Should return geometry_rv.
	bool intersects(const oct_bb_t& oct, const bool doEnds) const;

    /**
    * @brief
    *	Call this every time the camera moves or the projection matrix changes to update the frustum
    * @param projection
    *	the projection matrix
    * @param view
    *	the view matrix
    */
    void calculate(const Matrix4f4f& projection, const Matrix4f4f& view);
protected:

    /**
     * @brief
     *  Extract the view frustum planes for given projection, view and world matrices.
     * @param projection, view, world
     *  the projection, view and world matrices
     * @param [out] left, right, bottom, top, near, far
     *  the view frustum planes
     */
    static void calculatePlanes(const Matrix4f4f& projection, const Matrix4f4f& view, const Matrix4f4f& world, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far);


    /**
     * @brief
     *  Extract the view frustum planes for given vprojection and view matrix.
     * @param projection, view
     *  the projection and view matrices
     * @param [out] left, right, bottom, top, near, far
     *  the view frustum planes
     */
    static void calculatePlanes(const Matrix4f4f& projection, const Matrix4f4f& view, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far);

    /**
     * @brief
     *  Extract the view frustum planes from the combined view and projection matrix.
     * @param matrix combined view and projection matrix
     *  this must be either a) the projection matrix, b) the combined view projection matrix or c) the combined world view projection matrix
     * @param [out] left, right, bottom, top, near, far
     *  the view frustum planes
     */
    static void calculatePlanes(const Matrix4f4f& matrix, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far);

};

#pragma pop_macro("NEAR")
#pragma pop_macro("near")
#pragma pop_macro("FAR")
#pragma pop_macro("far")

} // namespace Ego::Graphics
