/* Egoboo - mathstuff.c
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

/**> HEADER FILES <**/
#include "egoboo_math.h"
#include "mesh.h"
//#include <SDL_opengl.h>

float turntosin[TRIGTABLE_SIZE];

//---------------------------------------------------------------------------------------------
void turn_to_vec( Uint16 turn, float * dx, float * dy )
{
  float rad = TURN_TO_RAD( turn );

  *dx = cos( rad );
  *dy = sin( rad );
};

//---------------------------------------------------------------------------------------------
Uint16 vec_to_turn( float dx, float dy )
{
  return RAD_TO_TURN( atan2( dy, dx ) );
};

// FAKE D3D FUNCTIONS
//---------------------------------------------------------------------------------------------
vect3 VSub( vect3 A, vect3 B )
{
  vect3 tmp;
  tmp.x = A.x - B.x; tmp.y = A.y - B.y; tmp.z = A.z - B.z;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
vect3 Normalize( vect3 vec )
{
  vect3 tmp = vec;
  float len;

  len = ( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );
  if ( len == 0.0f )
  {
    tmp.x = tmp.y = tmp.z = 0.0f;
  }
  else
  {
    len = sqrt( len );
    tmp.x /= len;
    tmp.y /= len;
    tmp.z /= len;
  };

  return ( tmp );
}

//---------------------------------------------------------------------------------------------
vect3 CrossProduct( vect3 A, vect3 B )
{
  vect3 tmp;
  tmp.x = A.y * B.z - A.z * B.y;
  tmp.y = A.z * B.x - A.x * B.z;
  tmp.z = A.x * B.y - A.y * B.x;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
float DotProduct( vect3 A, vect3 B )
{ return ( A.x*B.x + A.y*B.y + A.z*B.z ); }

//---------------------------------------------------------------------------------------------
vect4 GLVSub( vect4 A, vect4 B )
{
  vect4 tmp;
  tmp.x = A.x - B.x; tmp.y = A.y - B.y; tmp.z = A.z - B.z;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
vect4 GLNormalize( vect4 vec )
{
  vect4 tmp = vec;
  float len;

  len = ( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );
  if ( len == 0.0f )
  {
    tmp.x = tmp.y = tmp.z = 0.0f;
  }
  else
  {
    len = sqrt( len );
    tmp.x /= len;
    tmp.y /= len;
    tmp.z /= len;
  };

  return ( tmp );
}

//---------------------------------------------------------------------------------------------
vect4 GLCrossProduct( vect4 A, vect4 B )
{
  vect4 tmp;
  tmp.x = A.y * B.z - A.z * B.y;
  tmp.y = A.z * B.x - A.x * B.z;
  tmp.z = A.x * B.y - A.y * B.x;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
float GLDotProduct( vect4 A, vect4 B )
{ return ( A.x*B.x + A.y*B.y + A.z*B.z ); }

//---------------------------------------------------------------------------------------------
//Math Stuff-----------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//inline D3DMATRIX IdentityMatrix()
matrix_4x4 IdentityMatrix()
{
  matrix_4x4 tmp;

  ( tmp ) _CNV( 0, 0 ) = 1; ( tmp ) _CNV( 1, 0 ) = 0; ( tmp ) _CNV( 2, 0 ) = 0; ( tmp ) _CNV( 3, 0 ) = 0;
  ( tmp ) _CNV( 0, 1 ) = 0; ( tmp ) _CNV( 1, 1 ) = 1; ( tmp ) _CNV( 2, 1 ) = 0; ( tmp ) _CNV( 3, 1 ) = 0;
  ( tmp ) _CNV( 0, 2 ) = 0; ( tmp ) _CNV( 1, 2 ) = 0; ( tmp ) _CNV( 2, 2 ) = 1; ( tmp ) _CNV( 3, 2 ) = 0;
  ( tmp ) _CNV( 0, 3 ) = 0; ( tmp ) _CNV( 1, 3 ) = 0; ( tmp ) _CNV( 2, 3 ) = 0; ( tmp ) _CNV( 3, 3 ) = 1;
  return ( tmp );
}

//--------------------------------------------------------------------------------------------
//inline D3DMATRIX ZeroMatrix(void)  // initializes matrix to zero
matrix_4x4 ZeroMatrix( void )
{
  matrix_4x4 ret;
  int i, j;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
      ( ret ) _CNV( i, j ) = 0;

  return ret;
}

//--------------------------------------------------------------------------------------------
//inline D3DMATRIX MatrixMult(const D3DMATRIX a, const D3DMATRIX b)
matrix_4x4 MatrixTranspose( const matrix_4x4 a )
{
  matrix_4x4 ret;
  int i, j;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
      ( ret ) _CNV( i, j ) = ( a ) _CNV( j, i );

  return ret;
}

//--------------------------------------------------------------------------------------------
//inline D3DMATRIX MatrixMult(const D3DMATRIX a, const D3DMATRIX b)
matrix_4x4 MatrixMult( const matrix_4x4 a, const matrix_4x4 b )
{
  matrix_4x4 ret;
  int i, j, k;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
    {
      ( ret ) _CNV( i, j ) = 0.0f;
      for ( k = 0; k < 4; k++ )
        ( ret ) _CNV( i, j ) += ( a ) _CNV( k, j ) * ( b ) _CNV( i, k );
    };

  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX Translate(const float dx, const float dy, const float dz)
matrix_4x4 Translate( const float dx, const float dy, const float dz )
{
  matrix_4x4 ret = IdentityMatrix();
  ( ret ) _CNV( 3, 0 ) = dx;
  ( ret ) _CNV( 3, 1 ) = dy;
  ( ret ) _CNV( 3, 2 ) = dz;
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateX(const float rads)
matrix_4x4 RotateX( const float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );
  matrix_4x4 ret = IdentityMatrix();
  ( ret ) _CNV( 1, 1 ) =  cosine;
  ( ret ) _CNV( 2, 2 ) =  cosine;
  ( ret ) _CNV( 1, 2 ) = -sine;
  ( ret ) _CNV( 2, 1 ) =  sine;
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateY(const float rads)
matrix_4x4 RotateY( const float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );
  matrix_4x4 ret = IdentityMatrix();
  ( ret ) _CNV( 0, 0 ) =  cosine;  //0,0
  ( ret ) _CNV( 2, 2 ) =  cosine;  //2,2
  ( ret ) _CNV( 0, 2 ) =  sine;    //0,2
  ( ret ) _CNV( 2, 0 ) = -sine;    //2,0
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateZ(const float rads)
matrix_4x4 RotateZ( const float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );
  matrix_4x4 ret = IdentityMatrix();
  ( ret ) _CNV( 0, 0 ) =  cosine;  //0,0
  ( ret ) _CNV( 1, 1 ) =  cosine;  //1,1
  ( ret ) _CNV( 0, 1 ) = -sine;    //0,1
  ( ret ) _CNV( 1, 0 ) =  sine;    //1,0
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX ScaleXYZ(const float sizex, const float sizey, const float sizez)
matrix_4x4 ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
  matrix_4x4 ret = IdentityMatrix();
  ( ret ) _CNV( 0, 0 ) = sizex;   //0,0
  ( ret ) _CNV( 1, 1 ) = sizey;   //1,1
  ( ret ) _CNV( 2, 2 ) = sizez;   //2,2
  return ret;
}

//--------------------------------------------------------------------------------------------
matrix_4x4 ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz )
{
  float cx = turntosin[( turnx+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
  float sx = turntosin[turnx & TRIGTABLE_MASK];
  float cy = turntosin[( turny+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
  float sy = turntosin[turny & TRIGTABLE_MASK];
  float cz = turntosin[( turnz+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
  float sz = turntosin[turnz & TRIGTABLE_MASK];
  float sxsy = sx * sy;
  float cxsy = cx * sy;
  float sxcy = sx * cy;
  float cxcy = cx * cy;
  matrix_4x4 ret;

  ( ret ) _CNV( 0, 0 ) = sizex * ( cy * cz );    //0,0
  ( ret ) _CNV( 0, 1 ) = sizex * ( sxsy * cz + cx * sz );  //0,1
  ( ret ) _CNV( 0, 2 ) = sizex * ( -cxsy * cz + sx * sz );  //0,2
  ( ret ) _CNV( 0, 3 ) = 0;      //0,3

  ( ret ) _CNV( 1, 0 ) = sizey * ( -cy * sz );    //1,0
  ( ret ) _CNV( 1, 1 ) = sizey * ( -sxsy * sz + cx * cz );  //1,1
  ( ret ) _CNV( 1, 2 ) = sizey * ( cxsy * sz + sx * cz );  //1,2
  ( ret ) _CNV( 1, 3 ) = 0;      //1,3

  ( ret ) _CNV( 2, 0 ) = sizez * ( sy );  //2,0
  ( ret ) _CNV( 2, 1 ) = sizez * ( -sxcy );    //2,1
  ( ret ) _CNV( 2, 2 ) = sizez * ( cxcy );    //2,2
  ( ret ) _CNV( 2, 3 ) = 0;      //2,3

  ( ret ) _CNV( 3, 0 ) = tx;      //3,0
  ( ret ) _CNV( 3, 1 ) = ty;      //3,1
  ( ret ) _CNV( 3, 2 ) = tz;      //3,2
  ( ret ) _CNV( 3, 3 ) = 1;      //3,3
  return ret;
}

//--------------------------------------------------------------------------------------------
matrix_4x4 FourPoints( vect4 ori, vect4 wid, vect4 frw, vect4 up, float scale )
{
  matrix_4x4 tmp;

  wid.x -= ori.x;  frw.x -= ori.x;  up.x -= ori.x;
  wid.y -= ori.y;  frw.y -= ori.y;  up.y -= ori.y;
  wid.z -= ori.z;  frw.z -= ori.z;  up.z -= ori.z;

  // fix -x scaling on the input
  //wid.x *= -1.0; // HUK
  //wid.y *= -1.0; // HUK
  //wid.z *= -1.0; // HUK

  wid = GLNormalize( wid );
  frw = GLNormalize( frw );
  up  = GLNormalize( up );

  ( tmp ) _CNV( 0, 0 ) = scale * wid.x;       //0,0
  ( tmp ) _CNV( 0, 1 ) = scale * wid.y;       //0,1
  ( tmp ) _CNV( 0, 2 ) = scale * wid.z;       //0,2
  ( tmp ) _CNV( 0, 3 ) = 0;  //0,3

  ( tmp ) _CNV( 1, 0 ) = scale * frw.x;       //1,0
  ( tmp ) _CNV( 1, 1 ) = scale * frw.y;       //1,1
  ( tmp ) _CNV( 1, 2 ) = scale * frw.z;       //1,2
  ( tmp ) _CNV( 1, 3 ) = 0;  //1,3

  ( tmp ) _CNV( 2, 0 ) = scale * up.x;       //2,0
  ( tmp ) _CNV( 2, 1 ) = scale * up.y;       //2,1
  ( tmp ) _CNV( 2, 2 ) = scale * up.z;       //2,2
  ( tmp ) _CNV( 2, 3 ) = 0;       //2,3

  ( tmp ) _CNV( 3, 0 ) = ori.x;       //3,0
  ( tmp ) _CNV( 3, 1 ) = ori.y;       //3,1
  ( tmp ) _CNV( 3, 2 ) = ori.z;       //3,2
  ( tmp ) _CNV( 3, 3 ) = 1;       //3,3

  return tmp;
}

//----------------------------------------------------
// GS - Normally we souldn't this function but I found it in the rendering of the particules.
//
// This is just a MulVectorMatrix for now. The W division and screen size multiplication
// must be done afterward.
// Isn't tested!!!!
void Transform4_Full( matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor )
{
  vect4 *psrc = pSourceV, *pdst = pDestV;
  matrix_4x4 *pmat = pMatrix;
  while ( NumVertor-- > 0 )
  {
    if(0.0f == ( *psrc ).w)
    {
      ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8];
      ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9];
      ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10];
      ( *pdst ).w = ( *psrc ).x * ( *pmat ).v[3] + ( *psrc ).y * ( *pmat ).v[7] + ( *psrc ).z * ( *pmat ).v[11];
    }
    else if(1.0f == ( *psrc ).w)
    {
      ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8] + ( *pmat ).v[12];
      ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9] + ( *pmat ).v[13];
      ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10] + ( *pmat ).v[14];
      ( *pdst ).w = ( *psrc ).x * ( *pmat ).v[3] + ( *psrc ).y * ( *pmat ).v[7] + ( *psrc ).z * ( *pmat ).v[11] + ( *pmat ).v[15];
    }
    else
    {
      ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8] + ( *psrc ).w * ( *pmat ).v[12];
      ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9] + ( *psrc ).w * ( *pmat ).v[13];
      ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10] + ( *psrc ).w * ( *pmat ).v[14];
      ( *pdst ).w = ( *psrc ).x * ( *pmat ).v[3] + ( *psrc ).y * ( *pmat ).v[7] + ( *psrc ).z * ( *pmat ).v[11] + ( *psrc ).w * ( *pmat ).v[15];
    }
    psrc++;
    pdst++;
  }
}

//----------------------------------------------------
void Transform4( matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor )
{
  vect4 *psrc = pSourceV, *pdst = pDestV;
  matrix_4x4 *pmat = pMatrix;
  while ( NumVertor-- > 0 )
  {
    ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8];
    ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9];
    ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10];
    ( *pdst ).w = ( *psrc ).x * ( *pmat ).v[3] + ( *psrc ).y * ( *pmat ).v[7] + ( *psrc ).z * ( *pmat ).v[11];
    psrc++;
    pdst++;
  }
}

//----------------------------------------------------
void Transform3_Full( matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor )
{
  vect3 *psrc = pSourceV, *pdst = pDestV;
  matrix_4x4 *pmat = pMatrix;
  while ( NumVertor-- > 0 )
  {
    ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8] + ( *pmat ).v[12];
    ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9] + ( *pmat ).v[13];
    ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10] + ( *pmat ).v[14];
    psrc++;
    pdst++;
  }
}

//----------------------------------------------------
void Transform3( matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor )
{
  vect3 *psrc = pSourceV, *pdst = pDestV;
  matrix_4x4 *pmat = pMatrix;
  while ( NumVertor-- > 0 )
  {
    ( *pdst ).x = ( *psrc ).x * ( *pmat ).v[0] + ( *psrc ).y * ( *pmat ).v[4] + ( *psrc ).z * ( *pmat ).v[ 8];
    ( *pdst ).y = ( *psrc ).x * ( *pmat ).v[1] + ( *psrc ).y * ( *pmat ).v[5] + ( *psrc ).z * ( *pmat ).v[ 9];
    ( *pdst ).z = ( *psrc ).x * ( *pmat ).v[2] + ( *psrc ).y * ( *pmat ).v[6] + ( *psrc ).z * ( *pmat ).v[10];
    psrc++;
    pdst++;
  }
}

//----------------------------------------------------
void VectorClear( float v[] )
{
  v[0] = v[1] = v[2] = 0.0f;
};