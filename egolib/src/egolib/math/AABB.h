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

/// @file  egolib/math/AABB.h
/// @brief Axis-aligned bounding boxes.

#pragma once

#include "egolib/math/Sphere.h"

// Forward declaration.
struct oct_bb_t;

/**
 * @brief
 *	An axis-aligned bounding box ("AABB").
 * @remark
 *	The terms "the/an axis-aligned bounding box (object)" and "the/an AABB (object)" are synonyms.
 */
struct aabb_t
{
	/// @brief The minimum of the bounding box.
	fvec3_t mins;
	/// @brief The maximum of the bounding box.
	fvec3_t maxs;

	/**
	 * @brief
	 *	Construct this bounding box assigning it the default values of a bounding box.
	 * @return
	 *	a pointer to this bounding box on success, @a nullptr on failure
	 * @post
	 *	This bounding box was assigned the default values of a bounding box.
	 * @remark
	 *	The default values of a bounding box are the center of @a (0,0,0) and the size of @a 0 along all axes
	 */
	aabb_t *ctor()
	{
		for (size_t i = 0; i < 3; ++i)
		{
			mins[i] = maxs[i] = 0.0f;
		}
		return this;
	}
	/**
	 * @brief
	 *	Destruct this bounding box.
	 */
	void dtor()
	{
		for (size_t i = 0; i < 3; ++i)
		{
			mins[i] = maxs[i] = 0.0f;
		}
	}
	/**
	 * @brief
	 *	Assign this bounding box the values of another bounding box.
	 * @param other
	 *	the other bounding box
	 * @post
	 *	This bounding box was assigned the values of the other bounding box.
	 */
	void assign(const aabb_t& other)
	{
		for (size_t cnt = 0; cnt < 3; cnt++)
		{
			mins[cnt] = other.mins[cnt];
			maxs[cnt] = other.maxs[cnt];
		}
	}
	/**
	 * @brief
	 *	Assign this bounding box the values of another bounding box.
	 * @param other
	 *	the other bounding box
	 * @return
	 *	this bounding box
	 * @post
	 *	This bounding box was assigned the values of the other bounding box.
	 */
	aabb_t& operator=(const aabb_t& other)
	{
		assign(other);
		return *this;
	}
};

/** @todo Remove this. */
bool aabb_self_clear(aabb_t *dst);

/** @todo Remove this. */
bool aabb_is_clear(const aabb_t *dst);

/** @todo Add documentation. */
bool aabb_from_oct_bb(aabb_t *self, const oct_bb_t *src);

/**
 * @brief
 *	Get if this AABB contains another AABB.
 * @param self
 *	this AABB
 * @param other
 *	the other AABB
 * @return
 *	@a true if the other AABB is contained in this AABB
 */
bool aabb_lhs_contains_rhs(const aabb_t& self, const aabb_t& other);

/**
 * @brief
 *	Get if this AABB and another AABB overlap.
 * @param self
 *	this AABB
 * @param other
 *	the other AABB
 * @return
 *	@a true if this AABB and the other AABB overlap
 */
bool aabb_overlap(const aabb_t& self, const aabb_t& other);

/**
 * @brief
 *	Compute the union of this AABB and another AABB.
 * @param self
 *	this AABB
 * @param other
 *	the other AABB
 * @return
 *	@a true if this AABB and the other AABB overlap
 * @post
 *	The union of this AABB and the other AABB was assigned to this AABB.
 */
void aabb_self_union(aabb_t& self, const aabb_t& other);
