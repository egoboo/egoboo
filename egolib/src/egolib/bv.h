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

/// @file egolib/bv.h
/// @brief Convex bounding volumes consiting of spheres enclosing boxes.

#pragma once

#include "egolib/sphere.h"
#include "egolib/aabb.h"

// Forward declaration.
struct oct_bb_t;

/**
 * @brief
 *	A convex bounding volume consisting of a sphere enclosing a bounding box.
 */
struct bv_t
{
	sphere_t sphere;
	aabb_t aabb;
	/**
	 * @brief
	 *	Construct this convex bounding volume assigning the default values of a convex bounding volume.
	 * @return
	 *	a pointer to this convex bounding volume on success, @a nullptr on failure
	 * @post
	 *	This convex bounding volume was assigned the default values of a convex bounding volume.
	 * @remark
	 *	The default values of a convex bounding volume are the default values of an axis-aligned bounding box and the smallest sphere enclosing that AABB.
	 */
	bv_t *ctor()
	{
		sphere.ctor();
		aabb.ctor();
		return this;
	}
	/**
	 * @brief
	 *	Destruct this convex bounding volume.
	 */
	void dtor()
	{
		aabb.dtor();
		sphere.dtor();
	}
	/**
	 * @brief
	 *	Assign this convex bounding volume the values of another convex bounding volume.
	 * @param other
	 *	the other convex bounding volume
	 * @post
	 *	This convex bounding volume box was assigned the values of the other convex bounding box.
	 */
	void assign(const bv_t& other)
	{
		aabb = other.aabb;
		sphere = other.sphere;
	}
	/**
	 * @brief
	 *	Assign this convex bounding volume the values of another convex bounding volume.
	 * @param other
	 *	the other convex bounding volume
	 * @return
	 *	this convex bounding volume
	 * @post
	 *	This convex bounding volume box was assigned the values of the other convex bounding box.
	 */
	bv_t& operator=(const bv_t& other)
	{
		assign(other);
		return *this;
	}
};


bool  bv_self_clear(bv_t *);
bool  bv_is_clear(const bv_t * pdst);

bool  bv_self_union(bv_t * pdst, const bv_t * psrc);
bool  bv_lhs_contains_rhs(const bv_t * lhs_ptr, const bv_t * rhs_ptr);
bool  bv_overlap(const bv_t * lhs_ptr, const bv_t * rhs_ptr);

bool  bv_from_oct_bb(bv_t * dst, const oct_bb_t * src);

bool  bv_validate(bv_t * rhs);
bool  bv_test(const bv_t * rhs);
