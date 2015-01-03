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

/// @file  egolib/aabb.h
/// @brief Axis-aligned bounding boxes.

#pragma once

#include "egolib/sphere.h"

// Forward declaration.
struct oct_bb_t;

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	An axis-aligned bounding box ("AABB").
 * @remark
 *	The terms "the/an axis-aligned bounding box (object)" and "the/an AABB (object)" are synonyms.
 */
#if 0
typedef struct aabb_t aabb_t;
#endif
struct aabb_t
{
	float mins[3]; /**< @todo Use fvec3_t. */
	float maxs[3]; /**< @todo Use fvec3_t. */
};

aabb_t *aabb_ctor(aabb_t *self);
aabb_t *aabb_dtor(aabb_t *self);
bool aabb_copy(aabb_t * pdst, const aabb_t * psrc);
bool aabb_self_clear(aabb_t * pdst);
bool aabb_is_clear(const aabb_t * pdst);

bool aabb_from_oct_bb(aabb_t * dst, const struct oct_bb_t * src);
/**
 * @brief
 *	Does this AABB contain another AABB.
 * @param self
 *	this AABB
 * @param other
 *	the other AABB
 * @return
 *	@a true if the other AABB is contained in this AABB
 */
bool aabb_lhs_contains_rhs(const aabb_t *self, const aabb_t *other);

/**
 * @brief
 *	Do this AABB and another AABB overlap.
 * @param self
 *	this AABB
 * @param other
 *	the other AABB
 * @return
 *	@a true if this AABB and the other AABB overlap
 */
bool aabb_overlap(const aabb_t *self, const aabb_t *other);
bool aabb_self_union(aabb_t * pdst, const aabb_t * psrc);

#if 0
/** @todo Remove this. aabb_ctor must be declared, defined and used. */
#define AABB_INIT_VALS   { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }
#endif

//--------------------------------------------------------------------------------------------