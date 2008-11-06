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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

/**> HEADER FILES <**/
#include "mathstuff.h"


// FAKE D3D FUNCTIONS
glVector vsub( glVector A, glVector B )
{
  glVector tmp;
  tmp.x=A.x-B.x; tmp.y=A.y-B.y; tmp.z=A.z-B.z;
  return( tmp );
}

glVector Normalize( glVector vec )
{
  glVector tmp=vec;
  float len;
  len= SQRT( vec.x*vec.x+vec.y*vec.y+vec.z*vec.z );
  tmp.x/=len;
  tmp.y/=len;
  tmp.z/=len;
  return( tmp );
}

glVector CrossProduct( glVector A, glVector B )
{
  glVector tmp;
  tmp.x=A.y*B.z-A.z*B.y;
  tmp.y=A.z*B.x-A.x*B.z;
  tmp.z=A.x*B.y-A.y*B.x;
  return( tmp );
}

float DotProduct( glVector A, glVector B )
{ return( A.x*B.x+A.y*B.y+A.z*B.z ); }

//---------------------------------------------------------------------------------------------
// Math Stuff-----------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// inline D3DMATRIX IdentityMatrix()
glMatrix IdentityMatrix()
{
  glMatrix tmp;

  ( tmp )_CNV( 0,0 )=1; ( tmp )_CNV( 1,0 )=0; ( tmp )_CNV( 2,0 )=0; ( tmp )_CNV( 3,0 )=0;
  ( tmp )_CNV( 0,1 )=0; ( tmp )_CNV( 1,1 )=1; ( tmp )_CNV( 2,1 )=0; ( tmp )_CNV( 3,1 )=0;
  ( tmp )_CNV( 0,2 )=0; ( tmp )_CNV( 1,2 )=0; ( tmp )_CNV( 2,2 )=1; ( tmp )_CNV( 3,2 )=0;
  ( tmp )_CNV( 0,3 )=0; ( tmp )_CNV( 1,3 )=0; ( tmp )_CNV( 2,3 )=0; ( tmp )_CNV( 3,3 )=1;
  return( tmp );
}

//--------------------------------------------------------------------------------------------
// inline D3DMATRIX ZeroMatrix(void)  // initializes matrix to zero
glMatrix ZeroMatrix( void )
{
  glMatrix ret;
  int i,j;

  for ( i=0; i<4; i++ )
    for ( j=0; j<4; j++ )
      ( ret )_CNV( i,j )=0;
  return ret;
}

//--------------------------------------------------------------------------------------------
// inline D3DMATRIX MatrixMult(const D3DMATRIX a, const D3DMATRIX b)
glMatrix MatrixMult( const glMatrix a, const glMatrix b )
{
  glMatrix ret = ZeroMatrix();
  int i,j,k;

  for ( i=0; i<4; i++ )
    for ( j=0; j<4; j++ )
      for ( k=0; k<4; k++ )
        ( ret )_CNV( i,j ) += ( a )_CNV( k,j ) * ( b )_CNV( i,k );
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX Translate(const float dx, const float dy, const float dz)
glMatrix Translate( const float dx, const float dy, const float dz )
{
  glMatrix ret = IdentityMatrix();
  ( ret )_CNV( 3,0 ) = dx;
  ( ret )_CNV( 3,1 ) = dy;
  ( ret )_CNV( 3,2 ) = dz;
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX RotateX(const float rads)
glMatrix RotateX( const float rads )
{
  float cosine = COS( rads );
  float sine = SIN( rads );
  glMatrix ret = IdentityMatrix();
  ( ret )_CNV( 1,1 ) = cosine;
  ( ret )_CNV( 2,2 ) = cosine;
  ( ret )_CNV( 1,2 ) = -sine;
  ( ret )_CNV( 2,1 ) = sine;
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX RotateY(const float rads)
glMatrix RotateY( const float rads )
{
  float cosine = COS( rads );
  float sine = SIN( rads );
  glMatrix ret = IdentityMatrix();
  ( ret )_CNV( 0,0 ) = cosine;  //0,0
  ( ret )_CNV( 2,2 ) = cosine;  //2,2
  ( ret )_CNV( 0,2 ) = sine;  //0,2
  ( ret )_CNV( 2,0 ) = -sine;  //2,0
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX RotateZ(const float rads)
glMatrix RotateZ( const float rads )
{
  float cosine = COS( rads );
  float sine = SIN( rads );
  glMatrix ret = IdentityMatrix();
  ( ret )_CNV( 0,0 ) = cosine;  //0,0
  ( ret )_CNV( 1,1 ) = cosine;  //1,1
  ( ret )_CNV( 0,1 ) = -sine;  //0,1
  ( ret )_CNV( 1,0 ) = sine;  //1,0
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX ScaleXYZ(const float sizex, const float sizey, const float sizez)
glMatrix ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
  glMatrix ret = IdentityMatrix();
  ( ret )_CNV( 0,0 ) = sizex;  //0,0
  ( ret )_CNV( 1,1 ) = sizey;  //1,1
  ( ret )_CNV( 2,2 ) = sizez;  //2,2
  return ret;
}

//--------------------------------------------------------------------------------------------
/*D3DMATRIX ScaleXYZRotateXYZTranslate(const float sizex, const float sizey, const float sizez,
   Uint16 turnz, Uint16 turnx, Uint16 turny,
   float tx, float ty, float tz)*/
glMatrix ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz )
{
  float cx = turntosin[( turnx+4096 )&16383];
  float sx = turntosin[turnx];
  float cy = turntosin[( turny+4096 )&16383];
  float sy = turntosin[turny];
  float cz = turntosin[( turnz+4096 )&16383];
  float sz = turntosin[turnz];
  float sxsy = sx*sy;
  float cxsy = cx*sy;
  float sxcy = sx*cy;
  float cxcy = cx*cy;
  glMatrix ret;
  ( ret )_CNV( 0,0 ) = sizex*( cy*cz );      //0,0
  ( ret )_CNV( 0,1 ) = sizex*( sxsy*cz+cx*sz );  //0,1
  ( ret )_CNV( 0,2 ) = sizex*( -cxsy*cz+sx*sz );  //0,2
  ( ret )_CNV( 0,3 ) = 0;        //0,3

  ( ret )_CNV( 1,0 ) = sizey*( -cy*sz );      //1,0
  ( ret )_CNV( 1,1 ) = sizey*( -sxsy*sz+cx*cz );  //1,1
  ( ret )_CNV( 1,2 ) = sizey*( cxsy*sz+sx*cz );  //1,2
  ( ret )_CNV( 1,3 ) = 0;        //1,3

  ( ret )_CNV( 2,0 ) = sizez*( sy );  //2,0
  ( ret )_CNV( 2,1 ) = sizez*( -sxcy );      //2,1
  ( ret )_CNV( 2,2 ) = sizez*( cxcy );      //2,2
  ( ret )_CNV( 2,3 ) = 0;        //2,3

  ( ret )_CNV( 3,0 ) = tx;        //3,0
  ( ret )_CNV( 3,1 ) = ty;        //3,1
  ( ret )_CNV( 3,2 ) = tz;        //3,2
  ( ret )_CNV( 3,3 ) = 1;        //3,3
  return ret;
}

//--------------------------------------------------------------------------------------------
// D3DMATRIX FourPoints(float orix, float oriy, float oriz,
glMatrix FourPoints( float orix, float oriy, float oriz,
                     float widx, float widy, float widz,
                     float forx, float fory, float forz,
                     float upx,  float upy,  float upz,
                     float scale )
{
  glMatrix tmp;
  widx-=orix;  forx-=orix;  upx-=orix;
  widx=-widx;  // HUK
  widy-=oriy;  fory-=oriy;  upy-=oriy;
  widy=-widy; // HUK
  widz-=oriz;  forz-=oriz;  upz-=oriz;
  widz=-widz; // HUK
  widx=widx*scale;  forx=forx*scale;  upx=upx*scale;
  widy=widy*scale;  fory=fory*scale;  upy=upy*scale;
  widz=widz*scale;  forz=forz*scale;  upz=upz*scale;
  ( tmp )_CNV( 0,0 )=widx;              //0,0
  ( tmp )_CNV( 0,1 )=widy;              //0,1
  ( tmp )_CNV( 0,2 )=widz;              //0,2
  ( tmp )_CNV( 0,3 )=0;                //0,3
  ( tmp )_CNV( 1,0 )=forx;              //1,0
  ( tmp )_CNV( 1,1 )=fory;              //1,1
  ( tmp )_CNV( 1,2 )=forz;              //1,2
  ( tmp )_CNV( 1,3 )=0;                //1,3
  ( tmp )_CNV( 2,0 )=upx;              //2,0
  ( tmp )_CNV( 2,1 )=upy;              //2,1
  ( tmp )_CNV( 2,2 )=upz;              //2,2
  ( tmp )_CNV( 2,3 )=0;              //2,3
  ( tmp )_CNV( 3,0 )=orix;              //3,0
  ( tmp )_CNV( 3,1 )=oriy;              //3,1
  ( tmp )_CNV( 3,2 )=oriz;              //3,2
  ( tmp )_CNV( 3,3 )=1;              //3,3
  return( tmp );
}

//--------------------------------------------------------------------------------------------
// MN This probably should be replaced by a call to gluLookAt, don't see why we need to make our own...
//
// inline D3DMATRIX ViewMatrix(const D3DVECTOR from,      // camera location
glMatrix ViewMatrix( const glVector from,     // camera location
                     const glVector at,        // camera look-at target
                     const glVector world_up,  // world’s up, usually 0, 0, 1
                     const float roll )         // clockwise roll around
//    viewing direction,
//    in radians
{
  glMatrix view = IdentityMatrix();
  glVector up, right, view_dir;

  view_dir = Normalize( vsub( at,from ) );
  right = CrossProduct( world_up, view_dir );
  up = CrossProduct( view_dir, right );
  right = Normalize( right );
  up = Normalize( up );
  ( view )_CNV( 0,0 ) = right.x;        //0,0
  ( view )_CNV( 1,0 ) = right.y;        //1,0
  ( view )_CNV( 2,0 ) = right.z;        //2,0
  ( view )_CNV( 0,1 ) = up.x;          //0,1
  ( view )_CNV( 1,1 ) = up.y;          //1,1
  ( view )_CNV( 2,1 ) = up.z;          //2,1
  ( view )_CNV( 0,2 ) = view_dir.x;        //0,2
  ( view )_CNV( 1,2 ) = view_dir.y;        //1,2
  ( view )_CNV( 2,2 ) = view_dir.z;      //2,2
  ( view )_CNV( 3,0 ) = -DotProduct( right, from );    //3,0
  ( view )_CNV( 3,1 ) = -DotProduct( up, from );      //3,1
  ( view )_CNV( 3,2 ) = -DotProduct( view_dir, from );  //3,2

  if ( roll != 0.0f )
  {
    // MatrixMult function shown above
    view = MatrixMult( RotateZ( -roll ), view );
  }

  return view;
}

//--------------------------------------------------------------------------------------------
// MN Again, there is a gl function for this, glFrustum or gluPerspective... does this account for viewport ratio?
//
// inline D3DMATRIX ProjectionMatrix(const float near_plane,     // distance to near clipping plane
glMatrix ProjectionMatrix( const float near_plane,    // distance to near clipping plane
                           const float far_plane,      // distance to far clipping plane
                           const float fov )           // field of view angle, in radians
{
  float c = COS( fov*0.5f );
  float s = SIN( fov*0.5f );
  float Q = s/( 1.0f - near_plane/far_plane );
  glMatrix ret = ZeroMatrix();
  ( ret )_CNV( 0,0 ) = c;          //0,0
  ( ret )_CNV( 1,1 ) = c;          //1,1
  ( ret )_CNV( 2,2 ) = Q;          //2,2
  ( ret )_CNV( 3,2 ) = -Q*near_plane;    //3,2
  ( ret )_CNV( 2,3 ) = s;          //2,3
  return ret;
}


//----------------------------------------------------
// GS - Normally we souldn't this function but I found it in the rendering of the particules.
//
// This is just a MulVectorMatrix for now. The W division and screen size multiplication
// must be done afterward.
// Isn't tested!!!!
void  TransformVertices( glMatrix *pMatrix, glVector *pSourceV, glVector *pDestV, Uint32  pNumVertor )
{
  while ( pNumVertor-- )
  {
    pDestV->x = pSourceV->x * pMatrix->v[0] + pSourceV->y * pMatrix->v[4] + pSourceV->z * pMatrix->v[8] + pSourceV->w * pMatrix->v[12];
    pDestV->y = pSourceV->x * pMatrix->v[1] + pSourceV->y * pMatrix->v[5] + pSourceV->z * pMatrix->v[9] + pSourceV->w * pMatrix->v[13];
    pDestV->z = pSourceV->x * pMatrix->v[2] + pSourceV->y * pMatrix->v[6] + pSourceV->z * pMatrix->v[10] + pSourceV->w * pMatrix->v[14];
    pDestV->w = pSourceV->x * pMatrix->v[3] + pSourceV->y * pMatrix->v[7] + pSourceV->z * pMatrix->v[11] + pSourceV->w * pMatrix->v[15];
    pDestV++;
    pSourceV++;
  }
}
