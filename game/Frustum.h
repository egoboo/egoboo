// This file is an experimental addition to Egoboo.

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
