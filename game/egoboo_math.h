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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
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
#define SQRT_TWO            1.4142135623730950488016887242097f

#define INV_FF              0.003921568627450980392156862745098f
#define INV_0100            0.00390625f
#define INV_FFFF            0.000015259021896696421759365224689097f

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
#define CopyMatrix( pMatrixDest, pMatrixSource ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( GLmatrix ) )

typedef float glmatrix_base_t[16];
typedef float glvector2_base_t[2];
typedef float glvector3_base_t[3];
typedef float glvector4_base_t[4];

typedef struct s_glmatrix  { glmatrix_base_t  v; } GLmatrix;
typedef union  u_glvector2 { glvector2_base_t v; struct { float x, y; }; } GLvector2;
typedef union  u_glvector3 { glvector3_base_t v; struct { float x, y, z; }; struct { float r, g, b; }; } GLvector3;
typedef union  u_glvector4 { glvector4_base_t v; struct { float x, y, z, w; }; struct { float r, g, b, a; }; } GLvector4;

#define ZERO_VECT2 { {0,0} }
#define ZERO_VECT3 { {0,0,0} }
#define ZERO_VECT4 { {0,0,0,0} }

#define VECT2(XX,YY) { {XX,YY} }
#define VECT3(XX,YY,ZZ) { {XX,YY,ZZ} }
#define VECT4(XX,YY,ZZ,WW) { {XX,YY,ZZ,WW} }

/**> GLOBAL VARIABLES <**/
extern float                   turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( isnan(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n\t(\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

/**> FUNCTION PROTOTYPES <**/
GLvector3 VSub( GLvector3 A, GLvector3 B );
GLvector3 VNormalize( GLvector3 vec );
GLvector3 VCrossProduct( GLvector3 A, GLvector3 B );
float     VDotProduct( GLvector3 A, GLvector3 B );
GLmatrix  IdentityMatrix( void );
GLmatrix  ZeroMatrix( void );
GLmatrix  MatrixMult( const GLmatrix a, const GLmatrix b );
GLmatrix  Translate( const float dx, const float dy, const float dz );
GLmatrix  RotateX( const float rads );
GLmatrix  RotateY( const float rads );
GLmatrix  RotateZ( const float rads );
GLmatrix  ScaleXYZ( const float sizex, const float sizey, const float sizez );
GLmatrix  ScaleXYZRotateXYZTranslate(  const float sizex, const float sizey, const float sizez, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float tx, const float ty, const float tz  );
GLmatrix  FourPoints( float orix, float oriy, float oriz, float widx, float widy, float widz, float forx, float fory, float forz, float upx,  float upy,  float upz, float scale );
GLmatrix  ViewMatrix( const GLvector3 from, const GLvector3 at, const GLvector3 world_up, const float roll );
GLmatrix  ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
void      TransformVertices( GLmatrix *pMatrix, GLvector4 *pSourceV, GLvector4 *pDestV, Uint32  NumVertor );

GLvector3 mat_getChrUp(GLmatrix mat);
GLvector3 mat_getChrRight(GLmatrix mat);
GLvector3 mat_getChrForward(GLmatrix mat);

GLvector3 mat_getCamUp(GLmatrix mat);
GLvector3 mat_getCamRight(GLmatrix mat);
GLvector3 mat_getCamForward(GLmatrix mat);

GLvector3 mat_getTranslate(GLmatrix mat);

void make_turntosin( void );
