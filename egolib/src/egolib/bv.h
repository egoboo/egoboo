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
	aabb_t data;
};

#if 0
/* @todo Remove this. bv_ctor must be used. */
#define BV_INIT_VALS                            \
    {                                           \
        SPHERE_INIT_VALS, /* sphere_t sphere */ \
        AABB_INIT_VALS    /* aabb_t aabb   */   \
    }
#endif

bv_t *bv_ctor(bv_t *);
bv_t *bv_dtor(bv_t *);
bool  bv_self_clear(bv_t *);
bool  bv_is_clear(const bv_t * pdst);

bool  bv_self_union(bv_t * pdst, const bv_t * psrc);
bool  bv_lhs_contains_rhs(const bv_t * lhs_ptr, const bv_t * rhs_ptr);
bool  bv_overlap(const bv_t * lhs_ptr, const bv_t * rhs_ptr);

bool  bv_copy(bv_t * pdst, const bv_t * psrc);
bool  bv_from_oct_bb(bv_t * dst, const oct_bb_t * src);

bool  bv_validate(bv_t * rhs);
bool  bv_test(const bv_t * rhs);
