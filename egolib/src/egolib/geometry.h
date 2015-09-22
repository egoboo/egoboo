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

#include "egolib/platform.h"
#include "egolib/Math/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Ego {
namespace Math {
enum class Relation {
	error,
	outside,
	intersect,
	inside
};
} // namespace Math
} // namespace Ego

//--------------------------------------------------------------------------------------------
// intersection routines
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Get the relation of a point to an AABB.
 * @param point
 *  the point
 * @param corner1, corner2
 *  the AABB
 * @todo
 *  @a corner1 and @a corner2 should be replaced by @ AABB3f.
 * @todo
 *  Document return value.
 */
Ego::Math::Relation point_intersects_aabb(const Vector3f& point, const Vector3f& corner1, const Vector3f& corner2);

Ego::Math::Relation aabb_intersects_aabb(const AABB3f& lhs, const AABB3f& rhs);

Ego::Math::Relation plane_intersects_aabb_min(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs);
Ego::Math::Relation plane_intersects_aabb_max(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs);

/**
 * @brief
 *	Get if a plane and an AABB intersect.
 * @param plane
 *	the plane
 * @param mins, maxs
 *	the AABB
 * @todo
 *	Replace <tt>const Vector3f& mins</tt> and <tt>const Vector3f& maxs</tt> by <tt>const AABB3f& aabb</tt>.
 * @todo
 *	Rename <tt>plane</tt> to <tt>lhs</tt> and <tt>aabb</tt> to <tt>rhs</tt>.
 */
Ego::Math::Relation plane_intersects_aabb(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs);

/**
 * @brief
 *  Get if two spheres intersect.
 * @param lhs
 *  a sphere
 * @param rhs
 *  the other sphere
 */
Ego::Math::Relation sphere_intersects_sphere(const Sphere3f& lhs, const Sphere3f& rhs);

/**
 * @brief
 *	Get if a cone and a point intersect.
 * @param lhs
 *	a cone
 * @param rhs
 *	a point
 */
Ego::Math::Relation cone_intersects_point(const Cone3f& lhs, const Vector3f& rhs);

/**
 * @brief
 *  Get the relation of a cone to a sphere.
 * @param lhs
 *  the cone
 * @param rhs
 *  the sphere
 * @todo
 *  Document return value.
 */
Ego::Math::Relation cone_intersects_sphere(const Cone3f& lhs, const Sphere3f& rhs);

/**
 * @brief
 *  Find the parametric line where two planes intersect.
 */
bool two_plane_intersection(Vector3f& dst_pos, Vector3f& dst_dir, const Plane3f& p0, const Plane3f& p1);

/**
 * @brief
 *  Find the point where 3 planes intersect.
 */
bool three_plane_intersection(Vector3f& dst_pos, const Plane3f& p0, const Plane3f& p1, const Plane3f& p2);
