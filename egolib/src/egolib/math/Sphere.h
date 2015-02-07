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

/// @file  egolib/math/Sphere.h
/// @brief Spheres.

#pragma once

#include "egolib/vec.h"

/**
* @brief
*	A sphere.
*	The terms the/a "sphere_t object" and the/a "sphere" are synonyms.
*/
struct sphere_t
{
	/**
	 * @brief
	 *	The center of the sphere.
	 * @todo
	 *	Rename to @a center.
	 */
	fvec3_t origin;
	/**
	 * @brief
	 *	The radius of the sphere.
	 * @invariant
	 *	Greater than or equal to @a 0.
	 */
	float radius;

	/**
	 * @brief
	 *	Get the center of this sphere.
	 * @return
	 *	the center of this sphere
	 */
	const fvec3_t& getCenter() const;

	/**
	 * @brief
	 *	Get the radius of this  sphere.
	 * @return
	 *	the radius of this sphere
	 */
	float getRadius() const;

	/**
	 * @brief
	 *	Construct this sphere assigning it the default values of a sphere.
	 * @return
	 *	a pointer to this sphere on success, @a nullptr on failure
	 * @post
	 *	This sphere was assigned the default values of a sphere.
	 * @remark
	 *	The default values of a sphere are the center of @a (0,0,0) and the radius of @a 0.
	 */
	sphere_t *ctor()
	{
		radius = 0.0f;
		fvec3_ctor(origin);
		return this;
	}

	/**
	 * @brief
	 *	Destruct this sphere.
	 */
	sphere_t *dtor()
	{
		fvec3_dtor(origin);
		radius = 0.0f;
		return this;
	}

	/**
	 * @brief
	 *	Assign this sphere values of another sphere.
	 * @param other
	 *	the other sphere
	 * @post
	 * 	This sphere was assigned the values of the sphere.
	 */
	void assign(const sphere_t& other);

	/**
	 * @brief
	 *	Assign this sphere the values of another sphere.
	 * @param other
	 *	the other sphere
	 * @return
	 *	this sphere
	 * @post
	 *	This sphere was assigned the values of the other sphere.
	 */
	sphere_t& operator=(const sphere_t& other);

	/**
	 * @brief
	 *	Construct this sphere assigning it the default values of a sphere.
	 * @post
	 *	This sphere was assigned the default values of a sphere.
	 * @remark
	 *	The default values of a sphere are the center of @a (0,0,0) and the radius of @a 0.
	 */
	sphere_t();

	/**
	 * @brief
	 *	Construct this sphere assigning it the values of another sphere.
	 * @post
	 *	This sphere was assigned the default values of another sphere.
	 */
	sphere_t(const sphere_t& other);

	/**
	 * @brief
	 *	Get if this sphere intersects with a point.
	 * @param other
	 *	the point
	 * @return
	 *	@a true if this sphere intersects with the point,
	 *	@a false otherwise
	 * @remark
	 *	A sphere \f$(c,r)\f$ with the center $c$ and the radius $r$
	 *	and a point \f$p\f$ intersect if \f$|p - c| \leq r\f$ holds.
	 *	That condition is equivalent to the condition \f$|p - c|^2
	 *	\leq r^2\f$ but the latter is more efficient to test (two
	 *	multiplications vs. one square root).
	 */
	bool intersects(const fvec3_t& point) const;

	/**
	 * @brief
	 *	Get if this sphere intersects with another sphere.
	 * @param other
	 *	the other sphere
	 * @return
	 *	@a true if this sphere intersects with the other sphere,
	 *	@a false otherwise
	 * @remark
	 *	Two spheres \f$(c_0,r_0)\f$ and \f$(c_1,r_1)\f$ with the
	 *	centers \f$c_0\f$ and \f$c_1\f$ and the radii \f$r_0\f$
	 *	and \f$r_1\f$ intersect if \f$|c_1 - c_0| \leq r_0 + r_1\f$
	 *	holds. That condition is equivalent to the condition
	 *	\f$|c_1 - c_0|^2 \leq (r_0 + r_1)^2\f$ but the latter
	 *	is more efficient to test (two multiplications vs. one
	 *	square root).
	 */
	bool intersects(const sphere_t& other) const;

};

/**
 * @brief
 *	Assign this sphere the default values of a sphere.
 * @param self
 *	this sphere
 */
bool sphere_self_clear(sphere_t& self);

/**
 * @brief
 *	Get if a sphere is "clear" i.e. has its default values assigned.
 * @param self
 *	a pointer to the sphere
 * @return
 *	@a true if the sphere is "clear", @a false otherwise
 */
bool sphere_is_clear(const sphere_t *self);
