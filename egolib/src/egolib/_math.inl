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

/// @file egolib/_math.inl
/// @brief
/// @details Almost all of the math functions are intended to be inlined for maximum speed

#include <float.h>

#include "egolib/_math.h"
#include "egolib/platform.h"
#include "egolib/log.h"

#include "egolib/extensions/ogl_include.h"
#include "egolib/extensions/ogl_debug.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define IEEE32_FRACTION 0x007FFFFFL
#define IEEE32_EXPONENT 0x7F800000L
#define IEEE32_SIGN     0x80000000L

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

#if defined(TEST_NAN_RESULT)
#    define LOG_NAN_FVEC2(XX)      if( !fvec2_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#    define LOG_NAN_FVEC3(XX)      if( !fvec3_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#    define LOG_NAN_FVEC4(XX)      if( !fvec4_valid(XX) ) log_error( "**** A math operation resulted in an invalid vector result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN_FVEC2(XX)
#    define LOG_NAN_FVEC3(XX)
#    define LOG_NAN_FVEC4(XX)
#endif

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif

// ieee 32-bit floating point number functions
    static INLINE Uint32 float32_to_uint32( float f );
    static INLINE float  uint32_to_float32( Uint32 i );
    static INLINE bool ieee32_infinite( float f );
    static INLINE bool ieee32_nan( float f );
    static INLINE bool ieee32_bad( float f );

// conversion functions
    static INLINE FACING_T vec_to_facing( const float dx, const float dy );
    static INLINE void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
    static INLINE int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
    static INLINE void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    static INLINE void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    static INLINE int generate_irand_pair( const IPair num );
    static INLINE int generate_irand_range( const FRange num );
    static INLINE int generate_randmask( const int base, const Uint32 mask );

// vector functions
    static INLINE bool  fvec2_valid( const fvec2_base_t A );
    static INLINE bool  fvec2_self_clear( fvec2_base_t A );
    static INLINE bool  fvec2_base_copy( fvec2_base_t A, const fvec2_base_t B );
    static INLINE float   fvec2_length( const fvec2_base_t A );
    static INLINE float   fvec2_length_abs( const fvec2_base_t A );
    static INLINE float   fvec2_length_2( const fvec2_base_t A );
    static INLINE bool  fvec2_self_scale( fvec2_base_t A, const float B );
    static INLINE bool  fvec2_self_sum( fvec2_base_t A, const fvec2_base_t B );
    static INLINE bool  fvec2_self_normalize( fvec2_base_t A );
    static INLINE float   fvec2_cross_product( const fvec2_base_t A, const fvec2_base_t B );
    static INLINE float   fvec2_dot_product( const fvec2_base_t A, const fvec2_base_t B );
    static INLINE float   fvec2_dist_abs( const fvec2_base_t A, const fvec2_base_t B );
    static INLINE float * fvec2_sub( fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS );
    static INLINE float * fvec2_add( fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS );
    static INLINE float * fvec2_normalize( fvec2_base_t DST, const fvec2_base_t SRC );
    static INLINE float * fvec2_scale( fvec2_base_t DST, const fvec2_base_t SRC, const float B );

    static INLINE bool  fvec3_valid( const fvec3_base_t A );
    static INLINE bool  fvec3_self_clear( fvec3_base_t A );
    static INLINE bool  fvec3_self_scale( fvec3_base_t A, const float B );
    static INLINE bool  fvec3_self_sum( fvec3_base_t A, const fvec3_base_t RHS );
    static INLINE float   fvec3_self_normalize( fvec3_base_t A );
    static INLINE float   fvec3_self_normalize_to( fvec3_base_t A, const float B );
    static INLINE float   fvec3_length_2( const fvec3_base_t SRC );
    static INLINE float   fvec3_length( const fvec3_base_t SRC );
    static INLINE float   fvec3_length_abs( const fvec3_base_t SRC );
    static INLINE float   fvec3_dot_product( const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float   fvec3_dist_abs( const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float   fvec3_dist_2( const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float * fvec3_base_copy( fvec3_base_t DST, const fvec3_base_t SRC );
    static INLINE float * fvec3_scale( fvec3_base_t DST, const fvec3_base_t SRC, const float B );
    static INLINE float * fvec3_normalize( fvec3_base_t DST, const fvec3_base_t SRC );
    static INLINE float * fvec3_add( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float * fvec3_sub( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float * fvec3_cross_product( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS );
    static INLINE float   fvec3_decompose( const fvec3_base_t src, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp );

    static INLINE bool fvec4_valid( const fvec4_base_t A );
    static INLINE bool fvec4_self_clear( fvec4_base_t A );
    static INLINE bool fvec4_self_scale( fvec4_base_t A, const float B );

// matrix functions
    static INLINE float * mat_Copy( fmat_4x4_base_t DST, const fmat_4x4_base_t src );
    static INLINE float * mat_Identity( fmat_4x4_base_t DST );
    static INLINE float * mat_Zero( fmat_4x4_base_t DST );
    static INLINE float * mat_Multiply( fmat_4x4_base_t DST, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2 );
    static INLINE float * mat_Translate( fmat_4x4_base_t DST, const float dx, const float dy, const float dz );
    static INLINE float * mat_RotateX( fmat_4x4_base_t DST, const float rads );
    static INLINE float * mat_RotateY( fmat_4x4_base_t DST, const float rads );
    static INLINE float * mat_RotateZ( fmat_4x4_base_t DST, const float rads );
    static INLINE float * mat_ScaleXYZ( fmat_4x4_base_t DST, const float sizex, const float sizey, const float sizez );
    static INLINE float * mat_FourPoints( fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale );
    static INLINE float * mat_View( fmat_4x4_base_t DST, const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll );
    static INLINE float * mat_Projection( fmat_4x4_base_t DST, const float near_plane,  const float far_plane,  const float fov, const float ar );
    static INLINE float * mat_Projection_orig( fmat_4x4_base_t DST, const float near_plane, const float far_plane, const float fov );
    static INLINE void    mat_TransformVertices( const fmat_4x4_base_t Matrix, const fvec4_t pSourceV[], fvec4_t pDestV[], const Uint32 NumVertor );

    static INLINE bool   mat_getChrUp( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getChrForward( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getChrRight( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getCamUp( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getCamRight( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getCamForward( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE bool   mat_getTranslate( const fmat_4x4_base_t mat, fvec3_base_t vec );
    static INLINE float *  mat_getTranslate_v( const fmat_4x4_base_t mat );

    static INLINE float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed( fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z );
    static INLINE float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed( fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z );

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
// IEEE 32-BIT FLOATING POINT NUMBER FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE Uint32 float32_to_uint32( float f )
{
    union { Uint32 i; float f; } val;

    val.f = f;

    return val.i;
}

//--------------------------------------------------------------------------------------------
static INLINE float uint32_to_float32( Uint32 i )
{
    union { Uint32 i; float f; } val;

    val.i = i;

    return val.f;

}

//--------------------------------------------------------------------------------------------
static INLINE bool ieee32_infinite( float f )
{
    Uint32 u = float32_to_uint32( f );

    return ( 0 == ( u & IEEE32_FRACTION ) ) && ( IEEE32_EXPONENT == ( u & IEEE32_EXPONENT ) );
}

//--------------------------------------------------------------------------------------------
static INLINE bool ieee32_nan( float f )
{
    Uint32 u = float32_to_uint32( f );

    return ( 0 != ( u&IEEE32_FRACTION ) ) && ( IEEE32_EXPONENT == ( u & IEEE32_EXPONENT ) );
}

//--------------------------------------------------------------------------------------------
static INLINE bool ieee32_bad( float f )
{
    Uint32 u = float32_to_uint32( f );

    return ( IEEE32_EXPONENT == ( u & IEEE32_EXPONENT ) ) ? true : false;
}

//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------

static INLINE FACING_T vec_to_facing( const float dx, const float dy )
{
    return ( FACING_T )( RAD_TO_FACING( ATAN2( dy, dx ) + PI ) );
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
    /// @author ZZ
    /// @details This function returns a direction between the major and minor ones, closer
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
static INLINE void getadd_int( const int min, const int value, const int max, int* valuetoadd )
{
    /// @author ZZ
    /// @details This function figures out what value to add should be in order
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
static INLINE void getadd_flt( const float min, const float value, const float max, float* valuetoadd )
{
    /// @author ZZ
    /// @details This function figures out what value to add should be in order
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
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = RANDIE;

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_irand_range( const FRange num )
{
    /// @author ZZ
    /// @details This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_randmask( const int base, const Uint32 mask )
{
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = RANDIE;

    tmp = base;
    tmp += irand & mask;

    return tmp;
}

//--------------------------------------------------------------------------------------------
// VECTOR FUNCTIONS
//--------------------------------------------------------------------------------------------

static INLINE bool fvec2_valid( const fvec2_base_t A )
{
    int cnt;

    if ( NULL == A ) return false;

    for ( cnt = 0; cnt < 2; cnt++ )
    {
        if ( ieee32_bad( A[cnt] ) ) return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec2_self_clear( fvec2_base_t A )
{
    if ( NULL == A ) return false;

    A[kX] = A[kY] = 0.0f;

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec2_base_copy( fvec2_base_t A, const fvec2_base_t B )
{
    if ( NULL == A ) return false;

    if ( NULL == B ) return fvec2_self_clear( A );

    A[kX] = B[kX];
    A[kY] = B[kY];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec2_self_scale( fvec2_base_t A, const float B )
{
    if ( NULL == A ) return false;

    A[kX] *= B;
    A[kY] *= B;

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec2_self_sum( fvec2_base_t A, const fvec2_base_t B )
{
    if ( NULL == A || NULL == B ) return false;

    A[kX] += B[kX];
    A[kY] += B[kY];

    LOG_NAN_FVEC2( A );

    return true;
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
static INLINE float * fvec2_sub( fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS )
{
    if ( NULL == DST )
    {
        return NULL;
    }
    else if ( NULL == LHS && NULL == RHS )
    {
        fvec2_self_clear( DST );
    }
    else if ( NULL == LHS )
    {
        DST[kX] = - RHS[kX];
        DST[kY] = - RHS[kY];
    }
    else if ( NULL == RHS )
    {
        DST[kX] = LHS[kX];
        DST[kY] = LHS[kY];
    }
    else
    {
        DST[kX] = LHS[kX] - RHS[kX];
        DST[kY] = LHS[kY] - RHS[kY];
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec2_add( fvec2_base_t DST, const fvec2_base_t LHS, const fvec2_base_t RHS )
{
    if ( NULL == DST )
    {
        return NULL;
    }
    else if ( NULL == LHS && NULL == RHS )
    {
        fvec2_self_clear( DST );
    }
    else if ( NULL == LHS )
    {
        fvec2_base_copy( DST, RHS );
    }
    else if ( NULL == RHS )
    {
        fvec2_base_copy( DST, LHS );
    }
    else
    {
        DST[kX] = LHS[kX] - RHS[kX];
        DST[kY] = LHS[kY] - RHS[kY];
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec2_dist_abs( const fvec2_base_t A, const fvec2_base_t B )
{
    return ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] );
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec2_scale( fvec2_base_t DST, const fvec2_base_t SRC, const float B )
{
    if ( NULL == DST ) return NULL;

    if ( NULL == SRC || 0.0f == B )
    {
        fvec2_self_clear( DST );
    }
    else
    {
        DST[kX] = SRC[kX] * B;
        DST[kY] = SRC[kY] * B;
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec2_normalize( fvec2_base_t DST, const fvec2_base_t SRC )
{
    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == SRC )
    {
        fvec2_self_clear( DST );
    }
    else if ( 0.0f == ABS( SRC[kX] ) + ABS( SRC[kY] ) )
    {
        fvec2_self_clear( DST );
    }
    else
    {
        float len2 = SRC[kX] * SRC[kX] + SRC[kY] * SRC[kY];

        if ( 0.0f != len2 )
        {
            float inv_len = 1.0f / SQRT( len2 );
            LOG_NAN( inv_len );

            DST[kX] = SRC[kX] * inv_len;
            LOG_NAN( DST[kX] );

            DST[kY] = SRC[kY] * inv_len;
            LOG_NAN( DST[kY] );
        }
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE bool  fvec2_self_normalize( fvec2_base_t A )
{
    float len2;
    float inv_len;

    if ( NULL == A ) return false;

    if ( 0.0f == fvec2_length_abs( A ) ) return false;

    len2 = A[kX] * A[kX] + A[kY] * A[kY];
    inv_len = 1.0f / SQRT( len2 );

    A[kX] *= inv_len;
    A[kY] *= inv_len;

    return true;
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
static INLINE bool fvec3_valid( const fvec3_base_t A )
{
    int cnt;

    if ( NULL == A ) return false;

    for ( cnt = 0; cnt < 3; cnt++ )
    {
        if ( ieee32_bad( A[cnt] ) ) return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec3_self_clear( fvec3_base_t A )
{
    if ( NULL == A ) return false;

    A[kX] = A[kY] = A[kZ] = 0.0f;

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_base_copy( fvec3_base_t DST, const fvec3_base_t SRC )
{
    if ( NULL == DST ) return NULL;

    if ( NULL == SRC )
    {
        fvec3_self_clear( DST );
    }
    else
    {
        DST[kX] = SRC[kX];
        DST[kY] = SRC[kY];
        DST[kZ] = SRC[kZ];

        LOG_NAN_FVEC3( DST );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec3_self_scale( fvec3_base_t A, const float B )
{
    if ( NULL == A ) return false;

    A[kX] *= B;
    A[kY] *= B;
    A[kZ] *= B;

    LOG_NAN_FVEC3( A );

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec3_self_sum( fvec3_base_t A, const fvec3_base_t B )
{
    if ( NULL == A || NULL == B ) return false;

    A[kX] += B[kX];
    A[kY] += B[kY];
    A[kZ] += B[kZ];

    LOG_NAN_FVEC3( A );

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length_abs( const fvec3_base_t A )
{
    float retval;

    if ( NULL == A ) return 0.0f;

    retval = ABS( A[kX] ) + ABS( A[kY] ) + ABS( A[kZ] );

    //DEBUG
    if ( ieee32_bad( retval ) )
    {
        log_debug( "Game decided to crash, but I refuse! (%s - %d)\n", __FILE__, __LINE__ );
        retval = 0.00f;
    }
    //DEBUG END

    return retval;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length_2( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    LOG_NAN( A2 );

    return A2;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_length( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    LOG_NAN( A2 );

    return SQRT( A2 );
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_add( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS )
{
    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == LHS && NULL == RHS )
    {
        fvec2_self_clear( DST );
    }
    else if ( NULL == LHS )
    {
        fvec3_base_copy( DST, RHS );

        LOG_NAN_FVEC3( DST );
    }
    else if ( NULL == RHS )
    {
        fvec3_base_copy( DST, LHS );

        LOG_NAN_FVEC3( DST );
    }
    else
    {
        DST[kX] = LHS[kX] + RHS[kX];
        DST[kY] = LHS[kY] + RHS[kY];
        DST[kZ] = LHS[kZ] + RHS[kZ];

        LOG_NAN_FVEC3( DST );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_sub( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS )
{
    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == LHS && NULL == RHS )
    {
        fvec2_self_clear( DST );
    }
    else if ( NULL == LHS )
    {
        DST[kX] = - RHS[kX];
        DST[kY] = - RHS[kY];
        DST[kZ] = - RHS[kZ];

        LOG_NAN_FVEC3( DST );
    }
    else if ( NULL == RHS )
    {
        DST[kX] = LHS[kX];
        DST[kY] = LHS[kY];
        DST[kZ] = LHS[kZ];

        LOG_NAN_FVEC3( DST );
    }
    else
    {
        DST[kX] = LHS[kX] - RHS[kX];
        DST[kY] = LHS[kY] - RHS[kY];
        DST[kZ] = LHS[kZ] - RHS[kZ];

        LOG_NAN_FVEC3( DST );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_scale( fvec3_base_t DST, const fvec3_base_t SRC, const float B )
{

    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == SRC )
    {
        fvec3_self_clear( DST );
    }
    else if ( 0.0f == B )
    {
        fvec3_self_clear( DST );
    }
    else
    {
        DST[kX] = SRC[kX] * B;
        DST[kY] = SRC[kY] * B;
        DST[kZ] = SRC[kZ] * B;

        LOG_NAN_FVEC3( DST );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_normalize( fvec3_base_t DST, const fvec3_base_t SRC )
{
    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == SRC )
    {
        fvec3_self_clear( DST );
    }
    else
    {
        float len2 = SRC[kX] * SRC[kX] + SRC[kY] * SRC[kY] + SRC[kZ] * SRC[kZ];

        if ( 0.0f == len2 )
        {
            fvec3_self_clear( DST );
        }
        else
        {
            float inv_len = 1.0f / SQRT( len2 );
            LOG_NAN( inv_len );

            DST[kX] = SRC[kX] * inv_len;
            DST[kY] = SRC[kY] * inv_len;
            DST[kZ] = SRC[kZ] * inv_len;

            LOG_NAN_FVEC3( DST );
        }
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_self_normalize( fvec3_base_t A )
{
    float len = -1.0f;

    if ( NULL == A ) return len;

    if ( 0.0f != fvec3_length_abs( A ) )
    {
        float len2, inv_len;

        len2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];
        len = SQRT( len2 );
        inv_len = 1.0f / len;

        LOG_NAN( inv_len );

        A[kX] *= inv_len;
        A[kY] *= inv_len;
        A[kZ] *= inv_len;
    }

    LOG_NAN_FVEC3( A );

    return len;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_self_normalize_to( fvec3_base_t vec, const float B )
{
    float len = -1.0f;

    if ( NULL == vec ) return len;

    if ( 0.0f == B )
    {
        fvec3_self_clear( vec );

        len = 0.0f;
    }
    else if ( 0.0f != fvec3_length_abs( vec ) )
    {
        float len2, inv_len;

        len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
        len = SQRT( len2 );
        inv_len = B / len;

        LOG_NAN( inv_len );

        vec[kX] *= inv_len;
        vec[kY] *= inv_len;
        vec[kZ] *= inv_len;
    }

    LOG_NAN_FVEC3( vec );

    return len;
}

//--------------------------------------------------------------------------------------------
static INLINE float * fvec3_cross_product( fvec3_base_t DST, const fvec3_base_t LHS, const fvec3_base_t RHS )
{
    if ( NULL == DST )
    {
        return NULL;
    }

    if ( NULL == LHS || NULL == RHS )
    {
        fvec3_self_clear( DST );
    }
    else
    {
        DST[kX] = LHS[kY] * RHS[kZ] - LHS[kZ] * RHS[kY];
        DST[kY] = LHS[kZ] * RHS[kX] - LHS[kX] * RHS[kZ];
        DST[kZ] = LHS[kX] * RHS[kY] - LHS[kY] * RHS[kX];

        LOG_NAN_FVEC3( DST );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_decompose( const fvec3_base_t A, const fvec3_base_t vnrm, fvec3_base_t vpara, fvec3_base_t vperp )
{
    /// @author BB
    /// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

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

            LOG_NAN_FVEC3( vperp );
        }
        else if ( NULL == vperp )
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;

            LOG_NAN_FVEC3( vpara );
        }
        else
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;

            vperp[kX] = A[kX];
            vperp[kY] = A[kY];
            vperp[kZ] = A[kZ];

            LOG_NAN_FVEC3( vperp );
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

            LOG_NAN_FVEC3( vperp );
        }
        else if ( NULL == vperp )
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];

            LOG_NAN_FVEC3( vpara );
        }
        else
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];

            LOG_NAN_FVEC3( vpara );

            vperp[kX] = A[kX] - vpara[kX];
            vperp[kY] = A[kY] - vpara[kY];
            vperp[kZ] = A[kZ] - vpara[kZ];

            LOG_NAN_FVEC3( vperp );
        }
    }

    return dot;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B )
{
    float retval;

    if ( NULL == A || NULL == B ) return 0.0f;

    retval = ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] ) + ABS( A[kZ] - B[kZ] );

    LOG_NAN( retval );

    return retval;
}

//--------------------------------------------------------------------------------------------
static INLINE float fvec3_dist_2( const fvec3_base_t LHS, const fvec3_base_t RHS )
{
    float retval = 0.0f, ftmp;

    if ( NULL == LHS || NULL == LHS ) return 0.0f;

    ftmp = LHS[kX] - RHS[kX];
    retval += ftmp * ftmp;

    ftmp = LHS[kY] - RHS[kY];
    retval += ftmp * ftmp;

    ftmp = LHS[kZ] - RHS[kZ];
    retval += ftmp * ftmp;

    LOG_NAN( retval );

    return retval;
}

//--------------------------------------------------------------------------------------------
static INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B )
{
    float retval;

    if ( NULL == A || NULL == B ) return 0.0f;

    retval = A[kX] * B[kX] + A[kY] * B[kY] + A[kZ] * B[kZ];

    LOG_NAN( retval );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool fvec4_valid( const fvec4_base_t A )
{
    int cnt;

    if ( NULL == A ) return false;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        if ( ieee32_bad( A[cnt] ) ) return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec4_self_clear( fvec4_base_t A )
{
    if ( NULL == A ) return false;

    A[kX] = A[kY] = A[kZ] = 0.0f;
    A[kW] = 1.0f;

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool fvec4_self_scale( fvec4_base_t A, const float B )
{
    if ( NULL == A ) return false;

    A[kX] *= B;
    A[kY] *= B;
    A[kZ] *= B;
    A[kW] *= B;

    LOG_NAN_FVEC4( A );

    return true;
}

//--------------------------------------------------------------------------------------------
// MATIX FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE float * mat_Copy( fmat_4x4_base_t DST, const fmat_4x4_base_t src )
{
    float * retval = NULL;

    if ( NULL == DST )
    {
        retval = NULL;
    }
    else if ( NULL == src )
    {
        retval = mat_Zero( DST );
    }
    else
    {
        retval = ( float * )memmove( DST, src, sizeof( fmat_4x4_base_t ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Identity( fmat_4x4_base_t DST )
{
    if ( NULL == DST ) return NULL;

    DST[MAT_IDX( 0, 0 )] = 1; DST[MAT_IDX( 1, 0 )] = 0; DST[MAT_IDX( 2, 0 )] = 0; DST[MAT_IDX( 3, 0 )] = 0;
    DST[MAT_IDX( 0, 1 )] = 0; DST[MAT_IDX( 1, 1 )] = 1; DST[MAT_IDX( 2, 1 )] = 0; DST[MAT_IDX( 3, 1 )] = 0;
    DST[MAT_IDX( 0, 2 )] = 0; DST[MAT_IDX( 1, 2 )] = 0; DST[MAT_IDX( 2, 2 )] = 1; DST[MAT_IDX( 3, 2 )] = 0;
    DST[MAT_IDX( 0, 3 )] = 0; DST[MAT_IDX( 1, 3 )] = 0; DST[MAT_IDX( 2, 3 )] = 0; DST[MAT_IDX( 3, 3 )] = 1;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Zero( fmat_4x4_base_t DST )
{
    // initializes matrix to zero

    if ( NULL == DST ) return NULL;

    DST[MAT_IDX( 0, 0 )] = 0; DST[MAT_IDX( 1, 0 )] = 0; DST[MAT_IDX( 2, 0 )] = 0; DST[MAT_IDX( 3, 0 )] = 0;
    DST[MAT_IDX( 0, 1 )] = 0; DST[MAT_IDX( 1, 1 )] = 0; DST[MAT_IDX( 2, 1 )] = 0; DST[MAT_IDX( 3, 1 )] = 0;
    DST[MAT_IDX( 0, 2 )] = 0; DST[MAT_IDX( 1, 2 )] = 0; DST[MAT_IDX( 2, 2 )] = 0; DST[MAT_IDX( 3, 2 )] = 0;
    DST[MAT_IDX( 0, 3 )] = 0; DST[MAT_IDX( 1, 3 )] = 0; DST[MAT_IDX( 2, 3 )] = 0; DST[MAT_IDX( 3, 3 )] = 0;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Multiply( fmat_4x4_base_t DST, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2 )
{
    int i, j, k;

    if ( NULL == mat_Zero( DST ) ) return NULL;

    //===============================================================================
    // the multiplocation code is the equivalent of the following function calls
    //    GLint matrix_mode[1];
    //    glGetIntegerv( GL_MATRIX_MODE, matrix_mode );
    //        glMatrixMode( GL_MODELVIEW );                   // or whatever matrix mode you want to use
    //        glPushMatrix( void );
    //            glLoadMatrixf( src1 );
    //            glMultMatrixf( src2 );
    //            glGetFloatv( GL_MODELVIEW_MATRIX, DST );    // use the correct argument here to grab the resultant matrix
    //        glPopMatrix( void );
    //    glMatrixMode( matrix_mode[0] );
    //===============================================================================

    for ( i = 0; i < 4; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            for ( k = 0; k < 4; k++ )
            {
                DST[MAT_IDX( i, j )] += src1[MAT_IDX( k, j )] * src2[MAT_IDX( i, k )];
            }
        }
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Translate( fmat_4x4_base_t DST, const float dx, const float dy, const float dz )
{
    if ( NULL == mat_Identity( DST ) ) return NULL;

    DST[MAT_IDX( 3, 0 )] = dx;
    DST[MAT_IDX( 3, 1 )] = dy;
    DST[MAT_IDX( 3, 2 )] = dz;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_RotateX( fmat_4x4_base_t DST, const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    if ( NULL == mat_Identity( DST ) ) return NULL;

    DST[MAT_IDX( 1, 1 )] = cosine;
    DST[MAT_IDX( 2, 2 )] = cosine;
    DST[MAT_IDX( 1, 2 )] = -sine;
    DST[MAT_IDX( 2, 1 )] = sine;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_RotateY( fmat_4x4_base_t DST, const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    if ( NULL == mat_Identity( DST ) ) return NULL;

    DST[MAT_IDX( 0, 0 )] = cosine;
    DST[MAT_IDX( 2, 2 )] = cosine;
    DST[MAT_IDX( 0, 2 )] = sine;
    DST[MAT_IDX( 2, 0 )] = -sine;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_RotateZ( fmat_4x4_base_t DST, const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    if ( NULL == mat_Identity( DST ) ) return NULL;

    DST[MAT_IDX( 0, 0 )] = cosine;
    DST[MAT_IDX( 1, 1 )] = cosine;
    DST[MAT_IDX( 0, 1 )] = -sine;
    DST[MAT_IDX( 1, 0 )] = sine;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_ScaleXYZ( fmat_4x4_base_t DST, const float sizex, const float sizey, const float sizez )
{
    if ( NULL == mat_Identity( DST ) ) return NULL;

    DST[MAT_IDX( 0, 0 )] = sizex;
    DST[MAT_IDX( 1, 1 )] = sizey;
    DST[MAT_IDX( 2, 2 )] = sizez;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed( fmat_4x4_base_t DST, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    if ( NULL == DST ) return NULL;

    DST[MAT_IDX( 0, 0 )] = scale_x * ( cz * cy );
    DST[MAT_IDX( 0, 1 )] = scale_x * ( cz * sy * sx + sz * cx );
    DST[MAT_IDX( 0, 2 )] = scale_x * ( sz * sx - cz * sy * cx );
    DST[MAT_IDX( 0, 3 )] = 0.0f;

    DST[MAT_IDX( 1, 0 )] = scale_y * ( -sz * cy );
    DST[MAT_IDX( 1, 1 )] = scale_y * ( -sz * sy * sx + cz * cx );
    DST[MAT_IDX( 1, 2 )] = scale_y * ( sz * sy * cx + cz * sx );
    DST[MAT_IDX( 1, 3 )] = 0.0f;

    DST[MAT_IDX( 2, 0 )] = scale_z * ( sy );
    DST[MAT_IDX( 2, 1 )] = scale_z * ( -cy * sx );
    DST[MAT_IDX( 2, 2 )] = scale_z * ( cy * cx );
    DST[MAT_IDX( 2, 3 )] = 0.0f;

    DST[MAT_IDX( 3, 0 )] = translate_x;
    DST[MAT_IDX( 3, 1 )] = translate_y;
    DST[MAT_IDX( 3, 2 )] = translate_z;
    DST[MAT_IDX( 3, 3 )] = 1.0f;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed( fmat_4x4_base_t DST, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    /// @author BB
    /// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    if ( NULL == DST ) return NULL;

    DST[MAT_IDX( 0, 0 )] = scale_x * ( cz * cy - sz * sy * sx );
    DST[MAT_IDX( 0, 1 )] = scale_x * ( sz * cy + cz * sy * sx );
    DST[MAT_IDX( 0, 2 )] = scale_x * ( -cx * sy );
    DST[MAT_IDX( 0, 3 )] = 0.0f;

    DST[MAT_IDX( 1, 0 )] = scale_y * ( -sz * cx );
    DST[MAT_IDX( 1, 1 )] = scale_y * ( cz * cx );
    DST[MAT_IDX( 1, 2 )] = scale_y * ( sx );
    DST[MAT_IDX( 1, 3 )] = 0.0f;

    DST[MAT_IDX( 2, 0 )] = scale_z * ( cz * sy + sz * sx * cy );
    DST[MAT_IDX( 2, 1 )] = scale_z * ( sz * sy - cz * sx * cy );
    DST[MAT_IDX( 2, 2 )] = scale_z * ( cy * cx );
    DST[MAT_IDX( 2, 3 )] = 0.0f;

    DST[MAT_IDX( 3, 0 )] = translate_x;
    DST[MAT_IDX( 3, 1 )] = translate_y;
    DST[MAT_IDX( 3, 2 )] = translate_z;
    DST[MAT_IDX( 3, 3 )] = 1.0f;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_FourPoints( fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t up, const float scale )
{
    fvec3_t vWid, vFor, vUp;

    if ( NULL == DST ) return NULL;

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

    DST[MAT_IDX( 0, 0 )] = -scale * vWid.x;  // HUK
    DST[MAT_IDX( 0, 1 )] = -scale * vWid.y;  // HUK
    DST[MAT_IDX( 0, 2 )] = -scale * vWid.z;  // HUK
    DST[MAT_IDX( 0, 3 )] = 0.0f;

    DST[MAT_IDX( 1, 0 )] = scale * vFor.x;
    DST[MAT_IDX( 1, 1 )] = scale * vFor.y;
    DST[MAT_IDX( 1, 2 )] = scale * vFor.z;
    DST[MAT_IDX( 1, 3 )] = 0.0f;

    DST[MAT_IDX( 2, 0 )] = scale * vUp.x;
    DST[MAT_IDX( 2, 1 )] = scale * vUp.y;
    DST[MAT_IDX( 2, 2 )] = scale * vUp.z;
    DST[MAT_IDX( 2, 3 )] = 0.0f;

    DST[MAT_IDX( 3, 0 )] = ori[kX];
    DST[MAT_IDX( 3, 1 )] = ori[kY];
    DST[MAT_IDX( 3, 2 )] = ori[kZ];
    DST[MAT_IDX( 3, 3 )] = 1.0f;

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_View( fmat_4x4_base_t DST,
                                const fvec3_base_t   from,     // camera location
                                const fvec3_base_t   at,        // camera look-at target
                                const fvec3_base_t   world_up,  // world’s up, usually 0, 0, 1
                                const float roll )         // clockwise roll around
//   viewing direction,
//   in radians
{
    /// @author MN
    /// @details This probably should be replaced by a call to gluLookAt(),
    ///          don't see why we need to make our own...

    fvec3_t up, right, view_dir, temp;

    if ( NULL == mat_Identity( DST ) ) return NULL;

    fvec3_normalize( view_dir.v, fvec3_sub( temp.v, at, from ) );
    fvec3_cross_product( right.v, world_up, view_dir.v );
    fvec3_cross_product( up.v, view_dir.v, right.v );
    fvec3_self_normalize( right.v );
    fvec3_self_normalize( up.v );

    DST[MAT_IDX( 0, 0 )] = right.x;
    DST[MAT_IDX( 1, 0 )] = right.y;
    DST[MAT_IDX( 2, 0 )] = right.z;

    DST[MAT_IDX( 0, 1 )] = up.x;
    DST[MAT_IDX( 1, 1 )] = up.y;
    DST[MAT_IDX( 2, 1 )] = up.z;

    DST[MAT_IDX( 0, 2 )] = view_dir.x;
    DST[MAT_IDX( 1, 2 )] = view_dir.y;
    DST[MAT_IDX( 2, 2 )] = view_dir.z;

    DST[MAT_IDX( 3, 0 )] = -fvec3_dot_product( right.v,    from );
    DST[MAT_IDX( 3, 1 )] = -fvec3_dot_product( up.v,       from );
    DST[MAT_IDX( 3, 2 )] = -fvec3_dot_product( view_dir.v, from );

    if ( roll != 0.0f )
    {
        // mat_Multiply function shown above
        fmat_4x4_t tmp1, tmp2;

        mat_Multiply( DST, mat_RotateZ( tmp1.v, -roll ), mat_Copy( tmp2.v, DST ) );
    }

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Projection(
    fmat_4x4_base_t DST,
    const float near_plane,    // distance to near clipping plane
    const float far_plane,     // distance to far clipping plane
    const float fov,           // vertical field of view angle, in radians
    const float ar )           // aspect ratio
{
    /// @author MN
    /// @details Again, there is a gl function for this, glFrustum or gluPerspective...
    ///          does this account for viewport ratio?

    float inv_h = 1 / TAN( fov * 0.5f );
    float inv_w = inv_h / ar;
    float Q = far_plane / ( far_plane - near_plane );

    if ( NULL == mat_Zero( DST ) ) return NULL;

    DST[MAT_IDX( 0, 0 )] = inv_h;         // 0,0
    DST[MAT_IDX( 1, 1 )] = inv_w;         // 1,1
    DST[MAT_IDX( 2, 2 )] = Q;         // 2,2
    DST[MAT_IDX( 3, 2 )] = -1.0f; // 3,2
    DST[MAT_IDX( 2, 3 )] = Q * near_plane;         // 2,3

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE float * mat_Projection_orig(
    fmat_4x4_base_t DST,
    const float near_plane,    // distance to near clipping plane
    const float far_plane,      // distance to far clipping plane
    const float fov )           // field of view angle, in radians
{
    /// @author MN
    /// @details Again, there is a gl function for this, glFrustum or gluPerspective...
    ///          does this account for viewport ratio?

    float c = COS( fov * 0.5f );
    float s = SIN( fov * 0.5f );
    float Q = far_plane / ( far_plane - near_plane );

    if ( NULL == mat_Zero( DST ) ) return NULL;

    DST[MAT_IDX( 0, 0 )] = c / s;       // 0,0
    DST[MAT_IDX( 1, 1 )] = c / s;       // 1,1
    DST[MAT_IDX( 2, 2 )] = Q;         // 2,2
    DST[MAT_IDX( 3, 2 )] = -Q * near_plane; // 3,2
    DST[MAT_IDX( 2, 3 )] = 1.0f;         // 2,3

    return DST;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getTranslate( const fmat_4x4_base_t mat, fvec3_base_t vpos )
{
    if ( NULL == mat || NULL == vpos ) return false;

    vpos[kX] = mat[MAT_IDX( 3, 0 )];
    vpos[kY] = mat[MAT_IDX( 3, 1 )];
    vpos[kZ] = mat[MAT_IDX( 3, 2 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getChrUp( const fmat_4x4_base_t mat, fvec3_base_t vup )
{
    if ( NULL == mat || NULL == vup ) return false;

    // for a character
    vup[kX] = mat[MAT_IDX( 2, 0 )];
    vup[kY] = mat[MAT_IDX( 2, 1 )];
    vup[kZ] = mat[MAT_IDX( 2, 2 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getChrForward( const fmat_4x4_base_t mat, fvec3_base_t vright )
{
    if ( NULL == mat || NULL == vright ) return false;

    // for a character
    vright[kX] = -mat[MAT_IDX( 0, 0 )];
    vright[kY] = -mat[MAT_IDX( 0, 1 )];
    vright[kZ] = -mat[MAT_IDX( 0, 2 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getChrRight( const fmat_4x4_base_t mat, fvec3_base_t vfrw )
{
    if ( NULL == mat || NULL == vfrw ) return false;

    // for a character's matrix
    vfrw[kX] = mat[MAT_IDX( 1, 0 )];
    vfrw[kY] = mat[MAT_IDX( 1, 1 )];
    vfrw[kZ] = mat[MAT_IDX( 1, 2 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getCamUp( const fmat_4x4_base_t mat, fvec3_base_t vup )
{
    if ( NULL == mat || NULL == vup ) return false;

    // for the camera
    vup[kX] = mat[MAT_IDX( 0, 1 )];
    vup[kY] = mat[MAT_IDX( 1, 1 )];
    vup[kZ] = mat[MAT_IDX( 2, 1 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getCamRight( const fmat_4x4_base_t mat, fvec3_base_t vright )
{
    if ( NULL == mat || NULL == vright ) return false;

    // for the camera
    vright[kX] = -mat[MAT_IDX( 0, 0 )];
    vright[kY] = -mat[MAT_IDX( 1, 0 )];
    vright[kZ] = -mat[MAT_IDX( 2, 0 )];

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE bool mat_getCamForward( const fmat_4x4_base_t mat, fvec3_base_t vfrw )
{
    if ( NULL == mat || NULL == vfrw ) return false;

    // for the camera
    vfrw[kX] = -mat[MAT_IDX( 0, 2 )];
    vfrw[kY] = -mat[MAT_IDX( 1, 2 )];
    vfrw[kZ] = -mat[MAT_IDX( 2, 2 )];

    return true;
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
//--------------------------------------------------------------------------------------------
static INLINE void  mat_TransformVertices( const fmat_4x4_base_t Matrix, const fvec4_t pSourceV[], fvec4_t pDestV[], const Uint32 NumVertor )
{
    /// @author GS
    /// @details This is just a MulVectorMatrix for now. The W division and screen size multiplication
    ///                must be done afterward.
    /// @author BB
    /// @details the matrix transformation for OpenGL vertices. Some minor optimizations.
    ///      The value pSourceV->w is assumed to be constant for all of the elements of pSourceV

    Uint32    cnt;
    fvec4_t * SourceIt = ( fvec4_t * )pSourceV;

    if ( 1.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * Matrix[0] + SourceIt->y * Matrix[4] + SourceIt->z * Matrix[8] + Matrix[12];
            pDestV->y = SourceIt->x * Matrix[1] + SourceIt->y * Matrix[5] + SourceIt->z * Matrix[9] + Matrix[13];
            pDestV->z = SourceIt->x * Matrix[2] + SourceIt->y * Matrix[6] + SourceIt->z * Matrix[10] + Matrix[14];
            pDestV->w = SourceIt->x * Matrix[3] + SourceIt->y * Matrix[7] + SourceIt->z * Matrix[11] + Matrix[15];

            pDestV++;
            SourceIt++;
        }
    }
    else if ( 0.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * Matrix[0] + SourceIt->y * Matrix[4] + SourceIt->z * Matrix[8];
            pDestV->y = SourceIt->x * Matrix[1] + SourceIt->y * Matrix[5] + SourceIt->z * Matrix[9];
            pDestV->z = SourceIt->x * Matrix[2] + SourceIt->y * Matrix[6] + SourceIt->z * Matrix[10];
            pDestV->w = SourceIt->x * Matrix[3] + SourceIt->y * Matrix[7] + SourceIt->z * Matrix[11];

            pDestV++;
            SourceIt++;
        }
    }
    else
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * Matrix[0] + SourceIt->y * Matrix[4] + SourceIt->z * Matrix[8]  + SourceIt->w * Matrix[12];
            pDestV->y = SourceIt->x * Matrix[1] + SourceIt->y * Matrix[5] + SourceIt->z * Matrix[9]  + SourceIt->w * Matrix[13];
            pDestV->z = SourceIt->x * Matrix[2] + SourceIt->y * Matrix[6] + SourceIt->z * Matrix[10] + SourceIt->w * Matrix[14];
            pDestV->w = SourceIt->x * Matrix[3] + SourceIt->y * Matrix[7] + SourceIt->z * Matrix[11] + SourceIt->w * Matrix[15];

            pDestV++;
            SourceIt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif
