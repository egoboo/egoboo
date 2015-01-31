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

/// @file  egolib/math/Plane.h
/// @brief Planes.

#pragma once

#include "egolib/vec.h"

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
 */
struct plane_t
{
public:

	/**
	 * @brief
	 *	Default constructor
	 */
	plane_t() :
		_n(0.0f, 0.0f, 0.0f),
		_d(0.0f)
	{
		//ctor
	}

	/**
	 * @brief
	 *	Create a plane from three non-collinear points.
	 * @param a,b,c
	 *	the point
	 * @throw std::domain_error
	 *	if the points are collinear
	 * @remark
	 *	Assume \f$a\f$, \f$b\f$ and \f$c\f$ are not collinear. Let \f$u = b - a\f$,
	 *	\f$v = c - a\f$, \f$n = u \times v\f$ \f$\hat{n} = \frac{n}{|n|}\f$ and
	 *	\f$d = -(\hat{n} \cdot a)\f$.
	 * @remark
	 *	We show that \f$a\f$, \f$b\f$ and \f$c\f$ are on the plane given by the
	 *	equation \f$\hat{n} \cdot p + d = 0\f$. To show this for \f$a\f$, rewrite
	 *	the plane equation to
	 *	\f[
	 *	\hat{n} \cdot p  + -(\hat{n} \cdot a) = 0\\
	 *	\f]
	 *	shows for \f$p=a\f$ immediatly that \f$a\f$ is on the plane.
	 * @remark
	 *	To show that \f$b\f$ and \f$c\f$ are on the plane, the plane equation is
	 *	rewritten yet another time:
	 *	\f[
	 *	\hat{n} \cdot p + -(\hat{n} \cdot a) = 0      &| \text{Definition} d\\
	 *	\hat{n} \cdot p + \hat{n} \cdot -a = 0        &| \text{Bilinearity of the dot product}\\
	 *	\hat{n} \cdot (p - a) = 0                     &| \text{Bilinearity of the dot product}\\
	 *	\frac{n}{|n|} \cdot (p - a) = 0               &| \text{Definition} \hat{n}\\
	 *	\frac{u \times v}{|u \times v|} \cdot (p - a) &| \text{Definition} n\\
	 *	\f]
	 *	Let \f$p = b\f$ then
	 *	\f[
	 *	\frac{u \times v}{|u \times v|}  \cdot (b - a) = 0   &\\
	 *	\frac{u \times v}{|u \times v|}  \cdot u = 0         &|\text{Definition} u\\
	 *	\f]
	 *	shows that \f$b\f$ is on the plane. The proof for \f$c\f$ is left to the reader.
	 */
	plane_t(const fvec3_t& a, const fvec3_t& b, const fvec3_t& c) : plane_t()
	{
		fvec3_t u = b - a;
		if (u == fvec3_t::zero)
		{
			throw std::domain_error("b = a");
		}
		fvec3_t v = c - a;
		if (u == fvec3_t::zero)
		{
			throw std::domain_error("c = a");
		}
		_n = u.cross(v);
		if (0.0f == _n.normalize())
		{
			/* u x v = 0 is only possible for u,v != 0 if u = v and thus b = c. */
			throw std::domain_error("b = c");
		}
		_d = - _n.dot(a);
	}

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
	 *	Let \f$\hat{n} = \frac{n}{|n|}$, \f$l=|p|\f$ and \f$d = -(\hat{n} \cdot p)\f$.
	 *	then \f$p\f$ is on the plane as
	 *	\f[
	 *	\hat{n} \cdot p + d = 0\\
	 *	\Rightarrow \hat{n} \cdot p -(\hat{n} \cdot p) = 0
	 *	\Rightarrow 0 = 0
	 *	\f]
	 */
	plane_t(const fvec3_t& p, const fvec3_t& n) :
		_n(n),
		_d(0.0f)
	{
		if (_n.normalize() == 0.0f)
		{
			throw std::domain_error("normal vector is zero vector");
		}
		_d = -_n.dot(p);
	}

	/**
	 * @brief
	 *	Create a plane from another plane (copy constructor).
	 * @param other
	 *	the other plane
	 * @post
	 *	This plane was assigned the values the other plane.
	 */
	plane_t(const plane_t& other) :
		_n(other._n),
		_d(other._d)
	{
	}

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
