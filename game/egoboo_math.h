#pragma once

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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - mathstuff.h
 * The name's pretty self explanatory, doncha think?
 */

//*include "egoboo.h"
#include <math.h>
#include "egoboo_typedef.h"

#define PI                  3.1415926535897932384626433832795f
#define TWO_PI              6.283185307179586476925286766559f
#define INV_FF              0.003921568627450980392156862745098f

#define TRIG_TABLE_BITS   14
#define TRIG_TABLE_SIZE   (1<<TRIG_TABLE_BITS)
#define TRIG_TABLE_MASK   (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_OFFSET (TRIG_TABLE_SIZE>>2)

extern float turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine
extern float turntocos[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to cosine

// Just define ABS, MIN, and MAX using macros for the moment. This is likely to be the
// fastest and most cross-platform solution
#ifndef ABS
#    define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif

#ifndef MIN
#    define MIN(x, y)  (((x) > (y)) ? (y) : (x))
#endif

#ifndef MAX
#    define MAX(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef SQRT
#    define SQRT(A) ((float)sqrt((float)(A)))
#endif

#ifndef SIN
#    define SIN(A) ((float)sin((float)(A)))
#endif

#ifndef COS
#    define COS(A) ((float)cos((float)(A)))
#endif

#ifndef POW
#    define POW(A, B) ((float)pow((float)(A), (float)(B)))
#endif

#ifndef ATAN2
#    define ATAN2(A, B) ((float)atan2((float)(A), (float)(B)))
#endif

#ifndef CLIP
#    define CLIP(VAL,VMIN,VMAX) MIN(MAX(VAL,VMIN),VMAX)
#endif

#ifndef SWAP
#    define SWAP(TYPE, A, B) { TYPE temp; temp = A; A = B; B = temp; }
#endif

#define CNV(i,j) v[4*i+j]
#define CopyMatrix( pMatrixDest, pMatrixSource ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( glMatrix ) )

typedef struct glmatrix { float v[16]; } glMatrix;
typedef struct glvector { float x, y, z, w; } glVector;

/**> GLOBAL VARIABLES <**/
extern float                   turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine

/**> FUNCTION PROTOTYPES <**/
glVector vsub( glVector A, glVector B );
glVector Normalize( glVector vec );
glVector CrossProduct( glVector A, glVector B );
float DotProduct( glVector A, glVector B );
glMatrix IdentityMatrix( void );
glMatrix ZeroMatrix( void );
glMatrix MatrixMult( const glMatrix a, const glMatrix b );
glMatrix Translate( const float dx, const float dy, const float dz );
glMatrix RotateX( const float rads );
glMatrix RotateY( const float rads );
glMatrix RotateZ( const float rads );
glMatrix ScaleXYZ( const float sizex, const float sizey, const float sizez );
glMatrix ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz );
glMatrix FourPoints( float orix, float oriy, float oriz, float widx, float widy, float widz, float forx, float fory, float forz, float upx,  float upy,  float upz, float scale );
glMatrix ViewMatrix( const glVector from, const glVector at, const glVector world_up, const float roll );
glMatrix ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
void  TransformVertices( glMatrix *pMatrix, glVector *pSourceV, glVector *pDestV, Uint32  pNumVertor );

void make_turntosin( void );
