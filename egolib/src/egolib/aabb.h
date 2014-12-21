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
typedef struct oct_bb_t oct_bb_t;

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	An axis-aligned bounding box ("AABB").
 * @remark
 *	The terms "the/an axis-aligned bounding box (object)" and "the/an AABB (object)" are synonyms.
 */
typedef struct aabb_t aabb_t;

struct aabb_t
{
	float mins[3];
	float maxs[3];
};

aabb_t *aabb_ctor(aabb_t *self);
aabb_t *aabb_dtor(aabb_t *self);
bool aabb_copy(aabb_t * pdst, const aabb_t * psrc);
bool aabb_self_clear(aabb_t * pdst);
bool aabb_is_clear(const aabb_t * pdst);

bool aabb_from_oct_bb(aabb_t * dst, const struct oct_bb_t * src);
bool aabb_lhs_contains_rhs(const aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool aabb_overlap(const aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool aabb_self_union(aabb_t * pdst, const aabb_t * psrc);

/** @todo Remove this. aabb_ctor must be declared, defined and used. */
#define AABB_INIT_VALS   { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }

//--------------------------------------------------------------------------------------------