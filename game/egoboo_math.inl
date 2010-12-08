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

/// @file egoboo_math.inl
/// @brief
/// @details Almost all of the math functions are intended to be inlined for maximum speed

#include "egoboo_math.h"
#include "log.h"
#include "ogl_include.h"
#include "ogl_debug.h"

#include <float.h>

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

// conversion functions
static INLINE FACING_T vec_to_facing( const float dx, const float dy );
static INLINE void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
static INLINE int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
static INLINE void getadd( const int min, const int value, const int max, int* valuetoadd );
static INLINE void fgetadd( const float min, const float value, const float max, float* valuetoadd );

// random functions
static INLINE int generate_irand_pair( const IPair num );
static INLINE int generate_irand_range( const FRange num );
static INLINE int generate_randmask( const int base, const int mask );

// vector functions
static INLINE bool_t  fvec2_self_clear( fvec2_base_t A );
static INLINE bool_t  fvec2_base_copy( fvec2_base_t A, const fvec2_base_t B );
static INLINE bool_t  fvec2_base_assign( fvec2_base_t A, const fvec2_t B );
static INLINE float   fvec2_length( const fvec2_base_t A );
static INLINE float   fvec2_length_abs( const fvec2_base_t A );
static INLINE float   fvec2_length_2( const fvec2_base_t A );
static INLINE bool_t  fvec2_self_scale( fvec2_base_t A, const float B );
static INLINE fvec2_t fvec2_sub( const fvec2_base_t A, const fvec2_base_t B );
static INLINE fvec2_t fvec2_normalize( const fvec2_base_t vec );
static INLINE bool_t  fvec2_self_normalize( fvec2_base_t A );
static INLINE float   fvec2_cross_product( const fvec2_base_t A, const fvec2_base_t B );
static INLINE float   fvec2_dot_product( const fvec2_base_t A, const fvec2_base_t B );
static INLINE float   fvec2_dist_abs( const fvec2_base_t A, const fvec2_base_t B );

static INLINE bool_t  fvec3_self_clear( fvec3_base_t A );
static INLINE bool_t  fvec3_base_copy( fvec3_base_t A, const fvec3_base_t B );
static INLINE bool_t  fvec3_base_assign( fvec3_base_t A, const fvec3_t B );
static INLINE bool_t  fvec3_self_scale( fvec3_base_t A, const float B );
static INLINE bool_t  fvec3_self_sum( fvec3_base_t A, const fvec3_base_t B );
static INLINE bool_t  fvec3_self_normalize( fvec3_base_t A );
static INLINE bool_t  fvec3_self_normalize_to( fvec3_base_t A, const float B );
static INLINE float   fvec3_length_2( const fvec3_base_t A );
static INLINE float   fvec3_length( const fvec3_base_t A );
static INLINE float   fvec3_length_abs( const fvec3_base_t A );
static INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B );
static INLINE float   fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B );
static INLINE fvec3_t fvec3_scale( const fvec3_base_t A, const float B );
static INLINE fvec3_t fvec3_normalize( const fvec3_base_t A );
static INLINE fvec3_t fvec3_add( const fvec3_base_t A, const fvec3_base_t B );
static INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B );
static INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B );
static INLINE float   fvec3_decompose( const fvec3_base_t A, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp );

static INLINE bool_t fvec4_self_clear( fvec4_base_t A );

// matrix functions
static INLINE fmat_4x4_t IdentityMatrix( void );
static INLINE fmat_4x4_t ZeroMatrix( void );
static INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b );
static INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz );
static INLINE fmat_4x4_t RotateX( const float rads );
static INLINE fmat_4x4_t RotateY( const float rads );
static INLINE fmat_4x4_t RotateZ( const float rads );
static INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez );
static INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale );
static INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll );
static INLINE fmat_4x4_t ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
static INLINE void       TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor );

static INLINE bool_t   mat_getChrUp( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getChrForward( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getChrRight( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getCamUp( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getCamRight( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getCamForward( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE bool_t   mat_getTranslate( const fmat_4x4_base_t mat, fvec3_base_t vec );
static INLINE float *  mat_getTranslate_v( const fmat_4x4_base_t mat );

//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE FACING_T vec_to_facing( const float dx, const float dy )
{
    return ( FACING_T )(( ATAN2( dy, dx ) + PI ) * RAD_TO_TURN );
}

//--------------------------------------------------------------------------------------------
static INLINE void facing_to_vec( const FACING_T facing, float * dx, float * dy )
{
    TURN_T turn = TO_TURN( facing - 0x8000 );

    if ( NULL != dx )
    {
        *dx = turntocos[turn];
    }

    if ( NULL != dy )
    {
        *dy = turntosin[turn];
    }
}

//--------------------------------------------------------------------------------------------
// ROTATION FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight )
{
    /// @details ZZ@> This function returns a direction between the major and minor ones, closer
    ///    to the major.

    int diff;

    // Align major direction with 0
    diff = ( int )minordir - ( int )majordir;

    if ( diff <= -( int )0x8000L )
    {
        diff += ( int )0x00010000L;
    }
    else if ( diff >= ( int )0x8000L )
    {
        diff -= ( int )0x00010000L;
    }

    return diff / weight;
}

//--------------------------------------------------------------------------------------------
// LIMITING FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE void getadd( const int min, const int value, const int max, int* valuetoadd )
{
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    int newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
static INLINE void fgetadd( const float min, const float value, const float max, float* valuetoadd )
{
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    float newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
// RANDOM FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE int generate_irand_pair( const IPair num )
{
    /// @details ZZ@> This function generates a random number

    int tmp = num.base;
    if ( num.rand > 1 )
    {
        int irand = RANDIE;
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_irand_range( const FRange num )
{
    /// @details ZZ@> This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_randmask( const int base, const int mask )
{
    /// @details ZZ@> This function generates a random number
    int tmp;
    int irand = RANDIE;

    tmp = base;
    if ( mask > 0 )
    {
        tmp += irand & mask;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
// VECTOR FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec2_self_clear( fvec2_base_t A )
{
    if ( NULL == A ) return bfalse;

    A[kX] = A[kY] = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec2_base_copy( fvec2_base_t A, const fvec2_base_t B )
{
    if ( NULL == A ) return bfalse;

    if ( NULL == B ) return fvec2_self_clear( A );

    A[kX] = B[kX];
    A[kY] = B[kY];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t  fvec2_base_assign( fvec2_base_t A, const fvec2_t B )
{
    if ( NULL == A ) return bfalse;

    A[kX] = B.v[kX];
    A[kY] = B.v[kY];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec2_self_scale( fvec2_base_t A, const float B )
{
    if ( NULL == A ) return bfalse;

    A[kX] *= B;
    A[kY] *= B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_length_abs( const fvec2_base_t A )
{
    if ( NULL == A ) return 0.0f;

    return ABS( A[kX] ) + ABS( A[kY] );
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_length_2( const fvec2_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY];

    return A2;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_length( const fvec2_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY];

    return SQRT( A2 );
}

//--------------------------------------------------------------------------------------------
static INLINE fvec2_t fvec2_sub( const fvec2_base_t A, const fvec2_base_t B )
{
    fvec2_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_dist_abs( const fvec2_base_t A, const fvec2_base_t B )
{
    return ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] );
}

//--------------------------------------------------------------------------------------------
static INLINE fvec2_t fvec2_scale( const fvec2_base_t A, const float B )
{
    fvec2_t tmp = ZERO_VECT2;

    if ( NULL == A || 0.0f == B ) return tmp;

    tmp.v[kX] = A[kX] * B;
    tmp.v[kY] = A[kY] * B;

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE fvec2_t fvec2_normalize( const fvec2_base_t vec )
{
    fvec2_t tmp = ZERO_VECT2;

    if ( ABS( vec[kX] ) + ABS( vec[kY] ) > 0 )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        tmp.x = vec[kX] * inv_len;
        LOG_NAN( tmp.x );

        tmp.y = vec[kY] * inv_len;
        LOG_NAN( tmp.y );
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t  fvec2_self_normalize( fvec2_base_t A )
{
    float len2;
    float inv_len;

    if ( NULL == A ) return bfalse;

    if ( 0.0f == fvec2_length_abs( A ) ) return bfalse;

    len2 = A[kX] * A[kX] + A[kY] * A[kY];
    inv_len = 1.0f / SQRT( len2 );

    A[kX] *= inv_len;
    A[kY] *= inv_len;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_cross_product( const fvec2_base_t A, const fvec2_base_t B )
{
    return A[kX] * B[kY] - A[kY] * B[kX];
}

//--------------------------------------------------------------------------------------------
static INLINE float   fvec2_dot_product( const fvec2_base_t A, const fvec2_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec3_self_clear( fvec3_base_t A )
{
    if ( NULL == A ) return bfalse;

    A[kX] = A[kY] = A[kZ] = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec3_base_copy( fvec3_base_t A, const fvec3_base_t B )
{
    if ( NULL == A ) return bfalse;

    if ( NULL == B ) return fvec3_self_clear( A );

    A[kX] = B[kX];
    A[kY] = B[kY];
    A[kZ] = B[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t  fvec3_base_assign( fvec3_base_t A, const fvec3_t B )
{
    if ( NULL == A ) return bfalse;

    A[kX] = B.v[kX];
    A[kY] = B.v[kY];
    A[kZ] = B.v[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec3_self_scale( fvec3_base_t A, const float B )
{
    if ( NULL == A ) return bfalse;

    A[kX] *= B;
    A[kY] *= B;
    A[kZ] *= B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec3_self_sum( fvec3_base_t A, const fvec3_base_t B )
{
    if ( NULL == A || NULL == B ) return bfalse;

    A[kX] += B[kX];
    A[kY] += B[kY];
    A[kZ] += B[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length_abs( const fvec3_base_t A )
{
    if ( NULL == A ) return 0.0f;

    return ABS( A[kX] ) + ABS( A[kY] ) + ABS( A[kZ] );
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length_2( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    return A2;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    return SQRT( A2 );
}

//--------------------------------------------------------------------------------------------
static INLINE fvec3_t fvec3_add( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] + B[kX];
    tmp.y = A[kY] + B[kY];
    tmp.z = A[kZ] + B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];
    tmp.z = A[kZ] - B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE fvec3_t fvec3_scale( const fvec3_base_t A, const float B )
{
    fvec3_t tmp = ZERO_VECT3;

    if ( NULL == A || 0.0f == B ) return tmp;

    tmp.v[kX] = A[kX] * B;
    tmp.v[kY] = A[kY] * B;
    tmp.v[kZ] = A[kZ] * B;

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE fvec3_t fvec3_normalize( const fvec3_base_t vec )
{
    float len2, inv_len;
    fvec3_t tmp = ZERO_VECT3;

    if ( NULL == vec ) return tmp;

    if ( 0.0f == fvec3_length_abs( vec ) ) return tmp;

    len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
    inv_len = 1.0f / SQRT( len2 );
    LOG_NAN( inv_len );

    tmp.x = vec[kX] * inv_len;
    tmp.y = vec[kY] * inv_len;
    tmp.z = vec[kZ] * inv_len;

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t  fvec3_self_normalize( fvec3_base_t A )
{
    if ( NULL == A ) return bfalse;

    if ( 0.0f != fvec3_length_abs( A ) )
    {
        float len2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        A[kX] *= inv_len;
        A[kY] *= inv_len;
        A[kZ] *= inv_len;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec3_self_normalize_to( fvec3_base_t vec, const float B )
{
    if ( NULL == vec ) return bfalse;

    if ( 0.0f == B )
    {
        fvec3_self_clear( vec );
        return btrue;
    }

    if ( 0.0f != fvec3_length_abs( vec ) )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
        float inv_len = B / SQRT( len2 );
        LOG_NAN( inv_len );

        vec[kX] *= inv_len;
        vec[kY] *= inv_len;
        vec[kZ] *= inv_len;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t   tmp;

    tmp.x = A[kY] * B[kZ] - A[kZ] * B[kY];
    tmp.y = A[kZ] * B[kX] - A[kX] * B[kZ];
    tmp.z = A[kX] * B[kY] - A[kY] * B[kX];

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_decompose( const fvec3_base_t A, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp )
{
    /// BB@> the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

    float dot;

    // error trapping
    if ( NULL == A || NULL == vnrm ) return 0.0f;

    // if this is true, there is no reason to run this function
    dot = fvec3_dot_product( A, vnrm );

    if ( 0.0f == dot )
    {
        // handle optional parameters
        if ( NULL == vpara && NULL == vperp )
        {
            // no point in doing anything
            return 0.0f;
        }
        else if ( NULL == vpara )
        {
            vperp[kX] = A[kX];
            vperp[kY] = A[kY];
            vperp[kZ] = A[kZ];
        }
        else if ( NULL == vperp )
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;
        }
        else
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;

            vperp[kX] = A[kX];
            vperp[kY] = A[kY];
            vperp[kZ] = A[kZ];
        }
    }
    else
    {
        // handle optional parameters
        if ( NULL == vpara && NULL == vperp )
        {
            // no point in doing anything
            return 0.0f;
        }
        else if ( NULL == vpara )
        {
            vperp[kX] = A[kX] - dot * vnrm[kX];
            vperp[kY] = A[kY] - dot * vnrm[kY];
            vperp[kZ] = A[kZ] - dot * vnrm[kZ];
        }
        else if ( NULL == vperp )
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];
        }
        else
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];

            vperp[kX] = A[kX] - vpara[kX];
            vperp[kY] = A[kY] - vpara[kY];
            vperp[kZ] = A[kZ] - vpara[kZ];
        }
    }

    return dot;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B )
{
    return ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] ) + ABS( A[kZ] - B[kZ] );
}

//--------------------------------------------------------------------------------------------
static INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY] + A[kZ]*B[kZ];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool_t fvec4_self_clear( fvec4_base_t A )
{
    if ( NULL == A ) return bfalse;

    A[kX] = A[kY] = A[kZ] = 0.0f;
    A[kW] = 1.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
// MATIX FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getTranslate( const fmat_4x4_base_t mat, fvec3_base_t vpos )
{
    if ( NULL == mat || NULL == vpos ) return bfalse;

    vpos[kX] = mat[MAT_IDX( 3, 0 )];
    vpos[kY] = mat[MAT_IDX( 3, 1 )];
    vpos[kZ] = mat[MAT_IDX( 3, 2 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getChrUp( const fmat_4x4_base_t mat, fvec3_base_t vup )
{
    if ( NULL == mat || NULL == vup ) return bfalse;

    // for a character
    vup[kX] = mat[MAT_IDX( 2, 0 )];
    vup[kY] = mat[MAT_IDX( 2, 1 )];
    vup[kZ] = mat[MAT_IDX( 2, 2 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getChrForward( const fmat_4x4_base_t mat, fvec3_base_t vright )
{
    if ( NULL == mat || NULL == vright ) return bfalse;

    // for a character
    vright[kX] = -mat[MAT_IDX( 0, 0 )];
    vright[kY] = -mat[MAT_IDX( 0, 1 )];
    vright[kZ] = -mat[MAT_IDX( 0, 2 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getChrRight( const fmat_4x4_base_t mat, fvec3_base_t vfrw )
{
    if ( NULL == mat || NULL == vfrw ) return bfalse;

    // for a character's matrix
    vfrw[kX] = mat[MAT_IDX( 1, 0 )];
    vfrw[kY] = mat[MAT_IDX( 1, 1 )];
    vfrw[kZ] = mat[MAT_IDX( 1, 2 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getCamUp( const fmat_4x4_base_t mat, fvec3_base_t vup )
{
    if ( NULL == mat || NULL == vup ) return bfalse;

    // for the camera
    vup[kX] = -mat[MAT_IDX( 0, 1 )];
    vup[kY] = -mat[MAT_IDX( 1, 1 )];
    vup[kZ] = -mat[MAT_IDX( 2, 1 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getCamRight( const fmat_4x4_base_t mat, fvec3_base_t vright )
{
    if ( NULL == mat || NULL == vright ) return bfalse;

    // for the camera
    vright[kX] = mat[MAT_IDX( 0, 0 )];
    vright[kY] = mat[MAT_IDX( 1, 0 )];
    vright[kZ] = mat[MAT_IDX( 2, 0 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mat_getCamForward( const fmat_4x4_base_t mat, fvec3_base_t vfrw )
{
    if ( NULL == mat || NULL == vfrw ) return bfalse;

    // for the camera
    vfrw[kX] = mat[MAT_IDX( 0, 2 )];
    vfrw[kY] = mat[MAT_IDX( 1, 2 )];
    vfrw[kZ] = mat[MAT_IDX( 2, 2 )];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_getTranslate_v( const fmat_4x4_base_t mat )
{
    static fvec3_t pos;

    pos.x = mat[MAT_IDX( 3, 0 )];
    pos.y = mat[MAT_IDX( 3, 1 )];
    pos.z = mat[MAT_IDX( 3, 2 )];

    return pos.v;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t IdentityMatrix()
{
    fmat_4x4_t tmp;

    tmp.CNV( 0, 0 ) = 1; tmp.CNV( 1, 0 ) = 0; tmp.CNV( 2, 0 ) = 0; tmp.CNV( 3, 0 ) = 0;
    tmp.CNV( 0, 1 ) = 0; tmp.CNV( 1, 1 ) = 1; tmp.CNV( 2, 1 ) = 0; tmp.CNV( 3, 1 ) = 0;
    tmp.CNV( 0, 2 ) = 0; tmp.CNV( 1, 2 ) = 0; tmp.CNV( 2, 2 ) = 1; tmp.CNV( 3, 2 ) = 0;
    tmp.CNV( 0, 3 ) = 0; tmp.CNV( 1, 3 ) = 0; tmp.CNV( 2, 3 ) = 0; tmp.CNV( 3, 3 ) = 1;

    return( tmp );
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ZeroMatrix( void )
{
    // initializes matrix to zero

    fmat_4x4_t ret;
    int i, j;

    for ( i = 0; i < 4; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            ret.CNV( i, j ) = 0;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b )
{
    fmat_4x4_t ret = ZERO_MAT_4X4;
    int i, j, k;

    for ( i = 0; i < 4; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            for ( k = 0; k < 4; k++ )
            {
                ret.CNV( i, j ) += a.CNV( k, j ) * b.CNV( i, k );
            }
        }
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 3, 0 ) = dx;
    ret.CNV( 3, 1 ) = dy;
    ret.CNV( 3, 2 ) = dz;

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t RotateX( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 1, 1 ) = cosine;
    ret.CNV( 2, 2 ) = cosine;
    ret.CNV( 1, 2 ) = -sine;
    ret.CNV( 2, 1 ) = sine;

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t RotateY( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = cosine; // 0,0
    ret.CNV( 2, 2 ) = cosine; // 2,2
    ret.CNV( 0, 2 ) = sine; // 0,2
    ret.CNV( 2, 0 ) = -sine; // 2,0

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t RotateZ( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = cosine; // 0,0
    ret.CNV( 1, 1 ) = cosine; // 1,1
    ret.CNV( 0, 1 ) = -sine; // 0,1
    ret.CNV( 1, 0 ) = sine; // 1,0

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = sizex; // 0,0
    ret.CNV( 1, 1 ) = sizey; // 1,1
    ret.CNV( 2, 2 ) = sizez; // 2,2

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate_SpaceFixed( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    fmat_4x4_t ret;

    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    ret.CNV( 0, 0 ) = scale_x * ( cz * cy );
    ret.CNV( 0, 1 ) = scale_x * ( cz * sy * sx + sz * cx );
    ret.CNV( 0, 2 ) = scale_x * ( sz * sx - cz * sy * cx );
    ret.CNV( 0, 3 ) = 0.0f;

    ret.CNV( 1, 0 ) = scale_y * ( -sz * cy );
    ret.CNV( 1, 1 ) = scale_y * ( -sz * sy * sx + cz * cx );
    ret.CNV( 1, 2 ) = scale_y * ( sz * sy * cx + cz * sx );
    ret.CNV( 1, 3 ) = 0.0f;

    ret.CNV( 2, 0 ) = scale_z * ( sy );
    ret.CNV( 2, 1 ) = scale_z * ( -cy * sx );
    ret.CNV( 2, 2 ) = scale_z * ( cy * cx );
    ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 3, 0 ) = translate_x;
    ret.CNV( 3, 1 ) = translate_y;
    ret.CNV( 3, 2 ) = translate_z;
    ret.CNV( 3, 3 ) = 1.0f;

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate_BodyFixed( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    /// @details BB@> Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

    fmat_4x4_t ret;

    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    //ret.CNV( 0, 0 ) = scale_x * ( cz * cy);
    //ret.CNV( 0, 1 ) = scale_x * ( sz * cy);
    //ret.CNV( 0, 2 ) = scale_x * (-sy);
    //ret.CNV( 0, 3 ) = 0.0f;

    //ret.CNV( 1, 0 ) = scale_y * (-sz * cx + cz * sy * sx);
    //ret.CNV( 1, 1 ) = scale_y * ( cz * cx + sz * sy * sx);
    //ret.CNV( 1, 2 ) = scale_y * ( cy * sx);
    //ret.CNV( 1, 3 ) = 0.0f;

    //ret.CNV( 2, 0 ) = scale_z * ( sz * sx + cz * sy * cx);
    //ret.CNV( 2, 1 ) = scale_z * (-cz * sx + sz * sy * cx);
    //ret.CNV( 2, 2 ) = scale_z * ( cy * cx);
    //ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 0, 0 ) = scale_x * ( cz * cy - sz * sy * sx );
    ret.CNV( 0, 1 ) = scale_x * ( sz * cy + cz * sy * sx );
    ret.CNV( 0, 2 ) = scale_x * ( -cx * sy );
    ret.CNV( 0, 3 ) = 0.0f;

    ret.CNV( 1, 0 ) = scale_y * ( -sz * cx );
    ret.CNV( 1, 1 ) = scale_y * ( cz * cx );
    ret.CNV( 1, 2 ) = scale_y * ( sx );
    ret.CNV( 1, 3 ) = 0.0f;

    ret.CNV( 2, 0 ) = scale_z * ( cz * sy + sz * sx * cy );
    ret.CNV( 2, 1 ) = scale_z * ( sz * sy - cz * sx * cy );
    ret.CNV( 2, 2 ) = scale_z * ( cy * cx );
    ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 3, 0 ) = translate_x;
    ret.CNV( 3, 1 ) = translate_y;
    ret.CNV( 3, 2 ) = translate_z;
    ret.CNV( 3, 3 ) = 1.0f;

    return ret;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t up, const float scale )
{
    fmat_4x4_t tmp;

    fvec3_t vWid, vFor, vUp;

    vWid.x = wid[kX] - ori[kX];
    vWid.y = wid[kY] - ori[kY];
    vWid.z = wid[kZ] - ori[kZ];

    vUp.x = up[kX] - ori[kX];
    vUp.y = up[kY] - ori[kY];
    vUp.z = up[kZ] - ori[kZ];

    vFor.x = frw[kX] - ori[kX];
    vFor.y = frw[kY] - ori[kY];
    vFor.z = frw[kZ] - ori[kZ];

    fvec3_self_normalize( vWid.v );
    fvec3_self_normalize( vUp.v );
    fvec3_self_normalize( vFor.v );

    tmp.CNV( 0, 0 ) = -scale * vWid.x;  // HUK
    tmp.CNV( 0, 1 ) = -scale * vWid.y;  // HUK
    tmp.CNV( 0, 2 ) = -scale * vWid.z;  // HUK
    tmp.CNV( 0, 3 ) = 0.0f;

    tmp.CNV( 1, 0 ) = scale * vFor.x;
    tmp.CNV( 1, 1 ) = scale * vFor.y;
    tmp.CNV( 1, 2 ) = scale * vFor.z;
    tmp.CNV( 1, 3 ) = 0.0f;

    tmp.CNV( 2, 0 ) = scale * vUp.x;
    tmp.CNV( 2, 1 ) = scale * vUp.y;
    tmp.CNV( 2, 2 ) = scale * vUp.z;
    tmp.CNV( 2, 3 ) = 0.0f;

    tmp.CNV( 3, 0 ) = ori[kX];
    tmp.CNV( 3, 1 ) = ori[kY];
    tmp.CNV( 3, 2 ) = ori[kZ];
    tmp.CNV( 3, 3 ) = 1.0f;

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from,     // camera location
                                     const fvec3_base_t   at,        // camera look-at target
                                     const fvec3_base_t   world_up,  // world’s up, usually 0, 0, 1
                                     const float roll )         // clockwise roll around
//   viewing direction,
//   in radians
{
    /// @details MN@> This probably should be replaced by a call to gluLookAt(),
    ///               don't see why we need to make our own...

    fmat_4x4_t view = IdentityMatrix();
    fvec3_t   up, right, view_dir, temp;

    temp     = fvec3_sub( at, from );
    view_dir = fvec3_normalize( temp.v );
    right    = fvec3_cross_product( world_up, view_dir.v );
    up       = fvec3_cross_product( view_dir.v, right.v );
    fvec3_self_normalize( right.v );
    fvec3_self_normalize( up.v );

    view.CNV( 0, 0 ) = right.x;
    view.CNV( 1, 0 ) = right.y;
    view.CNV( 2, 0 ) = right.z;
    view.CNV( 0, 1 ) = up.x;
    view.CNV( 1, 1 ) = up.y;
    view.CNV( 2, 1 ) = up.z;
    view.CNV( 0, 2 ) = view_dir.x;
    view.CNV( 1, 2 ) = view_dir.y;
    view.CNV( 2, 2 ) = view_dir.z;
    view.CNV( 3, 0 ) = -fvec3_dot_product( right.v,    from );
    view.CNV( 3, 1 ) = -fvec3_dot_product( up.v,       from );
    view.CNV( 3, 2 ) = -fvec3_dot_product( view_dir.v, from );

    if ( roll != 0.0f )
    {
        // MatrixMult function shown above
        view = MatrixMult( RotateZ( -roll ), view );
    }

    return view;
}

//--------------------------------------------------------------------------------------------
static INLINE fmat_4x4_t ProjectionMatrix( const float near_plane,    // distance to near clipping plane
        const float far_plane,      // distance to far clipping plane
        const float fov )           // field of view angle, in radians
{
    /// @details MN@> Again, there is a gl function for this, glFrustum or gluPerspective...
    ///               does this account for viewport ratio?

    fmat_4x4_t ret = ZERO_MAT_4X4;

    float c = COS( fov * 0.5f );
    float s = SIN( fov * 0.5f );
    float Q = s / ( 1.0f - near_plane / far_plane );

    ret.CNV( 0, 0 ) = c;         // 0,0
    ret.CNV( 1, 1 ) = c;         // 1,1
    ret.CNV( 2, 2 ) = Q;         // 2,2
    ret.CNV( 3, 2 ) = -Q * near_plane; // 3,2
    ret.CNV( 2, 3 ) = s;         // 2,3

    return ret;
}

//----------------------------------------------------
static INLINE void  TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor )
{
    /// @details  GS@> This is just a MulVectorMatrix for now. The W division and screen size multiplication
    ///                must be done afterward.
    ///
    /// BB@> the matrix transformation for OpenGL vertices. Some minor optimizations.
    ///      The value pSourceV->w is assumed to be constant for all of the elements of pSourceV

    Uint32    cnt;
    fvec4_t * SourceIt = ( fvec4_t * )pSourceV;

    if ( 1.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[8] + pMatrix->v[12];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[9] + pMatrix->v[13];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10] + pMatrix->v[14];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11] + pMatrix->v[15];

            pDestV++;
            SourceIt++;
        }
    }
    else if ( 0.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[8];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[9];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11];

            pDestV++;
            SourceIt++;
        }
    }
    else
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[8]  + SourceIt->w * pMatrix->v[12];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[9]  + SourceIt->w * pMatrix->v[13];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10] + SourceIt->w * pMatrix->v[14];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11] + SourceIt->w * pMatrix->v[15];

            pDestV++;
            SourceIt++;
        }
    }
}
