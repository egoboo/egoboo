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

/// @file    egolib/sphere.h
/// @brief   spheres
#pragma once

#include "egolib/vec.h"
#if 0
#include "egolib/typedef.h"
#include "egolib/_math.h"
#endif
#if 0
#include "egolib/typedef.h"
#include "egolib/_math.h"
#endif

/**
 * @brief
 *	A sphere.
 *	The terms the/a "sphere_t object" and the/a "sphere" are synonyms.
 */
#if 0
struct s_sphere;
#endif
typedef struct sphere_t sphere_t;

struct sphere_t
{
	fvec3_t origin;
	float   radius;
};

#define SPHERE_INIT_VALS                \
    {                                   \
        ZERO_VECT3, /*fvec3_t pos    */ \
        -1.0f       /*float   radius */ \
    }

/**
 * @brief
 *	Construct a sphere.
 * @param self
 *	a pointer to the uninitialized sphere_t object
 * @post
 *	the sphere_t object pointed by @a self is initialized with a sphere's default values
 * @remark
 *	The default values of a sphere are a radius of @a -1 and a position of @a (0,0,0).
 */
sphere_t *sphere_ctor(sphere_t *self);

/**
 * @brief
 *	Destruct a sphere.
 * @param self
 *	a pointer to the sphere
 */
sphere_t *sphere_dtor(sphere_t *self);

/**
 * @brief
 *	"Clear" a sphere i.e. assign it its default values.
 * @param self
 *	a pointer to the sphere
 */
bool sphere_self_clear(sphere_t *self);

/**
* @brief
*	Get if a sphere is "clear" i.e. has its default values assigned.
* @param self
*	a pointer to the sphere
* @return
*	@a true if the sphere is "clear", @a false otherwise
*/
bool sphere_is_clear(const sphere_t *self);
