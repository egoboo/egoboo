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

/// @file  egolib/bsp_aabb.h
/// @brief BSP AABBs

#pragma once

#include "egolib/math/AABB.h"
#include "egolib/DynamicArray.hpp"

/**
 * @brief
 *	An n-dimensional axis-aligned bounding box.
 */
class BSP_aabb_t
{
public:
	bool valid; ///< @brief @a true if this axis-aligned bounding box is valid, @a false otherwise.
	size_t dim; ///< @brief The dimension of this axis-aligned bounding box.

	Ego::DynamicArray<float> mins; ///< @brief The minimum.
	Ego::DynamicArray<float> mids; ///< @brief The medium.
	Ego::DynamicArray<float> maxs; ///< @brief The maximum.

	/**
	 * @brief
	 *	Construct this BSP AABB.
	 * @param dim
	 *	the dimensionality
	 * @return
	 *	a pointer to this BSP AABB on success, @a NULL on failure
	 */
	BSP_aabb_t(size_t dim);
	/**
	 * @brief
	 *	Destruct this BSP ABB.
	 */
	~BSP_aabb_t();
	/**
	 * @brief
	 *	Get if this bounding box is in the empty state.
	 * @param self
	 *	this bounding box
	 * @return
	 *	@a true if this bounding box in the empty state, @a false otherwise
	 * @remark
	 *	The empty bounding box has a size of @a 0 along all axes and is centered around the origin.
	 */
	static bool is_empty(const BSP_aabb_t *self);
	/**
	 * @brief
	 *	Assign this bounding box to the empty state.
	 * @param self
	 *	this bounding box
	 * @remark
	 *	The empty bounding box has a size of @a 0 along all axes and is centered around the origin.
	 */
	static bool set_empty(BSP_aabb_t *self);
	/**
	 * @brief
	 *	Get if this BSP AABB and and another BSP AABB overlap.
	 * @param self
	 *	this BSP AABB
	 * @param other
	 *	the other BSP AABB
	 * @return
	 *	@a true if this BSP AABB and the other BSP AABB overlap, @a false otherwise
	 * @remark
	 *	If the dimensionality of @a self and @a other are different,
	 *		check the lowest common dimensionality.
	 */
	bool overlap_with_BSP_aabb(const BSP_aabb_t& other) const;
	/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.
	bool contains_BSP_aabb(const BSP_aabb_t& other) const;
	/// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.
	bool overlap_with_aabb(const aabb_t& other) const;
	/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.
	bool contains_aabb(const aabb_t& other) const;
};


BSP_aabb_t *BSP_aabb_alloc(BSP_aabb_t *self, size_t dim); ///< @todo Remove this.
BSP_aabb_t *BSP_aabb_dealloc(BSP_aabb_t *self); ///< @todo Remove this.
bool BSP_aabb_validate(BSP_aabb_t& self); ///< @todo Remove this.

bool BSP_aabb_from_oct_bb(BSP_aabb_t *self, const oct_bb_t *source);
bool BSP_aabb_copy(BSP_aabb_t& dst, const BSP_aabb_t& source);
bool BSP_aabb_self_union(BSP_aabb_t& dst, const BSP_aabb_t& source);
bool BSP_aabb_invalidate(BSP_aabb_t& self);
