/* Egoboo - graphicmad.c
 * Character model drawing code.
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

#include "egoboo.h"
#include "Md2.h"
#include "id_md2.h"
#include <SDL_opengl.h>
#include "graphic.h"

/* Storage for blended vertices */
static GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
static GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

/* blend_md2_vertices
 * Blends the vertices and normals between 2 frames of a md2 model for animation.
 *
 * NOTE: Only meant to be called from draw_textured_md2, which does the necessary
 * checks to make sure that the inputs are valid.  So this function itself assumes
 * that they are valid.  User beware!
 */
static void blend_md2_vertices( const Md2Model *model, int from_, int to_, float lerp )
{
  struct Md2Frame *from, *to;
  int numVertices, i;

  from = &model->frames[from_];
  to = &model->frames[to_];
  numVertices = model->numVertices;

  if ( lerp <= 0 )
  {
    // copy the vertices in frame 'from' over
    for ( i = 0;i < numVertices;i++ )
    {
      md2_blendedVertices[i][0] = from->vertices[i].x;
      md2_blendedVertices[i][1] = from->vertices[i].y;
      md2_blendedVertices[i][2] = from->vertices[i].z;

      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
      md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
      md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];
    }
  }
  else if ( lerp >= 1.0f )
  {
    // copy the vertices in frame 'to'
    for ( i = 0;i < numVertices;i++ )
    {
      md2_blendedVertices[i][0] = to->vertices[i].x;
      md2_blendedVertices[i][1] = to->vertices[i].y;
      md2_blendedVertices[i][2] = to->vertices[i].z;

      md2_blendedNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
      md2_blendedNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
      md2_blendedNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];
    }
  }
  else
  {
    // mix the vertices
    for ( i = 0;i < numVertices;i++ )
    {
      md2_blendedVertices[i][0] = from->vertices[i].x +
                                  ( to->vertices[i].x - from->vertices[i].x ) * lerp;
      md2_blendedVertices[i][1] = from->vertices[i].y +
                                  ( to->vertices[i].y - from->vertices[i].y ) * lerp;
      md2_blendedVertices[i][2] = from->vertices[i].z +
                                  ( to->vertices[i].z - from->vertices[i].z ) * lerp;

      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
                                 ( kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0] ) * lerp;
      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][1] +
                                 ( kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1] ) * lerp;
      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][2] +
                                 ( kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2] ) * lerp;
    }
  }
}

/* draw_textured_md2
 * Draws a Md2Model in the new format
 */
void draw_textured_md2( const Md2Model *model, int from_, int to_, float lerp )
{
  int i, numTriangles;
  const struct Md2TexCoord *tc;
  const struct Md2Triangle *triangles;
  const struct Md2Triangle *tri;

  if ( model == NULL ) return;
  if ( from_ < 0 || from_ >= model->numFrames ) return;
  if ( to_ < 0 || to_ >= model->numFrames ) return;

  blend_md2_vertices( model, from_, to_, lerp );

  numTriangles = model->numTriangles;
  tc = model->texCoords;
  triangles = model->triangles;

  glEnableClientState( GL_VERTEX_ARRAY );
  glEnableClientState( GL_NORMAL_ARRAY );

  glVertexPointer( 3, GL_FLOAT, 0, md2_blendedVertices );
  glNormalPointer( GL_FLOAT, 0, md2_blendedNormals );

  glBegin( GL_TRIANGLES );
  {
    for ( i = 0;i < numTriangles;i++ )
    {
      tri = &triangles[i];

      glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[0]] ) );
      glArrayElement( tri->vertexIndices[0] );

      glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[1]] ) );
      glArrayElement( tri->vertexIndices[1] );

      glTexCoord2fv( ( const GLfloat* )&( tc[tri->texCoordIndices[2]] ) );
      glArrayElement( tri->vertexIndices[2] );
    }
  }
  glEnd();

  glDisableClientState( GL_VERTEX_ARRAY );
  glDisableClientState( GL_NORMAL_ARRAY );
}

//--------------------------------------------------------------------------------------------
void render_enviromad( Uint16 character, Uint8 trans )
{
  // ZZ> This function draws an environment mapped model
  GLVERTEX v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  Sint32 temp;
  Uint8 ambi;
  glMatrix tempWorld = mWorld;

  Uint16 model = chrmodel[character];
  Uint16 texture = chrtexture[character];
  Uint16 frame = chrframe[character];
  Uint16 lastframe = chrlastframe[character];
  Uint16 framestt = madframestart[chrmodel[character]];
  Uint8 lip = chrlip[character] >> 6;
  Uint8 lightrotation = ( chrturnleftright[character] + chrlightturnleftright[character] ) >> 8;
  Uint32  alpha = trans;
  Uint8 lightlevel = chrlightlevel[character] >> 4;
  float uoffset = textureoffset[chruoffset[character] >> 8] - camturnleftrightone;
  float voffset = textureoffset[chrvoffset[character] >> 8];
  Uint8 rs = chrredshift[character];
  Uint8 gs = chrgrnshift[character];
  Uint8 bs = chrblushift[character];


  // Original points with linear interpolation ( lip )
  switch ( lip )
  {
    case 0:  // 25% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        temp = madvrtx[lastframe][cnt];
        temp = temp + temp + temp;
        v[cnt].x = ( float ) ( ( madvrtx[frame][cnt] + temp ) >> 2 );
        temp = madvrty[lastframe][cnt];
        temp = temp + temp + temp;
        v[cnt].y = ( float ) ( ( madvrty[frame][cnt] + temp ) >> 2 );
        temp = madvrtz[lastframe][cnt];
        temp = temp + temp + temp;
        v[cnt].z = ( float ) ( ( madvrtz[frame][cnt] + temp ) >> 2 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].a = ( float )alpha / 255.0f;
        v[cnt].r = ( float )( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float )( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float )( ambi >> bs ) / 255.0f;
        // v[cnt].dwReserved = 0;
      }
      break;

    case 1:  // 50% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        v[cnt].x = ( float ) ( ( madvrtx[frame][cnt] +
                                 madvrtx[lastframe][cnt] ) >> 1 );
        v[cnt].y = ( float ) ( ( madvrty[frame][cnt] +
                                 madvrty[lastframe][cnt] ) >> 1 );
        v[cnt].z = ( float ) ( ( madvrtz[frame][cnt] +
                                 madvrtz[lastframe][cnt] ) >> 1 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].a = ( float )alpha / 255.0f;
        v[cnt].r = ( float )( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float )( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float )( ambi >> bs ) / 255.0f;
        // v[cnt].dwReserved = 0;
      }
      break;

    case 2:  // 75% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        temp = madvrtx[frame][cnt];
        temp = temp + temp + temp;
        v[cnt].x = ( float ) ( ( madvrtx[lastframe][cnt] + temp ) >> 2 );
        temp = madvrty[frame][cnt];
        temp = temp + temp + temp;
        v[cnt].y = ( float ) ( ( madvrty[lastframe][cnt] + temp ) >> 2 );
        temp = madvrtz[frame][cnt];
        temp = temp + temp + temp;
        v[cnt].z = ( float ) ( ( madvrtz[lastframe][cnt] + temp ) >> 2 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].a = ( float )alpha / 255.0f;
        v[cnt].r = ( float )( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float )( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float )( ambi >> bs ) / 255.0f;
        // v[cnt].dwReserved = 0;
      }
      break;

    case 3:  // 100% this frame...  This is the legible one
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        v[cnt].x = ( float ) madvrtx[frame][cnt];
        v[cnt].y = ( float ) madvrty[frame][cnt];
        v[cnt].z = ( float ) madvrtz[frame][cnt];

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].a = ( float )alpha / 255.0f;
        v[cnt].r = ( float )( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float )( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float )( ambi >> bs ) / 255.0f;
        // v[cnt].dwReserved = 0;
      }
      break;
  }

  // Do fog...
  /*
  if(fogon && chrlight[character]==255)
  {
      // The full fog value
      alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

      for (cnt = 0; cnt < madtransvertices[model]; cnt++)
      {
          // Figure out the z position of the vertex...  Not totally accurate
          z = (v[cnt].z * chrscale[character]) + chrmatrix[character](3,2);

          // Figure out the fog coloring
          if(z < fogtop)
          {
              if(z < fogbottom)
              {
                  v[cnt].specular = alpha;
              }
              else
              {
                  z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                  red = fogred * z;
                  grn = foggrn * z;
                  blu = fogblu * z;
                  fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                  v[cnt].specular = fogspec;
              }
          }
          else
          {
              v[cnt].specular = 0;
          }
      }
  }
  else
  {
      for (cnt = 0; cnt < madtransvertices[model]; cnt++)
          v[cnt].specular = 0;
  }
  */

  mWorld = chrmatrix[character];
  // GS - Begin3DMode ();

  glLoadMatrixf( mView.v );
  glMultMatrixf( mWorld.v );

  // Choose texture and matrix
    GLTexture_Bind( &txTexture[texture] );
  // Make new ones so we can index them and not transform 'em each time
  // if(transform_vertices(madtransvertices[model], v, vt))
  // return;

  // Render each command
  entry = 0;
  for ( cnt = 0; cnt < madcommands[model]; cnt++ )
  {
    if(cnt > MAXCOMMAND) continue;

    glBegin ( madcommandtype[model][cnt] );

    for ( tnc = 0; tnc < madcommandsize[model][cnt]; tnc++ )
    {
      vertex = madcommandvrt[model][entry];

      if( vertex < madvertices[model] && entry < MAXCOMMANDENTRIES )
      {
        glColor4fv( &v[vertex].r );
        glTexCoord2f ( indextoenvirox[madvrta[framestt][vertex]] + uoffset,
                      lighttoenviroy[chrvrta[character][vertex]] + voffset );
        glVertex3fv ( &v[vertex].x );
      }

      entry++;
    }
    glEnd ();
  }

  mWorld = tempWorld;
  glLoadMatrixf( mView.v );
  glMultMatrixf( mWorld.v );
  // GS - End3DMode ();
}

//--------------------------------------------------------------------------------------------
void render_texmad( Uint16 character, Uint8 trans )
{
  // ZZ> This function draws a model
  GLVERTEX v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  Sint32 temp;
//  float z, fogtokeep;
//  Uint8 red, grn, blu;
  Uint8 ambi;
//  DWORD fogspec;

  // To make life easier
  Uint16 model = chrmodel[character];
  Uint16 texture = chrtexture[character];
  Uint16 frame = chrframe[character];
  Uint16 lastframe = chrlastframe[character];
  Uint8 lip = chrlip[character] >> 6;
  Uint8 lightrotation =
    ( chrturnleftright[character] + chrlightturnleftright[character] ) >> 8;
  Uint8 lightlevel = chrlightlevel[character] >> 4;
  Uint32  alpha = trans;
  Uint8 spek = chrsheen[character];

  float uoffset = textureoffset[chruoffset[character] >> 8];
  float voffset = textureoffset[chrvoffset[character] >> 8];
  Uint8 rs = chrredshift[character];
  Uint8 gs = chrgrnshift[character];
  Uint8 bs = chrblushift[character];
  glMatrix mTempWorld = mWorld;

  if ( phongon && trans == 255 )
    spek = 0;

  // Original points with linear interpolation ( lip )
  switch ( lip )
  {
    case 0:  // 25% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        temp = madvrtx[lastframe][cnt];
        temp = temp + temp + temp;
        // v[cnt].x = (D3DVALUE) ((madvrtx[frame][cnt] + temp)>>2);
        v[cnt].x = ( float ) ( ( madvrtx[frame][cnt] + temp ) >> 2 );
        temp = madvrty[lastframe][cnt];
        temp = temp + temp + temp;
        // v[cnt].y = (D3DVALUE) ((madvrty[frame][cnt] + temp)>>2);
        v[cnt].y = ( float ) ( ( madvrty[frame][cnt] + temp ) >> 2 );
        temp = madvrtz[lastframe][cnt];
        temp = temp + temp + temp;
        // v[cnt].z = (D3DVALUE) ((madvrtz[frame][cnt] + temp)>>2);
        v[cnt].z = ( float ) ( ( madvrtz[frame][cnt] + temp ) >> 2 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].r = ( float ) ( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float ) ( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float ) ( ambi >> bs ) / 255.0f;
        v[cnt].a = ( float ) alpha / 255.0f;

        // v[cnt].specular = lighttospek[spek][ambi];

        // v[cnt].dwReserved = 0;
      }
      break;
    case 1:  // 50% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        /*
                    v[cnt].x = (D3DVALUE) (madvrtx[frame][cnt] +
                                           madvrtx[lastframe][cnt]>>1);
                    v[cnt].y = (D3DVALUE) (madvrty[frame][cnt] +
                                           madvrty[lastframe][cnt]>>1);
                    v[cnt].z = (D3DVALUE) (madvrtz[frame][cnt] +
                                           madvrtz[lastframe][cnt]>>1);
        */
        v[cnt].x = ( float ) ( ( madvrtx[frame][cnt] +
                                 madvrtx[lastframe][cnt] ) >> 1 );
        v[cnt].y = ( float ) ( ( madvrty[frame][cnt] +
                                 madvrty[lastframe][cnt] ) >> 1 );
        v[cnt].z = ( float ) ( ( madvrtz[frame][cnt] +
                                 madvrtz[lastframe][cnt] ) >> 1 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].r = ( float ) ( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float ) ( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float ) ( ambi >> bs ) / 255.0f;
        v[cnt].a = ( float ) alpha  / 255.0f;

        // v[cnt].specular = lighttospek[spek][ambi];

        // v[cnt].dwReserved = 0;
      }
      break;
    case 2:  // 75% this frame
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        temp = madvrtx[frame][cnt];
        temp = temp + temp + temp;
        // v[cnt].x = (D3DVALUE) (madvrtx[lastframe][cnt] + temp>>2);
        v[cnt].x = ( float ) ( ( madvrtx[lastframe][cnt] + temp ) >> 2 );
        temp = madvrty[frame][cnt];
        temp = temp + temp + temp;
        // v[cnt].y = (D3DVALUE) (madvrty[lastframe][cnt] + temp>>2);
        v[cnt].y = ( float ) ( ( madvrty[lastframe][cnt] + temp ) >> 2 );
        temp = madvrtz[frame][cnt];
        temp = temp + temp + temp;
        // v[cnt].z = (D3DVALUE) (madvrtz[lastframe][cnt] + temp>>2);
        v[cnt].z = ( float ) ( ( madvrtz[lastframe][cnt] + temp ) >> 2 );

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].r = ( float ) ( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float ) ( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float ) ( ambi >> bs ) / 255.0f;
        v[cnt].a = ( float ) alpha / 255.0f;

        // v[cnt].specular = lighttospek[spek][ambi];

        // v[cnt].dwReserved = 0;
      }
      break;
    case 3:  // 100% this frame...  This is the legible one
      for ( cnt = 0; cnt < madvertices[model]; cnt++ )
      {
        /*
                    v[cnt].x = (D3DVALUE) madvrtx[frame][cnt];
                    v[cnt].y = (D3DVALUE) madvrty[frame][cnt];
                    v[cnt].z = (D3DVALUE) madvrtz[frame][cnt];
        */
        v[cnt].x = ( float ) madvrtx[frame][cnt];
        v[cnt].y = ( float ) madvrty[frame][cnt];
        v[cnt].z = ( float ) madvrtz[frame][cnt];

        ambi = chrvrta[character][cnt];
        ambi = ( ( ( ambi + ambi + ambi ) << 1 ) + ambi + lighttable[lightlevel][lightrotation][madvrta[frame][cnt]] ) >> 3;
        chrvrta[character][cnt] = ambi;
        // v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
        v[cnt].r = ( float ) ( ambi >> rs ) / 255.0f;
        v[cnt].g = ( float ) ( ambi >> gs ) / 255.0f;
        v[cnt].b = ( float ) ( ambi >> bs ) / 255.0f;
        v[cnt].a = ( float ) alpha / 255.0f;

        // v[cnt].specular = lighttospek[spek][ambi];

        // v[cnt].dwReserved = 0;
      }
      break;
  }

  /*
      // Do fog...
      if(fogon && chrlight[character]==255)
      {
          // The full fog value
          alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

          for (cnt = 0; cnt < madtransvertices[model]; cnt++)
          {
              // Figure out the z position of the vertex...  Not totally accurate
              z = (v[cnt].z * chrscale[character]) + chrmatrix[character](3,2);

              // Figure out the fog coloring
              if(z < fogtop)
              {
                  if(z < fogbottom)
                  {
                      v[cnt].specular = alpha;
                  }
                  else
                  {
                      spek = v[cnt].specular & 255;
                      z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                      fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                      spek = spek * z;
                      red = (fogred * fogtokeep) + spek;
                      grn = (foggrn * fogtokeep) + spek;
                      blu = (fogblu * fogtokeep) + spek;
                      fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                      v[cnt].specular = fogspec;
                  }
              }
          }
      }
  */

  // Choose texture and matrix
/*  if ( KEYDOWN( SDLK_F7 ) )
  {
    glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[texture] ) );
  }
  else*/
  {
    GLTexture_Bind( &txTexture[texture] );
  }

  mWorld = chrmatrix[character];

  // Begin3DMode();
  glLoadMatrixf( mView.v );
  glMultMatrixf( mWorld.v );

  // Make new ones so we can index them and not transform 'em each time
  //  if(transform_vertices(madtransvertices[model], v, vt))
  //      return;

  // Render each command
  entry = 0;

  for ( cnt = 0; cnt < madcommands[model]; cnt++ )
  {
    if(cnt > MAXCOMMAND) continue;

    glBegin ( madcommandtype[model][cnt] );
    for ( tnc = 0; tnc < madcommandsize[model][cnt]; tnc++ )
    {
      vertex = madcommandvrt[model][entry];

      if( vertex < madvertices[model] && entry < MAXCOMMANDENTRIES )
      {
        glColor4fv( &v[vertex].r );
        glTexCoord2f ( madcommandu[model][entry] + uoffset, madcommandv[model][entry] + voffset );
        glVertex3fv ( &v[vertex].x );
      }
      entry++;
    }
    glEnd ();
  }
  // End3DMode ();

  mWorld = mTempWorld;
  glLoadMatrixf( mView.v );
  glMultMatrixf( mWorld.v );
}
//--------------------------------------------------------------------------------------------
void render_mad( Uint16 character, Uint8 trans )
{
  // ZZ> This function picks the actual function to use
  Sint8 hide = caphidestate[chrmodel[character]];

  if ( hide == NOHIDE || hide != chraistate[character] )
  {
    if ( chrenviro[character] )
      render_enviromad( character, trans );
    else
      render_texmad( character, trans );
  }
}

//--------------------------------------------------------------------------------------------
void render_refmad( int tnc, Uint8 trans )
{
  // ZZ> This function draws characters reflected in the floor

  float level;
  int trans_temp;
  int zpos;
  Uint8 sheen_save;
  Uint8 fog_save;
  glVector pos_save;

  if ( !capreflect[chrmodel[tnc]] ) return;

  level = chrlevel[tnc];
  trans_temp = trans;

  zpos = ( chrmatrix[tnc] )_CNV( 3, 2 ) - level;
  if(zpos < 0) zpos = 0;

  trans_temp -= zpos >> 1;
  if ( trans_temp < 0 ) trans_temp = 0;

  trans_temp |= reffadeor;  // Fix for Riva owners
  if ( trans_temp > 255 ) trans_temp = 255;

  if ( trans_temp <= 0 ) return;

  chrredshift[tnc] += 1;
  chrgrnshift[tnc] += 1;
  chrblushift[tnc] += 1;

  sheen_save = chrsheen[tnc];
  chrsheen[tnc] >>= 1;

  pos_save.x = ( chrmatrix[tnc] )_CNV( 0, 2 );
  pos_save.y = ( chrmatrix[tnc] )_CNV( 1, 2 );
  pos_save.z = ( chrmatrix[tnc] )_CNV( 2, 2 );
  pos_save.w = ( chrmatrix[tnc] )_CNV( 3, 2 );

  ( chrmatrix[tnc] )_CNV( 0, 2 ) = -( chrmatrix[tnc] )_CNV( 0, 2 );
  ( chrmatrix[tnc] )_CNV( 1, 2 ) = -( chrmatrix[tnc] )_CNV( 1, 2 );
  ( chrmatrix[tnc] )_CNV( 2, 2 ) = -( chrmatrix[tnc] )_CNV( 2, 2 );
  ( chrmatrix[tnc] )_CNV( 3, 2 ) = -( chrmatrix[tnc] )_CNV( 3, 2 ) + level + level;

  fog_save = fogon;
  fogon    = bfalse;

  render_mad( tnc, trans_temp );

  fogon = fog_save;

  ( chrmatrix[tnc] )_CNV( 0, 2 ) = pos_save.x;
  ( chrmatrix[tnc] )_CNV( 1, 2 ) = pos_save.y;
  ( chrmatrix[tnc] )_CNV( 2, 2 ) = pos_save.z;
  ( chrmatrix[tnc] )_CNV( 3, 2 ) = pos_save.w;

  chrsheen[tnc] = sheen_save;

  chrredshift[tnc] -= 1;
  chrgrnshift[tnc] -= 1;
  chrblushift[tnc] -= 1;

}
