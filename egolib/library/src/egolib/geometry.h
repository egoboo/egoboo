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
/// @details The frustum code was inspired by Ben Humphrey (DigiBen at <a href="http://www.gametutorials.com">http://www.gametutorials.com</a>), who was inspired by
///          Mark Morely (through the now vanished tutorial at <a href="http://www.markmorley.com/opengl/frustumculling.html">http://www.markmorley.com/opengl/frustumculling.html</a>)

#pragma once

#include "egolib/integrations/math.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Ego::Math {

enum class Relation {
	outside,
	intersect,
	inside,
    on,
};

} // namespace Ego::Math

//--------------------------------------------------------------------------------------------
// intersection routines
//--------------------------------------------------------------------------------------------

/** @internal */
Ego::Math::Relation plane_intersects_aab_min(const Ego::Plane3f& plane, const Ego::Point3f& mins, const Ego::Point3f& maxs);

/** @internal */
Ego::Math::Relation plane_intersects_aab_max(const Ego::Plane3f& plane, const Ego::Point3f& mins, const Ego::Point3f& maxs);

/**
 * @brief
 *	Get if a plane and an axis aligned box intersect.
 * @param plane
 *	the plane
 * @param mins, maxs
 *	the axis aligned box
 * @todo
 *	Rename <tt>plane</tt> to <tt>lhs</tt> and <tt>aabb</tt> to <tt>rhs</tt>.
 */
Ego::Math::Relation plane_intersects_aab(const Ego::Plane3f& plane, const Ego::AxisAlignedBox3f& aab);

/**
 * @brief
 *	Get if a cone and a point intersect.
 * @param lhs
 *	a cone
 * @param rhs
 *	a point
 * @return
 *  - Ego::Math::Relation::On if the point is on the cone
 *  - Ego::Math::Relation::Inside if the point is inside of the cone
 *  - Ego::Math::Relation::Outside otherwise
 * @remark
 *  A definition of the relation of a point and a cone is provided in Ego::Math::Cone3.
 * @see
 *  Ego::Math::Cone3
 */
Ego::Math::Relation cone_intersects_point(const Ego::Cone3f& lhs, const Ego::Vector3f& rhs);

/**
 * @brief
 *  Get the relation of a sphere to an infinite cone.
 * @param lhs
 *  the sphere
 * @param rhs
 *  the cone
 * @return
 *  @a true if the sphere and the cone intersect, @a false otherwise
 * @remark
 *  Let \f$K\f$ be a cone with an origin \f$O\f$, a direction \f$\hat{d}\f$, and an acute angle \f$\theta\f$.
 *  Let \f$S\f$ be a sphere with a center \f$C\f$ and a radius \f$r\f$. We shall determine if the cone and
 *  the sphere intersect.
 *
 *  The backward cone \f$K^-\f$ has
 *  - the origin \f$O^- = O\f$,
 *  - the direction \f$\hat{d}^- = -\hat{d}\f$, and
 *  - the angle \f$\theta^- = 90 - \theta\f$.
 *  The forward cone \f$K^+\f$ has
 *  - the origin \f$O^+ = O - \left(\frac{r}{\sin\theta}\right) \hat{d}\f$,
 *  - the direction \f$\hat{d}^+ = \hat{d}\f$, and
 *  - the angle \f$\theta^+ = \theta\f$.
 *  The condition wether the sphere does or does not intersect the cone are as follows:
 *  - If the sphere does not intersect the forward cone,
 *      then it does not intersect the cone.
 *  - If the sphere intersects the forward cone and does not intersect the backward cone,
 *      then the sphere intersects the cone
 *        if and only if \f$\left|C - V\right| \leq r\f$.
 *  - If the sphere intersects both the forward and the backward cone,
 *       then the sphere intersects the cone.
 *
 * @remark
 *  \image html Cone-Sphere-Intersection-1.svg
 *  illustrates the computation of the forward cone using trigonometry. As
 *  \f{align*}{
 *  \sin\theta=\frac{\mathit{opposite}}{\mathit{hypotenuse}}\\
 *  \hookrightarrow \mathit{hypotenuse}=\frac{\mathit{opposite}}{\sin\theta}\\
 *  \hookrightarrow \mathit{hypotenuse}=\frac{r}{\sin\theta}
 *  \f}
 *
 * @remark
 *  Wether a point is inside, on, or outside a cone, is explained in the point-cone
 *  intersection, however, it is restated here for conveninence: "If \f$\hat{d} \cdot
 *  \left(P-O\right) = \left|P-O\right|\cos \theta\f$ then the point is on the cone
 *  and if \f$\hat{d} \cdot \left(P-O\right) > \left|P-O\right|\cos \theta\f$ then it is
 *  in the cone. Otherwise it is outside the cone."
 */
bool sphere_intersects_cone(const Ego::Sphere3f& lhs, const Ego::Cone3f& rhs);

/**
 * @brief
 *  Find the parametric line where two planes intersect.
 */
bool two_plane_intersection(Ego::Vector3f& dst_pos, Ego::Vector3f& dst_dir, const Ego::Plane3f& p0, const Ego::Plane3f& p1);

/**
 * @brief
 *  Find the point where 3 planes intersect.
 */
bool three_plane_intersection(Ego::Point3f& dst_pos, const Ego::Plane3f& p0, const Ego::Plane3f& p1, const Ego::Plane3f& p2);
