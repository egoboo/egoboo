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
#include "egolib/vec.h"
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



//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif



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

    register float newvalue;

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
