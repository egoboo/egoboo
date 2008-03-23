/* Egoboo - mathstuff.h
 * The name's pretty self explanatory, doncha think?
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

#ifndef _MATHSTUFF_H_
#define _MATHSTUFF_H_


/**> HEADER FILES <**/
//#include "egoboo.h"
#include <math.h>
#include "egobootypedef.h"

#define SQRT_TWO            1.4142135623730950488016887242097f
#define INV_SQRT_TWO        0.70710678118654752440084436210485f
#define PI                  3.1415926535897932384626433832795f
#define TWO_PI              6.283185307179586476925286766559f
#define INV_TWO_PI          0.15915494309189533576888376337251f
#define PI_OVER_TWO         1.5707963267948966192313216916398f
#define PI_OVER_FOUR        0.78539816339744830961566084581988f
#define SHORT_TO_RAD        (TWO_PI / (float)(1<<16))
#define RAD_TO_SHORT        ((float)(1<<16) / TWO_PI)
#define RAD_TO_BYTE         ((float)(1<< 8) / TWO_PI)
#define DEG_TO_RAD          0.017453292519943295769236907684886f
#define RAD_TO_DEG          57.295779513082320876798154814105

#define UINT32_SIZE         (1<<32)
#define UINT32_MAX          (UINT32_SIZE-1)

#define UINT16_SIZE         (1<<16)
#define UINT16_MAX          (UINT16_SIZE-1)

#define UINT8_SIZE          (1<< 8)
#define UINT8_MAX           (UINT8_SIZE-1)


#define RAD_TO_TURN(XX)     ((Uint16)(((XX) + PI) * RAD_TO_SHORT))
#define TURN_TO_RAD(XX)     (((float)(XX))*SHORT_TO_RAD - PI)

#define FP8_TO_FLOAT(XX)   ( (float)(XX)/(float)(1<<8) )
#define FLOAT_TO_FP8(XX)   ( (Uint32)((XX)*(float)(1<<8)) )

#define FP8_TO_INT(XX)     ( (XX) >> 8 )                      // fast version of XX / 256
#define INT_TO_FP8(XX)     ( (XX) << 8 )                      // fast version of XX * 256
#define FP8_MUL(XX, YY)    ( ((XX)*(YY)) >> 8 )
#define FP8_DIV(XX, YY)    ( ((XX)<<8) / (YY) )


#define ABS(X)  (((X) > 0) ? (X) : -(X))

/* Neither Linux nor Mac OS X seem to have MIN and MAX defined, so if they
 * haven't already been found, define them here. */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a):(b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)>(b))? (b):(a) )
#endif

#ifndef SGN
#define SGN(a) ( ((a)>0) ? 1 : -1 )
#endif

#ifndef CLIP
#define CLIP(A, B, C) MIN(MAX(A,B),C)
#endif


/**> MACROS <**/
#define _CNV(i,j) .v[4*i+j]
#define CopyMatrix( pMatrixSource, pMatrixDest ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( GLMatrix ) )

#define INT_TO_BOOL(XX) (0!=(XX))

/**> DATA STRUCTURES <**/
#pragma pack(push,1)
typedef struct matrix_4x4_t { float v[16]; } matrix_4x4;
typedef union vector2_t { float _v[2]; struct { float x, y; }; struct { float u, v; }; struct { float s, t; }; } vect2;
typedef union vector3_t { float v[3]; struct { float x, y, z; }; struct { float r, g, b; }; } vect3;
typedef union vector4_t { float v[4]; struct { float x, y, z, w; }; struct { float r, g, b, a; }; } vect4;
#pragma pack(pop)

typedef matrix_4x4 GLMatrix;
typedef vect4 GLVector;


/**> GLOBAL VARIABLES <**/
#define TRIGTABLE_SIZE (1<<14)
#define TRIGTABLE_MASK (TRIGTABLE_SIZE-1)
#define TRIGTABLE_SHIFT (TRIGTABLE_SIZE>>2)       // TRIGTABLE_SIZE/4 == TWO_PI/4 == PI_OVER_2
extern float turntosin[TRIGTABLE_SIZE];           // Convert chrturn>>2...  to sine


/**> FUNCTION PROTOTYPES <**/
vect3 VSub( vect3 A, vect3 B );
vect3 Normalize( vect3 vec );
vect3 CrossProduct( vect3 A, vect3 B );
float DotProduct( vect3 A, vect3 B );

GLVector VSubGL( GLVector A, GLVector B );
GLVector NormalizeGL( GLVector vec );
GLVector CrossProductGL( GLVector A, GLVector B );
float DotProductGL( GLVector A, GLVector B );


GLMatrix IdentityMatrix( void );
GLMatrix ZeroMatrix( void );
GLMatrix MatrixTranspose( const GLMatrix a );
GLMatrix MatrixMult( const GLMatrix a, const GLMatrix b );
GLMatrix Translate( const float dx, const float dy, const float dz );
GLMatrix RotateX( const float rads );
GLMatrix RotateY( const float rads );
GLMatrix RotateZ( const float rads );
GLMatrix ScaleXYZ( const float sizex, const float sizey, const float sizez );
GLMatrix ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz );
GLMatrix FourPoints( GLVector ori, GLVector wid, GLVector forw, GLVector up, float scale );
GLMatrix ViewMatrix( const vect3 from, const vect3 at, const vect3 world_up, const float roll );
GLMatrix ProjectionMatrix( const float near_plane, const float far_plane, const float fov );

void Transform4_Full( GLMatrix *pMatrix, GLVector pSourceV[], GLVector pDestV[], Uint32 NumVertor );
void Transform4( GLMatrix *pMatrix, GLVector pSourceV[], GLVector pDestV[], Uint32 NumVertor );

void Transform3_Full( GLMatrix *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor );
void Transform3( GLMatrix *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor );


Uint16 vec_to_turn( float dx, float dy );
void turn_to_vec( Uint16 turn, float * dx, float * dy );

void VectorClear( float v[] );

#endif

