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
along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Md2.h"
#include "id_md2.h"
#include "Log.h"
#include <SDL_opengl.h>

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
//static void blend_md2_vertices(Uint8 character, const Md2Model *model, int from_, int to_, float lerp)
//{
//  Md2Frame *from, *to;
//  int numVertices, i;
//  Uint8 ambilevel_fp8 = chrlightambi_fp8[character];
//  Uint8 speklevel_fp8 = chrlightspek_fp8[character];
//  Uint8 sheen_fp8     = chrsheen_fp8[character];
//  Uint8 lightrotation = (chrturn_lr[character] + chrlightturn_lr[character]) >> 8;
//  Uint16 lightold, lightnew;
//  float spekularity_fp8 = FLOAT_TO_FP8(lightspek * ((float)sheen_fp8 / (float)MAXSPEKLEVEL));
//
//  from = &model->frames[from_];
//  to = &model->frames[to_];
//  numVertices = model->numVertices;
//
//
//  if (lerp <= 0)
//  {
//    // copy the vertices in frame 'from' over
//    for (i = 0;i < numVertices;i++)
//    {
//      md2_blendedVertices[i][0] = from->vertices[i].x;
//      md2_blendedVertices[i][1] = from->vertices[i].y;
//      md2_blendedVertices[i][2] = from->vertices[i].z;
//
//      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
//      md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
//      md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];
//
//      md2_blendedNormals[i][0] *= -1;
//
//      lightnew = MIN(255, speklevel_fp8 * spek_local[lightrotation][madvrta[from_][i]] + spekularity_fp8 * spek_global[lightrotation][madvrta[from_][i]]);
//      lightnew = lighttospek[sheen_fp8][lightnew] + ambilevel_fp8 + (lightambi*255);
//      lightold = chrvrta_fp8[character][i];
//      chrvrta_fp8[character][i] = 0.9 * lightold + 0.1 * lightnew;
//    }
//  }
//  else if (lerp >= 1.0f)
//  {
//    // copy the vertices in frame 'to'
//    for (i = 0;i < numVertices;i++)
//    {
//      md2_blendedVertices[i][0] = to->vertices[i].x;
//      md2_blendedVertices[i][1] = to->vertices[i].y;
//      md2_blendedVertices[i][2] = to->vertices[i].z;
//
//      md2_blendedNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
//      md2_blendedNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
//      md2_blendedNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];
//
//      md2_blendedNormals[i][0] *= -1;
//
//      lightnew = MIN(255, speklevel_fp8 * spek_local[lightrotation][madvrta[to_][i]] + spekularity_fp8 * spek_global[lightrotation][madvrta[to_][i]]);
//      lightnew = lighttospek[sheen_fp8][lightnew] + ambilevel_fp8 + (lightambi*255);
//      lightold = chrvrta_fp8[character][i];
//      chrvrta_fp8[character][i] = 0.9 * lightold + 0.1 * lightnew;
//    }
//  }
//  else
//  {
//    // mix the vertices
//    for (i = 0;i < numVertices;i++)
//    {
//      md2_blendedVertices[i][0] = from->vertices[i].x +
//                                  (to->vertices[i].x - from->vertices[i].x) * lerp;
//      md2_blendedVertices[i][1] = from->vertices[i].y +
//                                  (to->vertices[i].y - from->vertices[i].y) * lerp;
//      md2_blendedVertices[i][2] = from->vertices[i].z +
//                                  (to->vertices[i].z - from->vertices[i].z) * lerp;
//
//      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
//                                 (kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0]) * lerp;
//      md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1] +
//                                 (kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1]) * lerp;
//      md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2] +
//                                 (kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2]) * lerp;
//
//
//      md2_blendedNormals[i][0] *= -1;
//
//      lightold = MIN(255, speklevel_fp8 * spek_local[lightrotation][madvrta[from_][i]] + spekularity_fp8 * spek_global[lightrotation][madvrta[from_][i]]);
//      lightold = lighttospek[sheen_fp8][lightold] + ambilevel_fp8 + (lightambi * 255);
//
//      lightnew = MIN(255, speklevel_fp8 * spek_local[lightrotation][madvrta[to_][i]] + spekularity_fp8 * spek_global[lightrotation][madvrta[to_][i]]);
//      lightnew = lighttospek[sheen_fp8][lightnew] + ambilevel_fp8 + (lightambi*255);
//
//      lightnew = (1 - lerp) * lightold + lerp * lightnew;
//      lightold = chrvrta_fp8[character][i];
//      chrvrta_fp8[character][i] = 0.9 * lightold + 0.1 * lightnew;
//    }
//  }
//}
//
/* draw_textured_md2
* Draws a Md2Model in the new format
*/
//void draw_textured_md2(CHR_REF character, const Md2Model *model, int from_, int to_, float lerp)
//{
//  int i, numTriangles;
//  const Md2TexCoord *tc;
//  const Md2Triangle *triangles;
//  const Md2Triangle *tri;
//
//  if (model == NULL) return;
//  if (from_ < 0 || from_ >= model->numFrames) return;
//  if (to_ < 0 || to_ >= model->numFrames) return;
//
//  blend_md2_vertices(character, model, from_, to_, lerp);
//
//  numTriangles = model->numTriangles;
//  tc = model->texCoords;
//  triangles = model->triangles;
//
//  glEnableClientState(GL_VERTEX_ARRAY);
//  glEnableClientState(GL_NORMAL_ARRAY);
//
//  glVertexPointer(3, GL_FLOAT, 0, md2_blendedVertices);
//  glNormalPointer(GL_FLOAT, 0, md2_blendedNormals);
//
//  glBegin(GL_TRIANGLES);
//  {
//    for (i = 0;i < numTriangles;i++)
//    {
//      tri = &triangles[i];
//
//      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[0]]));
//      glArrayElement(tri->vertexIndices[0]);
//
//      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[1]]));
//      glArrayElement(tri->vertexIndices[1]);
//
//      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[2]]));
//      glArrayElement(tri->vertexIndices[2]);
//    }
//  }
//  glEnd();
//
//  glDisableClientState(GL_VERTEX_ARRAY);
//  glDisableClientState(GL_NORMAL_ARRAY);
//}
//
//--------------------------------------------------------------------------------------------
void render_mad_lit( CHR_REF character )
{
  // ZZ> This function draws an environment mapped model

  GLVertex v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;

  Uint8  sheen_fp8 = chrsheen_fp8[character];
  float  spekularity_fp8 = FLOAT_TO_FP8(( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL );
  Uint16 model         = chrmodel[character];
  Uint16 texture       = chrtexture[character];
  Uint16 frame         = chrframe[character];
  Uint16 lastframe     = chrlastframe[character];
  float flip           = chrflip[character];
  Uint8 lightrotationr = ( chrturn_lr[character] + chrlightturn_lrr[character] ) >> 8;
  Uint8 lightrotationg = ( chrturn_lr[character] + chrlightturn_lrg[character] ) >> 8;
  Uint8 lightrotationb = ( chrturn_lr[character] + chrlightturn_lrb[character] ) >> 8;
  Uint8 ambilevelr_fp8 = chrlightambir_fp8[character];
  Uint8 ambilevelg_fp8 = chrlightambig_fp8[character];
  Uint8 ambilevelb_fp8 = chrlightambib_fp8[character];
  Uint8 speklevelr_fp8 = chrlightspekr_fp8[character];
  Uint8 speklevelg_fp8 = chrlightspekg_fp8[character];
  Uint8 speklevelb_fp8 = chrlightspekb_fp8[character];


  float uoffset = textureoffset[ FP8_TO_INT( chruoffset_fp8[character] )];
  float voffset = textureoffset[ FP8_TO_INT( chrvoffset_fp8[character] )];
  Uint8 rs = chrredshift[character];
  Uint8 gs = chrgrnshift[character];
  Uint8 bs = chrblushift[character];

  float mat_none[4]     = {0, 0, 0, 0};
  float mat_emission[4] = {0, 0, 0, 0};
  float mat_diffuse[4]  = {0, 0, 0, 0};
  float mat_specular[4] = {0, 0, 0, 0};
  float shininess[1] = {2};

  float ftmp;

  // Original points with linear interpolation ( lip )
  if ( frame == lastframe )
  {
    for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt];
      v[cnt].pos.y = madvrty[lastframe][cnt];
      v[cnt].pos.z = madvrtz[lastframe][cnt];
    }
  }
  else
  {
    for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt] + ( madvrtx[frame][cnt] - madvrtx[lastframe][cnt] ) * flip;
      v[cnt].pos.y = madvrty[lastframe][cnt] + ( madvrty[frame][cnt] - madvrty[lastframe][cnt] ) * flip;
      v[cnt].pos.z = madvrtz[lastframe][cnt] + ( madvrtz[frame][cnt] - madvrtz[lastframe][cnt] ) * flip;
    }
  };

  ftmp = (( float )( MAXSPEKLEVEL - sheen_fp8 ) / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( chralpha_fp8[character] ) );
  mat_diffuse[0] = ftmp * FP8_TO_FLOAT( chralpha_fp8[character] ) / ( float )( 1 << rs );
  mat_diffuse[1] = ftmp * FP8_TO_FLOAT( chralpha_fp8[character] ) / ( float )( 1 << gs );
  mat_diffuse[2] = ftmp * FP8_TO_FLOAT( chralpha_fp8[character] ) / ( float )( 1 << bs );


  ftmp = (( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( chralpha_fp8[character] ) );
  mat_specular[0] = ftmp / ( float )( 1 << rs );
  mat_specular[1] = ftmp / ( float )( 1 << gs );
  mat_specular[2] = ftmp / ( float )( 1 << bs );

  shininess[0] = sheen_fp8 + 2;

  if ( 255 != chrlight_fp8[character] )
  {
    ftmp = FP8_TO_FLOAT( chrlight_fp8[character] );
    mat_emission[0] = ftmp / ( float )( 1 << rs );
    mat_emission[1] = ftmp / ( float )( 1 << gs );
    mat_emission[2] = ftmp / ( float )( 1 << bs );
  }

  glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular );
  glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, shininess );
  glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,  mat_emission );

  ATTRIB_PUSH( "render_mad_lit", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[character]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[character].v );
    chrmatrix[character]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    // Render each command
    glColor4f( 1, 1, 1, 1 );
    entry = 0;
    for ( cnt = 0; cnt < madcommands[model]; cnt++ )
    {
      glBegin( madcommandtype[model][cnt] );

      for ( tnc = 0; tnc < madcommandsize[model][cnt]; tnc++ )
      {
        vertex = madcommandvrt[model][entry];


        glTexCoord2f( madcommandu[model][entry] + uoffset, madcommandv[model][entry] + voffset );
        glNormal3f( -kMd2Normals[madvrta[lastframe][vertex]][0], kMd2Normals[madvrta[lastframe][vertex]][1], kMd2Normals[madvrta[lastframe][vertex]][2] );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }

    glPopMatrix();
  }
  ATTRIB_POP( "render_mad_lit" );

}


//--------------------------------------------------------------------------------------------
void render_enviromad( CHR_REF character, Uint8 trans )
{
  // ZZ> This function draws an environment mapped model
  //D3DLVERTEX v[MAXVERTICES];
  //D3DTLVERTEX vt[MAXVERTICES];
  //D3DTLVERTEX vtlist[MAXCOMMANDSIZE];
  GLVertex v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  //float z;
  //Uint8 red, grn, blu;
  //DWORD fogspec;

  Uint8  sheen_fp8 = chrsheen_fp8[character];
  float  spekularity_fp8 = FLOAT_TO_FP8(( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL );
  Uint16 model        = chrmodel[character];
  Uint16 texture      = chrtexture[character];
  Uint16 frame        = chrframe[character];
  Uint16 lastframe    = chrlastframe[character];
  Uint16 framestt     = madframestart[chrmodel[character]];
  float flip           = chrflip[character];
  Uint8 lightrotationr = ( chrturn_lr[character] + chrlightturn_lrr[character] ) >> 8;
  Uint8 lightrotationg = ( chrturn_lr[character] + chrlightturn_lrg[character] ) >> 8;
  Uint8 lightrotationb = ( chrturn_lr[character] + chrlightturn_lrb[character] ) >> 8;
  Uint8 ambilevelr_fp8 = chrlightambir_fp8[character];
  Uint8 ambilevelg_fp8 = chrlightambig_fp8[character];
  Uint8 ambilevelb_fp8 = chrlightambib_fp8[character];
  Uint8 speklevelr_fp8 = chrlightspekr_fp8[character];
  Uint8 speklevelg_fp8 = chrlightspekg_fp8[character];
  Uint8 speklevelb_fp8 = chrlightspekb_fp8[character];
  float uoffset = textureoffset[ FP8_TO_INT( chruoffset_fp8[character] )] + camturn_lr_one;
  float voffset = textureoffset[ FP8_TO_INT( chrvoffset_fp8[character] )];
  Uint8 rs = chrredshift[character];
  Uint8 gs = chrgrnshift[character];
  Uint8 bs = chrblushift[character];
  Uint16 lightnew_r, lightnew_g, lightnew_b;
  Uint16 lightold_r, lightold_g, lightold_b;


  // Original points with linear interpolation ( lip )

  for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
  {
    if ( frame == lastframe )
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt];
      v[cnt].pos.y = madvrty[lastframe][cnt];
      v[cnt].pos.z = madvrtz[lastframe][cnt];
    }
    else
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt] + ( madvrtx[frame][cnt] - madvrtx[lastframe][cnt] ) * flip;
      v[cnt].pos.y = madvrty[lastframe][cnt] + ( madvrty[frame][cnt] - madvrty[lastframe][cnt] ) * flip;
      v[cnt].pos.z = madvrtz[lastframe][cnt] + ( madvrtz[frame][cnt] - madvrtz[lastframe][cnt] ) * flip;
    }

    lightnew_r = speklevelr_fp8 * spek_local[lightrotationr][madvrta[lastframe][cnt]] + lightspekcol.x * spekularity_fp8 * spek_global[lightrotationr][madvrta[lastframe][cnt]];
    lightnew_g = speklevelg_fp8 * spek_local[lightrotationg][madvrta[lastframe][cnt]] + lightspekcol.y * spekularity_fp8 * spek_global[lightrotationg][madvrta[lastframe][cnt]];
    lightnew_b = speklevelb_fp8 * spek_local[lightrotationb][madvrta[lastframe][cnt]] + lightspekcol.z * spekularity_fp8 * spek_global[lightrotationb][madvrta[lastframe][cnt]];

    lightnew_r = lightnew_r + ( speklevelr_fp8 * spek_local[lightrotationr][madvrta[frame][cnt]] + lightspekcol.x * spekularity_fp8 * spek_global[lightrotationr][madvrta[frame][cnt]] - lightnew_r ) * flip;
    lightnew_g = lightnew_g + ( speklevelg_fp8 * spek_local[lightrotationg][madvrta[frame][cnt]] + lightspekcol.y * spekularity_fp8 * spek_global[lightrotationg][madvrta[frame][cnt]] - lightnew_g ) * flip;
    lightnew_b = lightnew_b + ( speklevelb_fp8 * spek_local[lightrotationb][madvrta[frame][cnt]] + lightspekcol.z * spekularity_fp8 * spek_global[lightrotationb][madvrta[frame][cnt]] - lightnew_b ) * flip;

    lightnew_r = lighttospek[sheen_fp8][lightnew_r] + ambilevelr_fp8 + ( lightambicol.x * 255 );
    lightnew_g = lighttospek[sheen_fp8][lightnew_g] + ambilevelg_fp8 + ( lightambicol.y * 255 );
    lightnew_b = lighttospek[sheen_fp8][lightnew_b] + ambilevelb_fp8 + ( lightambicol.z * 255 );

    lightold_r = chrvrtar_fp8[character][cnt];
    lightold_g = chrvrtag_fp8[character][cnt];
    lightold_b = chrvrtab_fp8[character][cnt];

    chrvrtar_fp8[character][cnt] = MIN( 255, 0.9 * lightold_r + 0.1 * lightnew_r );
    chrvrtag_fp8[character][cnt] = MIN( 255, 0.9 * lightold_g + 0.1 * lightnew_g );
    chrvrtab_fp8[character][cnt] = MIN( 255, 0.9 * lightold_b + 0.1 * lightnew_b );

    v[cnt].col.r = FP8_TO_FLOAT( chrvrtar_fp8[character][cnt] ) / ( float )( 1 << rs );
    v[cnt].col.g = FP8_TO_FLOAT( chrvrtag_fp8[character][cnt] ) / ( float )( 1 << gs );
    v[cnt].col.b = FP8_TO_FLOAT( chrvrtab_fp8[character][cnt] ) / ( float )( 1 << bs );
    v[cnt].col.a = FP8_TO_FLOAT( trans );
  }

  // Do fog...
  /*
  if(fogon && chrlight_fp8[character]==255)
  {
  // The full fog value
  alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

  for (cnt = 0; cnt < madtransvertices[model]; cnt++)
  {
  // Figure out the z position of the vertex...  Not totally accurate
  z = (v[cnt].pos.z * chrscale[character]) + chrmatrix[character](3,2);

  // Figure out the fog coloring
  if(z < fogtop)
  {
  if(z < fogbottom)
  {
  v[cnt].specular = alpha;
  }
  else
  {
  z = 1.0 - ((z - fogbottom)/fogdistance);  // 0.0 to 1.0...  Amount of fog to keep
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

  ATTRIB_PUSH( "render_enviromad", GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[character]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[character].v );
    chrmatrix[character]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    glEnable( GL_TEXTURE_GEN_S );     // Enable Texture Coord Generation For S (NEW)
    glEnable( GL_TEXTURE_GEN_T );     // Enable Texture Coord Generation For T (NEW)

    // Render each command
    entry = 0;
    for ( cnt = 0; cnt < madcommands[model]; cnt++ )
    {
      glBegin( madcommandtype[model][cnt] );

      for ( tnc = 0; tnc < madcommandsize[model][cnt]; tnc++ )
      {
        vertex = madcommandvrt[model][entry];

        glColor4fv( v[vertex].col.v );
        glNormal3f( -kMd2Normals[madvrta[lastframe][vertex]][0], kMd2Normals[madvrta[lastframe][vertex]][1], kMd2Normals[madvrta[lastframe][vertex]][2] );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }

    glPopMatrix();
  }
  ATTRIB_POP( "render_enviromad" );
}




//--------------------------------------------------------------------------------------------
void render_texmad( CHR_REF character, Uint8 trans )
{
  // ZZ> This function draws a model

  GLVertex v[MAXVERTICES];
  float ftmp;
  Uint16 cnt, tnc, entry;
  Uint16 vertex;

  Uint8  sheen_fp8 = chrsheen_fp8[character];
  float  spekularity  = (( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL );
  Uint16 model        = chrmodel[character];
  Uint16 texture      = chrtexture[character];
  Uint16 frame        = chrframe[character];
  Uint16 lastframe    = chrlastframe[character];
  float flip           = chrflip[character];
  Uint8 lightrotationr = ( chrturn_lr[character] + chrlightturn_lrr[character] ) >> 8;
  Uint8 lightrotationg = ( chrturn_lr[character] + chrlightturn_lrg[character] ) >> 8;
  Uint8 lightrotationb = ( chrturn_lr[character] + chrlightturn_lrb[character] ) >> 8;
  Uint8 ambilevelr_fp8 = chrlightambir_fp8[character];
  Uint8 ambilevelg_fp8 = chrlightambig_fp8[character];
  Uint8 ambilevelb_fp8 = chrlightambib_fp8[character];
  Uint8 speklevelr_fp8 = chrlightspekr_fp8[character];
  Uint8 speklevelg_fp8 = chrlightspekg_fp8[character];
  Uint8 speklevelb_fp8 = chrlightspekb_fp8[character];



  float uoffset = textureoffset[ FP8_TO_INT( chruoffset_fp8[character] )];
  float voffset = textureoffset[ FP8_TO_INT( chrvoffset_fp8[character] )];
  Uint8 rs = chrredshift[character];
  Uint8 gs = chrgrnshift[character];
  Uint8 bs = chrblushift[character];
  Uint16 lightold_r, lightold_g, lightold_b;
  Uint16 lightnew_r, lightnew_g, lightnew_b;

  // Original points with linear interpolation
  for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
  {
    if ( frame == lastframe )
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt];
      v[cnt].pos.y = madvrty[lastframe][cnt];
      v[cnt].pos.z = madvrtz[lastframe][cnt];

      v[cnt].nrm.x = kMd2Normals[madvrta[lastframe][cnt]][0];
      v[cnt].nrm.y = kMd2Normals[madvrta[lastframe][cnt]][1];
      v[cnt].nrm.z = kMd2Normals[madvrta[lastframe][cnt]][2];
    }
    else
    {
      v[cnt].pos.x = madvrtx[lastframe][cnt] + ( madvrtx[frame][cnt] - madvrtx[lastframe][cnt] ) * flip;
      v[cnt].pos.y = madvrty[lastframe][cnt] + ( madvrty[frame][cnt] - madvrty[lastframe][cnt] ) * flip;
      v[cnt].pos.z = madvrtz[lastframe][cnt] + ( madvrtz[frame][cnt] - madvrtz[lastframe][cnt] ) * flip;

      v[cnt].nrm.x = kMd2Normals[madvrta[lastframe][cnt]][0] + ( kMd2Normals[madvrta[frame][cnt]][0] - kMd2Normals[madvrta[lastframe][cnt]][0] ) * flip;
      v[cnt].nrm.y = kMd2Normals[madvrta[lastframe][cnt]][1] + ( kMd2Normals[madvrta[frame][cnt]][1] - kMd2Normals[madvrta[lastframe][cnt]][1] ) * flip;
      v[cnt].nrm.z = kMd2Normals[madvrta[lastframe][cnt]][2] + ( kMd2Normals[madvrta[frame][cnt]][2] - kMd2Normals[madvrta[lastframe][cnt]][2] ) * flip;
    }

    v[cnt].nrm.x *= -1;

    lightnew_r = 0;
    lightnew_g = 0;
    lightnew_b = 0;
    ftmp = DotProduct( v[cnt].nrm, lightspekdir );
    if ( ftmp > 0.0f )
    {
      lightnew_r += ftmp * ftmp * spekularity * lightspekcol.x;
      lightnew_g += ftmp * ftmp * spekularity * lightspekcol.y;
      lightnew_b += ftmp * ftmp * spekularity * lightspekcol.z;
    }
    lightnew_r += speklevelr_fp8 * spek_local[lightrotationr][madvrta[frame][cnt]];
    lightnew_g += speklevelg_fp8 * spek_local[lightrotationg][madvrta[frame][cnt]];
    lightnew_b += speklevelb_fp8 * spek_local[lightrotationb][madvrta[frame][cnt]];

    lightnew_r = lighttospek[sheen_fp8][lightnew_r] + ambilevelr_fp8 + lightambicol.x * 255;
    lightnew_g = lighttospek[sheen_fp8][lightnew_g] + ambilevelg_fp8 + lightambicol.y * 255;
    lightnew_b = lighttospek[sheen_fp8][lightnew_b] + ambilevelb_fp8 + lightambicol.z * 255;

    lightold_r = chrvrtar_fp8[character][cnt];
    lightold_g = chrvrtag_fp8[character][cnt];
    lightold_b = chrvrtab_fp8[character][cnt];

    chrvrtar_fp8[character][cnt] = MIN( 255, 0.9 * lightold_r + 0.1 * lightnew_r );
    chrvrtag_fp8[character][cnt] = MIN( 255, 0.9 * lightold_g + 0.1 * lightnew_g );
    chrvrtab_fp8[character][cnt] = MIN( 255, 0.9 * lightold_b + 0.1 * lightnew_b );

    v[cnt].col.r = FP8_TO_FLOAT( FP8_MUL( chrvrtar_fp8[character][cnt] >> rs, trans ) );
    v[cnt].col.g = FP8_TO_FLOAT( FP8_MUL( chrvrtag_fp8[character][cnt] >> gs, trans ) );
    v[cnt].col.b = FP8_TO_FLOAT( FP8_MUL( chrvrtab_fp8[character][cnt] >> bs, trans ) );
    v[cnt].col.a = 1.0f;
  }


  /*
  // Do fog...
  if(fogon && chrlight_fp8[character]==255)
  {
  // The full fog value
  alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

  for (cnt = 0; cnt < madtransvertices[model]; cnt++)
  {
  // Figure out the z position of the vertex...  Not totally accurate
  z = (v[cnt].pos.z * chrscale[character]) + chrmatrix[character](3,2);

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
  z = (z - fogbottom)/fogdistance;  // 0.0 to 1.0...  Amount of old to keep
  fogtokeep = 1.0-z;  // 0.0 to 1.0...  Amount of fog to keep
  spek *= z;
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

  ATTRIB_PUSH( "render_texmad", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[character]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[character].v );
    chrmatrix[character]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    // Render each command
    entry = 0;
    for ( cnt = 0; cnt < madcommands[model]; cnt++ )
    {
      glBegin( madcommandtype[model][cnt] );

      for ( tnc = 0; tnc < madcommandsize[model][cnt]; tnc++ )
      {
        vertex = madcommandvrt[model][entry];

        glColor4fv( v[vertex].col.v );
        glTexCoord2f( madcommandu[model][entry] + uoffset, madcommandv[model][entry] + voffset );
        glVertex3fv( v[vertex].pos.v );
        glNormal3fv( v[vertex].nrm.v );

        entry++;
      }
      glEnd();
    }

#ifdef DEBUG_NORMALS
    if ( CData.DevMode )
    {
      glBegin( GL_LINES );
      glLineWidth( 2.0f );
      glColor4f( 1, 1, 1, 1 );
      for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
      {
        glVertex3fv( v[cnt].pos.v );
        glVertex3f( v[cnt].pos.x + v[cnt].nrm.x*10, v[cnt].pos.y + v[cnt].nrm.y*10, v[cnt].pos.z + v[cnt].nrm.z*10 );
      };
      glEnd();
    }
#endif

    glPopMatrix();
  }
  ATTRIB_POP( "render_texmad" );

}

//--------------------------------------------------------------------------------------------
void render_mad( CHR_REF character, Uint8 trans )
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
void render_refmad( int ichr, Uint8 trans_fp8 )
{
  int alphatmp_fp8;
  float level = chrlevel[ichr];
  float zpos = ( chrmatrix[ichr] ) _CNV( 3, 2 ) - level;
  int   model = chrmodel[ichr];
  Uint16 lastframe = chrlastframe[ichr];
  Uint8 sheensave;
  bool_t fog_save;

  // ZZ> This function draws characters reflected in the floor
  if ( !capreflect[chrmodel[ichr]] ) return;

  alphatmp_fp8 = trans_fp8 - zpos * 0.5f;
  if ( alphatmp_fp8 <= 0 )   return;
  if ( alphatmp_fp8 > 255 ) alphatmp_fp8 = 255;

  sheensave = chrsheen_fp8[ichr];
  chrredshift[ichr] += 1;
  chrgrnshift[ichr] += 1;
  chrblushift[ichr] += 1;
  chrsheen_fp8[ichr] >>= 1;
  ( chrmatrix[ichr] ) _CNV( 0, 2 ) = - ( chrmatrix[ichr] ) _CNV( 0, 2 );
  ( chrmatrix[ichr] ) _CNV( 1, 2 ) = - ( chrmatrix[ichr] ) _CNV( 1, 2 );
  ( chrmatrix[ichr] ) _CNV( 2, 2 ) = - ( chrmatrix[ichr] ) _CNV( 2, 2 );
  ( chrmatrix[ichr] ) _CNV( 3, 2 ) = - ( chrmatrix[ichr] ) _CNV( 3, 2 ) + level + level;
  fog_save = fogon;
  fogon    = bfalse;

  render_mad( ichr, alphatmp_fp8 );

  fogon = fog_save;
  ( chrmatrix[ichr] ) _CNV( 0, 2 ) = - ( chrmatrix[ichr] ) _CNV( 0, 2 );
  ( chrmatrix[ichr] ) _CNV( 1, 2 ) = - ( chrmatrix[ichr] ) _CNV( 1, 2 );
  ( chrmatrix[ichr] ) _CNV( 2, 2 ) = - ( chrmatrix[ichr] ) _CNV( 2, 2 );
  ( chrmatrix[ichr] ) _CNV( 3, 2 ) = - ( chrmatrix[ichr] ) _CNV( 3, 2 ) + level + level;
  chrsheen_fp8[ichr] = sheensave;
  chrredshift[ichr] -= 1;
  chrgrnshift[ichr] -= 1;
  chrblushift[ichr] -= 1;

}

//#if 0
////--------------------------------------------------------------------------------------------
//void render_texmad(CHR_REF character, Uint8 trans)
//{
//  Md2Model *model;
//  Uint16 texture;
//  int frame0, frame1;
//  float lerp;
//  GLMatrix mTempWorld;
//
//  // Grab some basic information about the model
//  model = md2_models[chrmodel[character]];
//  texture = chrtexture[character];
//  frame0 = chrframe[character];
//  frame1 = chrframe[character];
//  lerp = chrflip;
//
//  mTempWorld = mWorld;
//  /*
//  // Lighting information
//  Uint8 lightrotation =
//  (chrturn_lr[character]+chrlightturn_lr[character])>>8;
//  Uint8 speklevel_fp8 = chrlightspek_fp8[character];
//  Uint32 alpha = trans<<24;
//  Uint8 spek = chrsheen_fp8[character];
//
//  float uoffset = textureoffset[ FP8_TO_INT(chruoffset_fp8[character]) ];
//  float voffset = textureoffset[ FP8_TO_INT(chrvoffset_fp8[character]) ];
//  Uint8 rs = chrredshift[character];
//  Uint8 gs = chrgrnshift[character];
//  Uint8 bs = chrblushift[character];
//
//
//  if(CData.phongon && trans == 255)
//  spek = 0;
//  */
//
//  // Choose texture and matrix
//  if(SDLKEYDOWN(SDLK_F7))
//  {
//    glBindTexture( GL_TEXTURE_2D, -1 );
//  }
//  else
//  {
//    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
//  }
//
//  mWorld = chrmatrix[character];
//
//  glLoadMatrixf(mView.v);
//  glMultMatrixf(mWorld.v);
//
//  draw_textured_md2(character, model, frame0, frame1, lerp);
//
//
//  mWorld = mTempWorld;
//  glLoadMatrixf(mView.v);
//  glMultMatrixf(mWorld.v);
//}
//#endif
