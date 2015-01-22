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
	void assign(const sphere_t& other)
	{
		radius = other.radius;
		origin = other.origin;
	}

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
	sphere_t& operator=(const sphere_t& other)
	{
		assign(other);
		return *this;
	}

	sphere_t() :
		origin(0, 0, 0),
		radius(0.0f)
	{

	}
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
