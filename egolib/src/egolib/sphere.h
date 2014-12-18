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
 */
struct s_sphere;
typedef struct s_sphere sphere_t;

struct s_sphere
{
	fvec3_t origin;
	float   radius;
};

#define SPHERE_INIT_VALS                \
    {                                   \
        ZERO_VECT3, /*fvec3_t pos    */ \
        -1.0f       /*float   radius */ \
    }

bool sphere_self_clear(sphere_t *);
