/* Egoboo - Frustum.h
 * 
 * 
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "egoboo_types.h"

typedef float frustum_data_t[6][4];

typedef struct frustum_t
{
  // This holds the A B C and D values for each side of our frustum.
  frustum_data_t planes;
} Frustum;

extern Frustum gFrustum;

// Call this every time the camera moves to update the frustum
void Frustum_CalculateFrustum( Frustum * pf, float proj[], float modl[] );

// This takes a 3D point and returns TRUE if it's inside of the frustum
bool_t Frustum_PointInFrustum( Frustum * pf, float pos[] );

// This takes a 3D point and a radius and returns TRUE if the sphere is inside of the frustum
bool_t Frustum_SphereInFrustum( Frustum * pf, float pos[], float radius );

// This takes the center and half the length of the cube.
bool_t Frustum_CubeInFrustum( Frustum * pf, float pos[], float size );

// This takes two vectors forming the corners of an axis-aligned bounding box
bool_t Frustum_BBoxInFrustum( Frustum * pf, float corner1[], float corner2[] );
