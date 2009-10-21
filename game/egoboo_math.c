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

/// @file mathstuff.c
/// @brief The name's pretty self explanatory, doncha think?
/// @details

#include "egoboo_math.h"

#include <assert.h>
#include <float.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine
float turntocos[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to cosine

Uint32  randindex = 0;
Uint16  randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
void make_turntosin( void )
{
    /// @details ZZ@> This function makes the lookup table for chrturn...
    int cnt;
    float ftmp = TWO_PI / (float)TRIG_TABLE_SIZE;

    for ( cnt = 0; cnt < TRIG_TABLE_SIZE; cnt++ )
    {
        turntosin[cnt] = SIN( cnt * ftmp );
        turntocos[cnt] = COS( cnt * ftmp );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// FAKE D3D FUNCTIONS
fvec3_t fvec3_sub( fvec3_base_t A, fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];
    tmp.z = A[kZ] - B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
fvec3_t fvec3_normalize( fvec3_base_t vec )
{
    fvec3_t tmp = ZERO_VECT3;

    if ( ABS(vec[kX]) + ABS(vec[kY]) + ABS(vec[kZ]) > 0 )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        tmp.x = vec[kX] * inv_len;
        LOG_NAN(tmp.x);

        tmp.y = vec[kY] * inv_len;
        LOG_NAN(tmp.y);

        tmp.z = vec[kZ] * inv_len;
        LOG_NAN(tmp.z);
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
fvec3_t   fvec3_cross_product( fvec3_base_t A, fvec3_base_t B )
{
    fvec3_t   tmp;

    tmp.x = A[kY] * B[kZ] - A[kZ] * B[kY];
    tmp.y = A[kZ] * B[kX] - A[kX] * B[kZ];
    tmp.z = A[kX] * B[kY] - A[kY] * B[kX];

    return tmp;
}

//--------------------------------------------------------------------------------------------
float fvec3_dot_product( fvec3_base_t A, fvec3_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY] + A[kZ]*B[kZ];
}

//---------------------------------------------------------------------------------------------
// Math Stuff-----------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// inline D3DMATRIX IdentityMatrix()
fmat_4x4_t IdentityMatrix()
{
    fmat_4x4_t tmp;

    tmp.CNV( 0, 0 ) = 1; tmp.CNV( 1, 0 ) = 0; tmp.CNV( 2, 0 ) = 0; tmp.CNV( 3, 0 ) = 0;
    tmp.CNV( 0, 1 ) = 0; tmp.CNV( 1, 1 ) = 1; tmp.CNV( 2, 1 ) = 0; tmp.CNV( 3, 1 ) = 0;
    tmp.CNV( 0, 2 ) = 0; tmp.CNV( 1, 2 ) = 0; tmp.CNV( 2, 2 ) = 1; tmp.CNV( 3, 2 ) = 0;
    tmp.CNV( 0, 3 ) = 0; tmp.CNV( 1, 3 ) = 0; tmp.CNV( 2, 3 ) = 0; tmp.CNV( 3, 3 ) = 1;

    return( tmp );
}

//--------------------------------------------------------------------------------------------
// inline D3DMATRIX ZeroMatrix(void)  // initializes matrix to zero
fmat_4x4_t ZeroMatrix( void )
{
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
// inline D3DMATRIX MatrixMult(const D3DMATRIX a, const D3DMATRIX b)
fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b )
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
// D3DMATRIX Translate(const float dx, const float dy, const float dz)
fmat_4x4_t Translate( const float dx, const float dy, const float dz )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 3, 0 ) = dx;
    ret.CNV( 3, 1 ) = dy;
    ret.CNV( 3, 2 ) = dz;

    return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX RotateX(const float rads)
fmat_4x4_t RotateX( const float rads )
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
// D3DMATRIX RotateY(const float rads)
fmat_4x4_t RotateY( const float rads )
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
// D3DMATRIX RotateZ(const float rads)
fmat_4x4_t RotateZ( const float rads )
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
// D3DMATRIX ScaleXYZ(const float sizex, const float sizey, const float sizez)
fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = sizex; // 0,0
    ret.CNV( 1, 1 ) = sizey; // 1,1
    ret.CNV( 2, 2 ) = sizez; // 2,2

    return ret;
}

//--------------------------------------------------------------------------------------------
/*D3DMATRIX ScaleXYZRotateXYZTranslate(const float sizex, const float sizey, const float sizez,
   Uint16 turnz, Uint16 turnx, Uint16 turny,
   float tx, float ty, float tz)*/
fmat_4x4_t ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float tx, const float ty, const float tz )
{
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

    fmat_4x4_t ret;

    ret.CNV( 0, 0 ) = sizex * ( cy * cz );
    ret.CNV( 0, 1 ) = sizex * ( sxsy * cz + cx * sz );
    ret.CNV( 0, 2 ) = sizex * ( -cxsy * cz + sx * sz );
    ret.CNV( 0, 3 ) = 0;

    ret.CNV( 1, 0 ) = sizey * ( -cy * sz );
    ret.CNV( 1, 1 ) = sizey * ( -sxsy * sz + cx * cz );
    ret.CNV( 1, 2 ) = sizey * ( cxsy * sz + sx * cz );
    ret.CNV( 1, 3 ) = 0;

    ret.CNV( 2, 0 ) = sizez * ( sy );
    ret.CNV( 2, 1 ) = sizez * ( -sxcy );
    ret.CNV( 2, 2 ) = sizez * ( cxcy );
    ret.CNV( 2, 3 ) = 0;

    ret.CNV( 3, 0 ) = tx;
    ret.CNV( 3, 1 ) = ty;
    ret.CNV( 3, 2 ) = tz;
    ret.CNV( 3, 3 ) = 1;

    return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX FourPoints(float orix, float oriy, float oriz,
fmat_4x4_t FourPoints( fvec4_base_t ori, fvec4_base_t wid,
                       fvec4_base_t frw, fvec4_base_t up, float scale )
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

    vWid = fvec3_normalize(vWid.v);
    vUp  = fvec3_normalize(vUp.v );
    vFor = fvec3_normalize(vFor.v);

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
// MN This probably should be replaced by a call to gluLookAt, don't see why we need to make our own...

// inline D3DMATRIX ViewMatrix(const D3DVECTOR from,      // camera location
fmat_4x4_t ViewMatrix( const fvec3_base_t   from,     // camera location
                       const fvec3_base_t   at,        // camera look-at target
                       const fvec3_base_t   world_up,  // world’s up, usually 0, 0, 1
                       const float roll )         // clockwise roll around
//   viewing direction,
//   in radians
{
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
// MN Again, there is a gl function for this, glFrustum or gluPerspective... does this account for viewport ratio?

// inline D3DMATRIX ProjectionMatrix(const float near_plane,     // distance to near clipping plane
fmat_4x4_t ProjectionMatrix( const float near_plane,    // distance to near clipping plane
                           const float far_plane,      // distance to far clipping plane
                           const float fov )           // field of view angle, in radians
{
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
// GS - Normally we souldn't this function but I found it in the rendering of the particules.

// This is just a MulVectorMatrix for now. The W division and screen size multiplication
// must be done afterward.
// Isn't tested!!!!
void  TransformVertices( fmat_4x4_t *pMatrix, fvec4_t   *pSourceV, fvec4_t   *pDestV, Uint32  NumVertor )
{
    /// @details BB@> the matrix transformation for OpenGL vertices. some minor optimizations.

    if ( 1.0f == pSourceV->w )
    {
        while ( NumVertor-- )
        {
            pDestV->x = pSourceV->x * pMatrix->v[0] + pSourceV->y * pMatrix->v[4] + pSourceV->z * pMatrix->v[ 8] + pMatrix->v[12];
            pDestV->y = pSourceV->x * pMatrix->v[1] + pSourceV->y * pMatrix->v[5] + pSourceV->z * pMatrix->v[ 9] + pMatrix->v[13];
            pDestV->z = pSourceV->x * pMatrix->v[2] + pSourceV->y * pMatrix->v[6] + pSourceV->z * pMatrix->v[10] + pMatrix->v[14];
            pDestV->w = pSourceV->x * pMatrix->v[3] + pSourceV->y * pMatrix->v[7] + pSourceV->z * pMatrix->v[11] + pMatrix->v[15];

            pDestV++;
            pSourceV++;
        }
    }
    else if ( 0.0f == pSourceV->w )
    {
        while ( NumVertor-- )
        {
            pDestV->x = pSourceV->x * pMatrix->v[0] + pSourceV->y * pMatrix->v[4] + pSourceV->z * pMatrix->v[ 8];
            pDestV->y = pSourceV->x * pMatrix->v[1] + pSourceV->y * pMatrix->v[5] + pSourceV->z * pMatrix->v[ 9];
            pDestV->z = pSourceV->x * pMatrix->v[2] + pSourceV->y * pMatrix->v[6] + pSourceV->z * pMatrix->v[10];
            pDestV->w = pSourceV->x * pMatrix->v[3] + pSourceV->y * pMatrix->v[7] + pSourceV->z * pMatrix->v[11];

            pDestV++;
            pSourceV++;
        }
    }
    else
    {
        while ( NumVertor-- )
        {
            pDestV->x = pSourceV->x * pMatrix->v[0] + pSourceV->y * pMatrix->v[4] + pSourceV->z * pMatrix->v[8]  + pSourceV->w * pMatrix->v[12];
            pDestV->y = pSourceV->x * pMatrix->v[1] + pSourceV->y * pMatrix->v[5] + pSourceV->z * pMatrix->v[9]  + pSourceV->w * pMatrix->v[13];
            pDestV->z = pSourceV->x * pMatrix->v[2] + pSourceV->y * pMatrix->v[6] + pSourceV->z * pMatrix->v[10] + pSourceV->w * pMatrix->v[14];
            pDestV->w = pSourceV->x * pMatrix->v[3] + pSourceV->y * pMatrix->v[7] + pSourceV->z * pMatrix->v[11] + pSourceV->w * pMatrix->v[15];

            pDestV++;
            pSourceV++;
        }
    }
}

//----------------------------------------------------
fvec3_t   mat_getTranslate(fmat_4x4_t mat)
{
    fvec3_t   pos;

    pos.x = mat.CNV( 3, 0 );
    pos.y = mat.CNV( 3, 1 );
    pos.z = mat.CNV( 3, 2 );

    return pos;
}

//----------------------------------------------------
fvec3_t   mat_getChrUp(fmat_4x4_t mat)
{
    fvec3_t   up;

    // for a character
    up.x = mat.CNV( 2, 0 );
    up.y = mat.CNV( 2, 1 );
    up.z = mat.CNV( 2, 2 );

    return up;
}

//----------------------------------------------------
fvec3_t   mat_getChrRight(fmat_4x4_t mat)
{
    fvec3_t   right;

    // for a character
    right.x = -mat.CNV( 0, 0 );
    right.y = -mat.CNV( 0, 1 );
    right.z = -mat.CNV( 0, 2 );

    return right;
}

//----------------------------------------------------
fvec3_t   mat_getChrForward(fmat_4x4_t mat)
{
    fvec3_t   frw;

    // for a character's matrix
    frw.x = mat.CNV( 1, 0 );
    frw.y = mat.CNV( 1, 1 );
    frw.z = mat.CNV( 1, 2 );

    return frw;
}

//----------------------------------------------------
fvec3_t   mat_getCamUp(fmat_4x4_t mat)
{
    fvec3_t   up;

    // for the camera
    up.x = -mat.CNV( 0, 1 );
    up.y = -mat.CNV( 1, 1 );
    up.z = -mat.CNV( 2, 1 );

    return up;
}

//----------------------------------------------------
fvec3_t   mat_getCamRight(fmat_4x4_t mat)
{
    fvec3_t   right;

    // for the camera
    right.x = mat.CNV( 0, 0 );
    right.y = mat.CNV( 1, 0 );
    right.z = mat.CNV( 2, 0 );

    return right;
}

//----------------------------------------------------
fvec3_t   mat_getCamForward(fmat_4x4_t mat)
{
    fvec3_t   frw;

    // for the camera
    frw.x = mat.CNV( 0, 2 );
    frw.y = mat.CNV( 1, 2 );
    frw.z = mat.CNV( 2, 2 );

    return frw;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int generate_irand_pair( IPair num )
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
int generate_irand_range( FRange num )
{
    /// @details ZZ@> This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
int generate_randmask( int base, int mask )
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
void make_randie()
{
    /// @details ZZ@> This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    for ( cnt = 0; cnt < RANDIE_COUNT; cnt++  )
    {
        randie[cnt] = 0;
    }

    // Keep adjusting those values
    for ( tnc = 0; tnc < 20; tnc++ )
    {
        for ( cnt = 0; cnt < RANDIE_COUNT; cnt++ )
        {
            randie[cnt] = ( randie[cnt] << 1 ) + rand();
        }
    }

    // All done
    randindex = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 vec_to_facing( float dx, float dy )
{
    return ( ATAN2( dy, dx ) + PI ) * RAD_TO_TURN;
}

//--------------------------------------------------------------------------------------------
void facing_to_vec( Uint16 facing, float * dx, float * dy )
{
    Uint16 turn = ( facing - 0x8000 ) >> 2;

    if( NULL != dx )
    {
        *dx = turntocos[turn & TRIG_TABLE_MASK];
    }

    if( NULL != dy )
    {
        *dy = turntosin[turn & TRIG_TABLE_MASK];
    }
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fvec2_clear( fvec2_t * A )
{
    if( NULL == A ) return bfalse;

    (*A).x = (*A).y = 0.0f;

    return btrue;
}


//--------------------------------------------------------------------------------------------
bool_t fvec3_clear( fvec3_t * A )
{
    if( NULL == A ) return bfalse;

    (*A).x = (*A).y = (*A).z = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fvec4_clear( fvec4_t * A )
{
    if( NULL == A ) return bfalse;

    (*A).x = (*A).y = (*A).z = 0.0f;
    (*A).w = 1.0f;

    return btrue;
}