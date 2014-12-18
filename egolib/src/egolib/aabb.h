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

/// @file egolib/aabb.h
/// @brief axis-aligned bounding boxes
#pragma once

#include "egolib/sphere.h"

// Forward declaration.
struct s_oct_bb;

//--------------------------------------------------------------------------------------------

typedef struct aabb_t aabb_t;
#if 0
struct s_aabb;
typedef struct s_aabb aabb_t;
#endif

/// axis aligned bounding box
struct aabb_t
{
	float mins[3];
	float maxs[3];
};

bool aabb_copy(aabb_t * pdst, const aabb_t * psrc);
bool aabb_self_clear(aabb_t * pdst);
bool aabb_is_clear(const aabb_t * pdst);

bool aabb_from_oct_bb(aabb_t * dst, const struct s_oct_bb * src);
bool aabb_lhs_contains_rhs(const aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool aabb_overlap(const aabb_t * lhs_ptr, const aabb_t * rhs_ptr);
bool aabb_self_union(aabb_t * pdst, const aabb_t * psrc);

/** @todo Remove this. aabb_ctor must be declared, defined and used. */
#define AABB_INIT_VALS   { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	An axis-aligned bounding box.
 * @todo
 *	Nope this is not a simple AABB its a bounding volume consisting of a sphere and an AABB.
 *	AABBs are implemented in aabb_t.
 */
struct s_ego_aabb;
typedef struct s_ego_aabb ego_aabb_t;

struct s_ego_aabb
{
	sphere_t sphere;
	aabb_t   data;
};

/* @todo Remove this. ego_aabb_ctor must be used. */
#define EGO_AABB_INIT_VALS                 \
    {                                          \
        SPHERE_INIT_VALS, /*sphere_t sphere */ \
        AABB_INIT_VALS    /*aabb_t   data   */ \
    }

ego_aabb_t *ego_aabb_ctor(ego_aabb_t *);
ego_aabb_t *ego_aabb_dtor(ego_aabb_t *);
bool        ego_aabb_self_clear(ego_aabb_t *);
bool        ego_aabb_is_clear(const ego_aabb_t * pdst);

bool        ego_aabb_self_union(ego_aabb_t * pdst, const ego_aabb_t * psrc);
bool        ego_aabb_lhs_contains_rhs(const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr);
bool        ego_aabb_overlap(const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr);

bool        ego_aabb_copy(ego_aabb_t * pdst, const ego_aabb_t * psrc);
bool        ego_aabb_from_oct_bb(ego_aabb_t * dst, const struct s_oct_bb * src);

bool        ego_aabb_validate(ego_aabb_t * rhs);
bool        ego_aabb_test(const ego_aabb_t * rhs);

//--------------------------------------------------------------------------------------------