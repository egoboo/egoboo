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
/// @file egolib/frustum.h
/// @brief integrating the basic frustum object into Egoboo algorithms
#pragma once

#include "egolib/geometry.h"
#include "egolib/bbox.h"
#include "egolib/aabb.h"
#include "egolib/bv.h"

#if 0
#if defined(__cplusplus)
extern "C"
{
#endif
#endif

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

    struct s_oct_bb;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------


#if 0
    struct s_egolib_frustum;
#endif
    typedef struct egolib_frustum_t egolib_frustum_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------
    struct egolib_frustum_t
    {
        // basic frustum data
        frustum_base_t data;

        // data for intersection optimization
        fvec3_t   origin;
        sphere_t  sphere;
        cone_t    cone;
    };

/// Call this every time the camera moves to update the frustum
    egolib_rv egolib_frustum_calculate( egolib_frustum_t * pfrust, const float proj[], const float modl[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Call this every time the camera moves to update the frustum
    geometry_rv egolib_frustum_intersects_bv( const egolib_frustum_t * pfrust, const bv_t * paabb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if 0
#if defined(__cplusplus)
}
#endif
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if 0
#define _egolib_frustum_h
#endif