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

#include <math.h>
#include <float.h>

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

// conversion functions
INLINE FACING_T vec_to_facing( float dx, float dy );
INLINE void     facing_to_vec( FACING_T facing, float * dx, float * dy );

// limiting functions
INLINE void getadd( int min, int value, int max, int* valuetoadd );
INLINE void fgetadd( float min, float value, float max, float* valuetoadd );

// random functions
INLINE int generate_irand_pair( IPair num );
INLINE int generate_irand_range( FRange num );
INLINE int generate_randmask( int base, int mask );

// vector functions
INLINE bool_t fvec2_clear( fvec2_t * A );
INLINE bool_t fvec3_clear( fvec3_t * A );
INLINE bool_t fvec4_clear( fvec4_t * A );
INLINE bool_t fvec3_scale( fvec3_t * A, const float B );

INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B );
INLINE fvec3_t fvec3_normalize( const fvec3_base_t A );
INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B );
INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B );

// matrix functions
INLINE fmat_4x4_t IdentityMatrix( void );
INLINE fmat_4x4_t ZeroMatrix( void );
INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b );
INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz );
INLINE fmat_4x4_t RotateX( const float rads );
INLINE fmat_4x4_t RotateY( const float rads );
INLINE fmat_4x4_t RotateZ( const float rads );
INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez );
INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z );
INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale );
INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll );
INLINE fmat_4x4_t ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
INLINE void       TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor );

INLINE fvec3_t   mat_getChrUp( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getChrRight( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getChrForward( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamUp( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamRight( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamForward( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getTranslate( const fmat_4x4_t mat );

//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE FACING_T vec_to_facing( float dx, float dy )
{
    return ( ATAN2( dy, dx ) + PI ) * RAD_TO_TURN;
}

//--------------------------------------------------------------------------------------------
INLINE void facing_to_vec( FACING_T facing, float * dx, float * dy )
{
    Uint16 turn = ( facing - 0x8000 ) >> 2;

    if ( NULL != dx )
    {
        *dx = turntocos[turn & TRIG_TABLE_MASK];
    }

    if ( NULL != dy )
    {
        *dy = turntosin[turn & TRIG_TABLE_MASK];
    }
}

//--------------------------------------------------------------------------------------------
// LIMITING FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE void getadd( int min, int value, int max, int* valuetoadd )
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
INLINE void fgetadd( float min, float value, float max, float* valuetoadd )
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
INLINE int generate_irand_pair( IPair num )
{
    /// @details ZZ@> This function generates a random number

    int tmp = 0;
    int irand = RANDIE;

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE int generate_irand_range( FRange num )
{
    /// @details ZZ@> This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
INLINE int generate_randmask( int base, int mask )
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
INLINE bool_t fvec2_clear( fvec2_t * A )
{
    if ( NULL == A ) return bfalse;

    ( *A ).x = ( *A ).y = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_clear( fvec3_t * A )
{
    if ( NULL == A ) return bfalse;

    ( *A ).x = ( *A ).y = ( *A ).z = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec4_clear( fvec4_t * A )
{
    if ( NULL == A ) return bfalse;

    ( *A ).x = ( *A ).y = ( *A ).z = 0.0f;
    ( *A ).w = 1.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_scale( fvec3_t * A, const float B )
{
    if ( NULL == A ) return bfalse;

    ( *A ).x /= B;
    ( *A ).y /= B;
    ( *A ).z /= B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];
    tmp.z = A[kZ] - B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_normalize( const fvec3_base_t vec )
{
    fvec3_t tmp = ZERO_VECT3;

    if ( ABS( vec[kX] ) + ABS( vec[kY] ) + ABS( vec[kZ] ) > 0 )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        tmp.x = vec[kX] * inv_len;
        LOG_NAN( tmp.x );

        tmp.y = vec[kY] * inv_len;
        LOG_NAN( tmp.y );

        tmp.z = vec[kZ] * inv_len;
        LOG_NAN( tmp.z );
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t   tmp;

    tmp.x = A[kY] * B[kZ] - A[kZ] * B[kY];
    tmp.y = A[kZ] * B[kX] - A[kX] * B[kZ];
    tmp.z = A[kX] * B[kY] - A[kY] * B[kX];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY] + A[kZ]*B[kZ];
}
//--------------------------------------------------------------------------------------------
// MATIX FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getTranslate( const fmat_4x4_t mat )
{
    fvec3_t   pos;

    pos.x = mat.CNV( 3, 0 );
    pos.y = mat.CNV( 3, 1 );
    pos.z = mat.CNV( 3, 2 );

    return pos;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrUp( const fmat_4x4_t mat )
{
    fvec3_t   up;

    // for a character
    up.x = mat.CNV( 2, 0 );
    up.y = mat.CNV( 2, 1 );
    up.z = mat.CNV( 2, 2 );

    return up;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrRight( const fmat_4x4_t mat )
{
    fvec3_t   right;

    // for a character
    right.x = -mat.CNV( 0, 0 );
    right.y = -mat.CNV( 0, 1 );
    right.z = -mat.CNV( 0, 2 );

    return right;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrForward( const fmat_4x4_t mat )
{
    fvec3_t   frw;

    // for a character's matrix
    frw.x = mat.CNV( 1, 0 );
    frw.y = mat.CNV( 1, 1 );
    frw.z = mat.CNV( 1, 2 );

    return frw;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamUp( const fmat_4x4_t mat )
{
    fvec3_t   up;

    // for the camera
    up.x = -mat.CNV( 0, 1 );
    up.y = -mat.CNV( 1, 1 );
    up.z = -mat.CNV( 2, 1 );

    return up;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamRight( const fmat_4x4_t mat )
{
    fvec3_t   right;

    // for the camera
    right.x = mat.CNV( 0, 0 );
    right.y = mat.CNV( 1, 0 );
    right.z = mat.CNV( 2, 0 );

    return right;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamForward( const fmat_4x4_t mat )
{
    fvec3_t   frw;

    // for the camera
    frw.x = mat.CNV( 0, 2 );
    frw.y = mat.CNV( 1, 2 );
    frw.z = mat.CNV( 2, 2 );

    return frw;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t IdentityMatrix()
{
    fmat_4x4_t tmp;

    tmp.CNV( 0, 0 ) = 1; tmp.CNV( 1, 0 ) = 0; tmp.CNV( 2, 0 ) = 0; tmp.CNV( 3, 0 ) = 0;
    tmp.CNV( 0, 1 ) = 0; tmp.CNV( 1, 1 ) = 1; tmp.CNV( 2, 1 ) = 0; tmp.CNV( 3, 1 ) = 0;
    tmp.CNV( 0, 2 ) = 0; tmp.CNV( 1, 2 ) = 0; tmp.CNV( 2, 2 ) = 1; tmp.CNV( 3, 2 ) = 0;
    tmp.CNV( 0, 3 ) = 0; tmp.CNV( 1, 3 ) = 0; tmp.CNV( 2, 3 ) = 0; tmp.CNV( 3, 3 ) = 1;

    return( tmp );
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ZeroMatrix( void )
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
INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b )
{
    fmat_4x4_t ret = ZeroMatrix();
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
INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 3, 0 ) = dx;
    ret.CNV( 3, 1 ) = dy;
    ret.CNV( 3, 2 ) = dz;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t RotateX( const float rads )
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
INLINE fmat_4x4_t RotateY( const float rads )
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
INLINE fmat_4x4_t RotateZ( const float rads )
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
INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = sizex; // 0,0
    ret.CNV( 1, 1 ) = sizey; // 1,1
    ret.CNV( 2, 2 ) = sizez; // 2,2

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    fmat_4x4_t ret;

    float cx = turntocos[ turn_x & TRIG_TABLE_MASK ];
    float sx = turntosin[ turn_x & TRIG_TABLE_MASK ];
    float cy = turntocos[ turn_y & TRIG_TABLE_MASK ];
    float sy = turntosin[ turn_y & TRIG_TABLE_MASK ];
    float cz = turntocos[ turn_z & TRIG_TABLE_MASK ];
    float sz = turntosin[ turn_z & TRIG_TABLE_MASK ];

    float sxsy = sx * sy;
    float cxsy = cx * sy;
    float sxcy = sx * cy;
    float cxcy = cx * cy;

    ret.CNV( 0, 0 ) = scale_x * ( cy * cz );
    ret.CNV( 0, 1 ) = scale_x * ( sxsy * cz + cx * sz );
    ret.CNV( 0, 2 ) = scale_x * ( -cxsy * cz + sx * sz );
    ret.CNV( 0, 3 ) = 0.0f;

    ret.CNV( 1, 0 ) = scale_y * ( -cy * sz );
    ret.CNV( 1, 1 ) = scale_y * ( -sxsy * sz + cx * cz );
    ret.CNV( 1, 2 ) = scale_y * ( cxsy * sz + sx * cz );
    ret.CNV( 1, 3 ) = 0.0f;

    ret.CNV( 2, 0 ) = scale_z * ( sy );
    ret.CNV( 2, 1 ) = scale_z * ( -sxcy );
    ret.CNV( 2, 2 ) = scale_z * ( cxcy );
    ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 3, 0 ) = translate_x;
    ret.CNV( 3, 1 ) = translate_y;
    ret.CNV( 3, 2 ) = translate_z;
    ret.CNV( 3, 3 ) = 1.0f;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t up, const float scale )
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

    vWid = fvec3_normalize( vWid.v );
    vUp  = fvec3_normalize( vUp.v );
    vFor = fvec3_normalize( vFor.v );

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
INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from,     // camera location
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
    right    = fvec3_normalize( right.v );
    up       = fvec3_normalize( up.v );

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
INLINE fmat_4x4_t ProjectionMatrix( const float near_plane,    // distance to near clipping plane
                                    const float far_plane,      // distance to far clipping plane
                                    const float fov )           // field of view angle, in radians
{
    /// @details MN@> Again, there is a gl function for this, glFrustum or gluPerspective...
    ///               does this account for viewport ratio?

    float c = COS( fov * 0.5f );
    float s = SIN( fov * 0.5f );
    float Q = s / ( 1.0f - near_plane / far_plane );

    fmat_4x4_t ret = ZeroMatrix();

    ret.CNV( 0, 0 ) = c;         // 0,0
    ret.CNV( 1, 1 ) = c;         // 1,1
    ret.CNV( 2, 2 ) = Q;         // 2,2
    ret.CNV( 3, 2 ) = -Q * near_plane; // 3,2
    ret.CNV( 2, 3 ) = s;         // 2,3

    return ret;
}

//----------------------------------------------------
INLINE void  TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor )
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
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[ 8] + pMatrix->v[12];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[ 9] + pMatrix->v[13];
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
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[ 8];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[ 9];
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