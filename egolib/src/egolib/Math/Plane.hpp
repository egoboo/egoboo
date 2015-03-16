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

/// @file  egolib/Math/Plane.hpp
/// @brief Planes.

#pragma once

#include "egolib/Math/Vector.hpp"

// The base type of the plane data.
// @todo Remove this.
typedef fvec4_base_t plane_base_t;

/// @todo Remove this.
bool plane_base_normalize(plane_base_t *self);

/**
 * @brief
 *	A plane in the normal-distance form defined as
 *	\f[
 *	\hat{n} \cdot P + d = 0
 *	\f]
 *	where \f$\hat{n}\f$ is the unit plane normal and \f$d\f$ is the distance of the plane from the origin.
 *	Any point \f$P\f$ for which the above equation is fulfilled is on the plane.
 * @author
 *  Michael Heilmann
 */
struct plane_t
{
public:

	/**
	 * @brief
	 *	Default constructor.
	 * @remark
	 *	The default plane has the plane normal @a (0,0,1) and a distance from the origin of @a 0.
	 */
	plane_t();

	/**
	 * @brief
	 *	Create a plane from three non-collinear points.
	 * @param a,b,c
	 *	the point
	 * @throw std::domain_error
	 *	if the points are collinear
	 * @remark
	 *	Assume \f$a\f$, \f$b\f$ and \f$c\f$ are not collinear. Let \f$u = b - a\f$,
	 *	\f$v = c - a\f$, \f$n = u \times v\f$, \f$\hat{n} = \frac{n}{|n|}\f$ and
	 *	\f$d = -\left(\hat{n} \cdot a\right)\f$.
	 * @remark
	 *	We show that \f$a\f$, \f$b\f$ and \f$c\f$ are on the plane given by the
	 *	equation \f$\hat{n} \cdot p + d = 0\f$. To show this for \f$a\f$, rewrite
	 *	the plane equation to
	 *	\f{eqnarray*}{
	 *	\hat{n} \cdot p  + -(\hat{n} \cdot a) = 0\\
	 *	\f}
	 *	shows for \f$p=a\f$ immediatly that \f$a\f$ is on the plane.
	 * @remark
	 *	To show that \f$b\f$ and \f$c\f$ are on the plane, the plane equation is
	 *	rewritten yet another time using the bilinearity property of the dot product
     *  and the definitions of \f$d\f$ and \f$\hat{n}\f$ its
	 *	\f{align*}{
	 *	 &\hat{n} \cdot p + -(\hat{n} \cdot a)  \;\;\text{Def. of } d\\
	 *	=&\hat{n} \cdot p + \hat{n} \cdot (-a)  \;\;\text{Bilinearity of the dot product}\\
	 *	=&\hat{n} \cdot (p - a)                 \;\;\text{Bilinearity of the dot product}\\
	 *	=&\frac{n}{|n|} \cdot (p - a)           \;\;\text{Def. of } \hat{n}\\
     *	=&(\frac{1}{|n|}n) \cdot (p - a)        \;\;\\
     *	=&\frac{1}{|n|}(n \cdot (p - a))       \;\;\text{Compatibility of the dot product w. scalar multiplication}\\
     *  =&n \cdot (p - a)                       \;\;\\
	 *  =&(u \times v) \cdot (p - a)            \;\;\text{Def. of } n  
     *	\f}
	 *	Let \f$p = b\f$ then
	 *	\f{align*}{
     *   &(u \times v) \cdot (b - a)            \;\text{Def. of } u\\
     *  =&(u \times v) \cdot u\\
     *  =&0
     *  \f}
     *  or let \f$p = c\f$ then
     *  \f{align*}{
     *   &(u \times v) \cdot (c - a)              \;\text{Def. of } u\\
     *  =&(u \times v) \cdot v\\
     *  =&0
     *  \f}
     *  as \f$u \times v\f$ is orthogonal to both \f$u\f$ and \f$v\f$.
     *  This shows that \f$b\f$ and \f$c\f$ are on the plane.
     */
	plane_t(const fvec3_t& a, const fvec3_t& b, const fvec3_t& c);

	/**
	 * @brief
	 *	Create a plane from a point and a normal.
	 * @param p
	 *	the point
	 * @param n
	 *	the normal
	 * @throw std::domain_error
	 *	if the normal vector is the zero vector
	 * @remark
	 *	The plane normal is normalized if necessary. 
	 * @remark
     *  Let \f$v\f$ be the point and \f$n\f$ the normal, then the plane equation is given by
     *  \f{align*}{
     *  \hat{n} \cdot p + d = 0, \hat{n}=\frac{n}{|n|}, d = d = -(\hat{n} \cdot v)
     *  \f}
     *  \f$v\f$ is on the plane as
     *  \f{align*}{
     *   &\hat{n} \cdot v + d\\
     *  =&\hat{n} \cdot v + -(\hat{n} \cdot v)\\
     *  =&\hat{n} \cdot v - \hat{n} \cdot v\\
     *  =&0
     *  \f}
	 */
	plane_t(const fvec3_t& p, const fvec3_t& n);

	/**
	 * @brief
	 *	Create a plane from another plane (copy constructor).
	 * @param other
	 *	the other plane
	 * @post
	 *	This plane was assigned the values the other plane.
	 */
	plane_t(const plane_t& other);

	/**
	 * @brief
	 *	Get the distance of a point from this plane.
	 * @param point
	 *	the point
	 * @return
	 *	the distance of the point from the plane.
	 *	The point is in the positive (negative) half-space of the plane if the distance is positive (negative).
	 *	Otherwise the point is on the plane.
     * @remark
     *  Let \f$\hat{n} \cdot P + d = 0\f$ be a plane and \f$v\f$ be some point.
     *  We claim that \f$d'=\hat{n} \cdot v + d\f$ is the signed distance of the
     *  point \f$v\f$ from the plane.
     *  
     *  To show this, assume \f$v\f$ is not in the plane. Then there
     *  exists a single point \f$u\f$ on the plane which is closest to
     *  \f$v\f$ such that \f$v\f$ can be expressed by translating \f$u\f$
     *  along the plane normal by the signed distance \f$d'\f$ from
     *  \f$u\f$ to \f$v\f$ i.e. \f$v = u + d' \hat{n}\f$. Obviously,
     *  if \f$v\f$ is in the positive (negative) half-space of the plane, then
     *  \f$d'>0\f$ (\f$d' < 0\f$). We obtain now
     *	\f{align*}{
     *	                &\hat{n} \cdot v + d\\
     *	              = &\hat{n} \cdot (u + d' \hat{n}) + d\\
     *	              = &\hat{n} \cdot u + d' (\hat{n} \cdot \hat{n}) + d\\
     *	              = &\hat{n} \cdot u + d' + d\\
     *	              = &\hat{n} \cdot u + d + d'\\
     *	\f}
     *  However, as \f$u\f$ is on the plane
     *	\f{align*}{
     *	              = &\hat{n} \cdot u + d + d'\\
     *                = &d'
     *	\f}
	 */
	float distance(const fvec3_t& point) const;


private:
	/**
	 * @brief
	 *	The plane normal.
	 * @invariant
	 *	The plane normal is a unit vector.
	 */
	fvec3_t _n;
	/**
	 * @brief
	 *	The distance of the plane from the origin.
	 */
	float _d;
};
