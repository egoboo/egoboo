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

#include "egolib/aabb.h"

struct oct_bb_t;

/**
 * @brief
 *	An axis-aligned bounding box for a binary space partition tree.
 */
struct BSP_aabb_t
{
	bool valid;

	/// The dimensionality of the AABB.
	size_t dim;

	/// An array of @a dim floats, the @a dim dimensional min vector of the AABB.
	float_ary_t mins;
	/// An array of @a dim floats, the @a dim dimensional mid vector of the AABB.
	float_ary_t mids;
	/// An array of @a dim floats, the @a dim dimensional max vector of the AABB.
	float_ary_t maxs;
};

//--------------------------------------------------------------------------------------------




bool BSP_aabb_invalidate(BSP_aabb_t * psrc);
bool BSP_aabb_self_clear(BSP_aabb_t * psrc);

bool BSP_aabb_overlap_with_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);
bool BSP_aabb_contains_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);

bool BSP_aabb_overlap_with_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool BSP_aabb_contains_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);

bool BSP_aabb_empty(const BSP_aabb_t * psrc);
bool BSP_aabb_empty(const BSP_aabb_t * psrc);
bool BSP_aabb_invalidate(BSP_aabb_t * psrc);
bool BSP_aabb_self_clear(BSP_aabb_t * psrc);

bool BSP_aabb_overlap_with_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);
bool BSP_aabb_contains_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);

bool BSP_aabb_overlap_with_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool BSP_aabb_contains_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);



//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @author BB
/// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
///               than lhs_ptr, just check the lowest common dimensions.
bool BSP_aabb_overlap_with_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);


//--------------------------------------------------------------------------------------------
/// @author BB
/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
///               than lhs_ptr, just check the lowest common dimensions.
bool BSP_aabb_contains_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @author BB
/// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
///               than lhs_ptr, just check the lowest common dimensions.
bool BSP_aabb_overlap_with_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);

//--------------------------------------------------------------------------------------------
/// @author BB
/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
///               than lhs_ptr, just check the lowest common dimensions.
bool BSP_aabb_contains_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr);

//--------------------------------------------------------------------------------------------
bool BSP_aabb_empty(const BSP_aabb_t * psrc);

//--------------------------------------------------------------------------------------------
bool BSP_aabb_invalidate(BSP_aabb_t * psrc);

//--------------------------------------------------------------------------------------------
/**
* @brief
*	Set a BSP AABB to an empty state.
* @param self
*	the BSP AABB
*/
bool BSP_aabb_self_clear(BSP_aabb_t * psrc);

bool BSP_aabb_ctor(BSP_aabb_t& self, size_t dim);
void BSP_aabb_dtor(BSP_aabb_t& self);

bool BSP_aabb_alloc(BSP_aabb_t& self, size_t dim);
void BSP_aabb_dealloc(BSP_aabb_t& self);

bool BSP_aabb_from_oct_bb(BSP_aabb_t * pdst, const oct_bb_t * psrc);

bool BSP_aabb_validate(BSP_aabb_t * pbb);
bool BSP_aabb_copy(BSP_aabb_t * pdst, const BSP_aabb_t * psrc);

bool BSP_aabb_self_union(BSP_aabb_t * pdst, const BSP_aabb_t * psrc);
