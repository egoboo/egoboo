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

/// @file mathstuff.h
/// @The name's pretty self explanatory, doncha think?

#include <math.h>
#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PI                  3.1415926535897932384626433832795f
#define TWO_PI              6.283185307179586476925286766559f
#define SQRT_TWO            1.4142135623730950488016887242097f
#define INV_SQRT_TWO        0.70710678118654752440084436210485f

#define INV_FF              0.003921568627450980392156862745098f
#define INV_0100            0.00390625f
#define INV_FFFF            0.000015259021896696421759365224689097f

#define RAD_TO_TURN         10430.378350470452724949566316381f
#define TURN_TO_RAD         0.000095873799242852576857380474343257

#define TRIG_TABLE_BITS   14
#define TRIG_TABLE_SIZE   (1<<TRIG_TABLE_BITS)
#define TRIG_TABLE_MASK   (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_OFFSET (TRIG_TABLE_SIZE>>2)

/// @note - Aaron uses two terms without much attention to their meaning
///         I think that we should use "face" or "facing" to mean the fill 16-bit value
///         and use "turn" to be the TRIG_TABLE_BITS-bit value

extern float turntosin[TRIG_TABLE_SIZE];           ///< Convert chrturn>>2...  to sine
extern float turntocos[TRIG_TABLE_SIZE];           ///< Convert chrturn>>2...  to cosine

/// pre defined directions
#define FACE_WEST    0x0000
#define FACE_NORTH   0x4000                                 ///< Character facings
#define FACE_EAST    0x8000
#define FACE_SOUTH   0xC000

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Just define ABS, MIN, and MAX using macros for the moment. This is likely to be the
/// fastest and most cross-platform solution
#ifndef ABS
#    define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif

#ifndef SGN
#    define SGN(X)  (((X) == 0) ? 0 : (((X) > 0) ? 1 : -1) )
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
#define CopyMatrix( pMatrixDest, pMatrixSource ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( fmat_4x4_t ) )

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( isnan(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum { kX = 0, kY, kZ, kW };

typedef float fmat_4x4_base_t[16];       ///< the basic 4x4 floating point matrix type
typedef float fvec2_base_t[2];           ///< the basic floating point 2-vector type
typedef float fvec3_base_t[3];           ///< the basic floating point 3-vector type
typedef float fvec4_base_t[4];           ///< the basic floating point 4-vector type

/// A wrapper for fmat_4x4_base_t
/// Necessary in c so that the function return can be assigned to another matrix more simply.
typedef struct s_fmat_4x4  { fmat_4x4_base_t  v; } fmat_4x4_t;

/// A 2-vector type that allows more than one form of access
typedef union  u_fvec2     { fvec2_base_t v; struct { float x, y; }; struct { float s, t; }; } fvec2_t;

/// A 3-vector type that allows more than one form of access
typedef union  u_fvec3     { fvec3_base_t v; struct { float x, y, z; }; struct { float r, g, b; }; } fvec3_t;

/// A 4-vector type that allows more than one form of access
typedef union  u_fvec4     { fvec4_base_t v; struct { float x, y, z, w; }; struct { float r, g, b, a; }; } fvec4_t;

#define ZERO_VECT2 { {0,0} }
#define ZERO_VECT3 { {0,0,0} }
#define ZERO_VECT4 { {0,0,0,0} }

#define VECT2(XX,YY) { {XX,YY} }
#define VECT3(XX,YY,ZZ) { {XX,YY,ZZ} }
#define VECT4(XX,YY,ZZ,WW) { {XX,YY,ZZ,WW} }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A lookup table for trug values
extern float                   turntosin[TRIG_TABLE_SIZE];           ///< Convert chrturn>>2...  to sine

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// My lil' random number table

// swig chokes on the definition below
#ifdef SWIG
#    define RANDIE_BITS    12
#    define RANDIE_COUNT 4096
#else
#    define RANDIE_BITS   12
#    define RANDIE_COUNT (1 << RANDIE_BITS)
#endif

#define RANDIE_MASK  ((Uint32)(RANDIE_COUNT - 1))
#define RANDIE       randie[randindex & RANDIE_MASK ];  randindex++; randindex &= RANDIE_MASK

extern Uint32  randindex;
extern Uint16  randie[RANDIE_COUNT];   ///< My lil' random number table

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Data for doing the physics in bump_all_objects()
/// @details should prevent you from being bumped into a wall
struct s_phys_data
{
    fvec3_t        apos_0, apos_1;
    fvec3_t        avel;

    float          bumpdampen;                    ///< "Mass" = weight / bumpdampen
    Uint32         weight;                        ///< Weight
    float          dampen;                        ///< Bounciness
};
typedef struct s_phys_data phys_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fvec2_clear( fvec2_t * A );
bool_t fvec3_clear( fvec3_t * A );
bool_t fvec4_clear( fvec4_t * A );

float      fvec3_dot_product( fvec3_base_t A, fvec3_base_t   B );
fvec3_t    fvec3_normalize( fvec3_base_t A );
fvec3_t    fvec3_sub( fvec3_base_t A, fvec3_base_t   B );
fvec3_t    fvec3_cross_product( fvec3_base_t A, fvec3_base_t   B );

fmat_4x4_t IdentityMatrix( void );
fmat_4x4_t ZeroMatrix( void );
fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b );
fmat_4x4_t Translate( const float dx, const float dy, const float dz );
fmat_4x4_t RotateX( const float rads );
fmat_4x4_t RotateY( const float rads );
fmat_4x4_t RotateZ( const float rads );
fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez );
fmat_4x4_t ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float tx, const float ty, const float tz );
fmat_4x4_t FourPoints( fvec4_base_t ori, fvec4_base_t wid, fvec4_base_t frw, fvec4_base_t upx, float scale );
fmat_4x4_t ViewMatrix( const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll );
fmat_4x4_t ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
void       TransformVertices( fmat_4x4_t *pMatrix, fvec4_t   *pSourceV, fvec4_t   *pDestV, Uint32  NumVertor );

fvec3_t   mat_getChrUp( fmat_4x4_t mat );
fvec3_t   mat_getChrRight( fmat_4x4_t mat );
fvec3_t   mat_getChrForward( fmat_4x4_t mat );

fvec3_t   mat_getCamUp( fmat_4x4_t mat );
fvec3_t   mat_getCamRight( fmat_4x4_t mat );
fvec3_t   mat_getCamForward( fmat_4x4_t mat );

fvec3_t   mat_getTranslate( fmat_4x4_t mat );

void make_turntosin( void );

void   make_randie();
int    generate_irand_pair( IPair num );
int    generate_irand_range( FRange num );
int    generate_randmask( int base, int mask );

Uint16 vec_to_facing( float dx, float dy );
void   facing_to_vec( Uint16 facing, float * dx, float * dy );