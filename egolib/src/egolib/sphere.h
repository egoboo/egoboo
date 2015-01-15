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

/// @file  egolib/sphere.h
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
	fvec3_t origin;
	float   radius;

	/**
	* @brief
	*	Construct a sphere assigning it a sphere's default values.
	* @param self
	*	this sphere
	* @remark
	*	The default values of a sphere are a radius of @a 0 and a position of @a (0,0,0).
	*/
	static sphere_t *ctor(sphere_t& self)
	{
		self.radius = 0.0f;
		fvec3_ctor(self.origin);
		return &self;
	}

	/**
	* @brief
	*	Destruct this sphere.
	* @param self
	*	this sphere
	*/
	static sphere_t *dtor(sphere_t& self)
	{
		fvec3_dtor(self.origin);
		self.radius = 0.0f;
		return &self;
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
