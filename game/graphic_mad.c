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
#include <assert.h>

float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];

float spek_global_lighting( int rotation, int normal, vect3 lite );
float spek_local_lighting( int rotation, int normal );
float spek_calc_local_lighting( Uint16 turn, vect3 nrm );
void make_speklut();
void make_spektable( vect3 lite );
void make_lighttospek( void );

void draw_Chr_BBox(CHR_REF ichr);
void draw_CVolume( CVolume * cv );

//---------------------------------------------------------------------------------------------
// md2_blend_vertices
// Blends the vertices and normals between 2 frames of a md2 model for animation.
//
// NOTE: Only meant to be called from draw_textured_md2, which does the necessary
// checks to make sure that the inputs are valid.  So this function itself assumes
// that they are valid.  User beware!
//
void md2_blend_vertices(CHR_REF ichr, Sint32 vrtmin, Sint32 vrtmax)
{
  const MD2_Frame *pfrom, *pto;
  Sint32 numVertices, i;
  vect2  off;
  Uint16 imdl;
  MD2_Model * pmdl;
  float lerp;

  if( !VALID_CHR(ichr) ) return;

  imdl = chrmodel[ichr];
  if( !VALID_MDL(imdl) ) return;

  pmdl = mad_md2[imdl];
  if(NULL == pmdl) return;

  // handle "default" argument
  if(vrtmin<0)
  {
    vrtmin = 0;
  };

  // handle "default" argument
  if(vrtmax<0)
  {
    vrtmax = md2_get_numVertices(pmdl);
  }

  // check pto see if the blend is already cached
  if( (chrframelast[ichr]  == chrvdata[ichr].frame0) && 
      (chrframe[ichr] == chrvdata[ichr].frame1) && 
      (vrtmin         >= chrvdata[ichr].vrtmin) &&
      (vrtmax         <= chrvdata[ichr].vrtmax) &&
      (chrflip[ichr]  == chrvdata[ichr].lerp))
      return;

  
  pfrom  = md2_get_Frame(pmdl, chrframelast[ichr]);
  pto    = md2_get_Frame(pmdl, chrframe[ichr]);
  lerp  = chrflip[ichr];

  off.u = FP8_TO_FLOAT(chruoffset_fp8[ichr]);
  off.v = FP8_TO_FLOAT(chrvoffset_fp8[ichr]);

  // do some error trapping
  if(NULL==pfrom && NULL==pto) return;
  else if(NULL==pfrom) lerp = 1.0f;
  else if(NULL==pto  ) lerp = 0.0f;
  else if(pfrom==pto  ) lerp = 0.0f;

  // do the interpolating
  numVertices = md2_get_numVertices(pmdl);

  if (lerp <= 0)
  {
    // copy the vertices in frame 'pfrom' over
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      chrvdata[ichr].Vertices[i].x = pfrom->vertices[i].x;
      chrvdata[ichr].Vertices[i].y = pfrom->vertices[i].y;
      chrvdata[ichr].Vertices[i].z = pfrom->vertices[i].z;

      chrvdata[ichr].Vertices[i].x *= -1;

      chrvdata[ichr].Vertices[i].x  *= 4.125;
      chrvdata[ichr].Vertices[i].y  *= 4.125;
      chrvdata[ichr].Vertices[i].z  *= 4.125;

      chrvdata[ichr].Normals[i].x = kMd2Normals[pfrom->vertices[i].normal][0];
      chrvdata[ichr].Normals[i].y = kMd2Normals[pfrom->vertices[i].normal][1];
      chrvdata[ichr].Normals[i].z = kMd2Normals[pfrom->vertices[i].normal][2];
      
      chrvdata[ichr].Normals[i].x *= -1;

      if(chrenviro[ichr])
      {
        chrvdata[ichr].Texture[i].s  = off.u + CLIP((1-chrvdata[ichr].Normals[i].z)/2.0f,0,1);
        chrvdata[ichr].Texture[i].t  = off.v + atan2(chrvdata[ichr].Normals[i].y, chrvdata[ichr].Normals[i].x)*INV_TWO_PI - chrturn_lr[ichr]/(float)(1<<16);
        chrvdata[ichr].Texture[i].t -= atan2(chrpos[ichr].y-campos.y, chrpos[ichr].x-campos.x)*INV_TWO_PI;
      };
    }
  }
  else if (lerp >= 1.0f)
  {
    // copy the vertices in frame 'pto'
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      chrvdata[ichr].Vertices[i].x = pto->vertices[i].x;
      chrvdata[ichr].Vertices[i].y = pto->vertices[i].y;
      chrvdata[ichr].Vertices[i].z = pto->vertices[i].z;

      chrvdata[ichr].Vertices[i].x  *= -1;

      chrvdata[ichr].Vertices[i].x  *= 4.125;
      chrvdata[ichr].Vertices[i].y  *= 4.125;
      chrvdata[ichr].Vertices[i].z  *= 4.125;

      chrvdata[ichr].Normals[i].x = kMd2Normals[pto->vertices[i].normal][0];
      chrvdata[ichr].Normals[i].y = kMd2Normals[pto->vertices[i].normal][1];
      chrvdata[ichr].Normals[i].z = kMd2Normals[pto->vertices[i].normal][2];

      chrvdata[ichr].Normals[i].x  *= -1;

      if(chrenviro[ichr])
      {
        chrvdata[ichr].Texture[i].s  = off.u + CLIP((1-chrvdata[ichr].Normals[i].z)/2.0f,0,1);
        chrvdata[ichr].Texture[i].t  = off.v + atan2(chrvdata[ichr].Normals[i].y, chrvdata[ichr].Normals[i].x )*INV_TWO_PI - chrturn_lr[ichr]/(float)(1<<16);
        chrvdata[ichr].Texture[i].t -= atan2(chrpos[ichr].y-campos.y, chrpos[ichr].x-campos.x )*INV_TWO_PI;
      };
    }
  }
  else
  {
    // mix the vertices
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      chrvdata[ichr].Vertices[i].x = pfrom->vertices[i].x + (pto->vertices[i].x - pfrom->vertices[i].x) * lerp;
      chrvdata[ichr].Vertices[i].y = pfrom->vertices[i].y + (pto->vertices[i].y - pfrom->vertices[i].y) * lerp;
      chrvdata[ichr].Vertices[i].z = pfrom->vertices[i].z + (pto->vertices[i].z - pfrom->vertices[i].z) * lerp;

      chrvdata[ichr].Vertices[i].x  *= -1;

      chrvdata[ichr].Vertices[i].x  *= 4.125;
      chrvdata[ichr].Vertices[i].y  *= 4.125;
      chrvdata[ichr].Vertices[i].z  *= 4.125;

      chrvdata[ichr].Normals[i].x = kMd2Normals[pfrom->vertices[i].normal][0] + (kMd2Normals[pto->vertices[i].normal][0] - kMd2Normals[pfrom->vertices[i].normal][0]) * lerp;
      chrvdata[ichr].Normals[i].y = kMd2Normals[pfrom->vertices[i].normal][1] + (kMd2Normals[pto->vertices[i].normal][1] - kMd2Normals[pfrom->vertices[i].normal][1]) * lerp;
      chrvdata[ichr].Normals[i].z = kMd2Normals[pfrom->vertices[i].normal][2] + (kMd2Normals[pto->vertices[i].normal][2] - kMd2Normals[pfrom->vertices[i].normal][2]) * lerp;

      chrvdata[ichr].Normals[i].x  *= -1;

      if(chrenviro[ichr])
      {
        chrvdata[ichr].Texture[i].s  = off.u + CLIP((1-chrvdata[ichr].Normals[i].z)/2.0f,0,1);
        chrvdata[ichr].Texture[i].t  = off.v + atan2(chrvdata[ichr].Normals[i].y, chrvdata[ichr].Normals[i].x)*INV_TWO_PI - chrturn_lr[ichr]/(float)(1<<16);
        chrvdata[ichr].Texture[i].t -= atan2(chrpos[ichr].y-campos.y, chrpos[ichr].x-campos.x)*INV_TWO_PI;
      };
    }
  }


  // cache the blend parameters
  chrvdata[ichr].frame0 = chrframelast[ichr];
  chrvdata[ichr].frame1 = chrframe[ichr];
  chrvdata[ichr].vrtmin = vrtmin;
  chrvdata[ichr].vrtmax = vrtmax;
  chrvdata[ichr].lerp   = chrflip[ichr];
  chrvdata[ichr].needs_lighting = btrue;

  // invalidate the cached bumper data
  chrbmpdata[ichr].cv.level = -1;
}

//---------------------------------------------------------------------------------------------
void md2_blend_lighting(CHR_REF ichr)
{
  Uint16  sheen_fp8, spekularity_fp8;
  Uint16 lightnew_r, lightnew_g, lightnew_b;
  Uint16 lightold_r, lightold_g, lightold_b;
  Uint16 lightrotationr, lightrotationg, lightrotationb;
  Uint8 ambilevelr_fp8, ambilevelg_fp8, ambilevelb_fp8;
  Uint8  speklevelr_fp8, speklevelg_fp8, speklevelb_fp8;
  Uint8  rs, gs, bs;
  Uint16 trans;
  vect2  offset;

  VData_Blended * vd;
  MD2_Model * model;
  Uint32 i, numVertices;

  if( !VALID_CHR(ichr) ) return;

  vd = &chrvdata[ichr];
  if(!vd->needs_lighting) return;

  model = mad_md2[ chrmodel[ichr]];
  if(NULL==model) return;

  sheen_fp8       = chrsheen_fp8[ichr];
  spekularity_fp8 = FLOAT_TO_FP8( ( float )sheen_fp8 / ( float )MAXSPEKLEVEL );

  lightrotationr = chrturn_lr[ichr] + chrlightturn_lrr[ichr];
  lightrotationg = chrturn_lr[ichr] + chrlightturn_lrg[ichr];
  lightrotationb = chrturn_lr[ichr] + chrlightturn_lrb[ichr];

  ambilevelr_fp8 = chrlightambir_fp8[ichr];
  ambilevelg_fp8 = chrlightambig_fp8[ichr];
  ambilevelb_fp8 = chrlightambib_fp8[ichr];

  speklevelr_fp8 = chrlightspekr_fp8[ichr];
  speklevelg_fp8 = chrlightspekg_fp8[ichr];
  speklevelb_fp8 = chrlightspekb_fp8[ichr];

  offset.u = textureoffset[ FP8_TO_INT( chruoffset_fp8[ichr] )];
  offset.v = textureoffset[ FP8_TO_INT( chrvoffset_fp8[ichr] )];

  rs = chrredshift[ichr];
  gs = chrgrnshift[ichr];
  bs = chrblushift[ichr];

  trans = capalpha_fp8[chrmodel[ichr]];

  numVertices = md2_get_numVertices(model);
  if(CData.shading != GL_FLAT)
  {

    // mix the vertices
    for (i=0; i<numVertices; i++)
    {
      float ftmp;

      lightnew_r = 0;
      lightnew_g = 0;
      lightnew_b = 0;

      ftmp = DotProduct( vd->Normals[i], lightspekdir );
      if ( ftmp > 0.0f )
      {
        lightnew_r += ftmp * ftmp * spekularity_fp8 * lightspekcol.r;
        lightnew_g += ftmp * ftmp * spekularity_fp8 * lightspekcol.g;
        lightnew_b += ftmp * ftmp * spekularity_fp8 * lightspekcol.b;
      }

      lightnew_r += speklevelr_fp8 * spek_calc_local_lighting(lightrotationr, vd->Normals[i]);
      lightnew_g += speklevelg_fp8 * spek_calc_local_lighting(lightrotationg, vd->Normals[i]);
      lightnew_b += speklevelb_fp8 * spek_calc_local_lighting(lightrotationb, vd->Normals[i]);

      lightnew_r = lighttospek[sheen_fp8][lightnew_r] + ambilevelr_fp8 + lightambicol.r * 255;
      lightnew_g = lighttospek[sheen_fp8][lightnew_g] + ambilevelg_fp8 + lightambicol.g * 255;
      lightnew_b = lighttospek[sheen_fp8][lightnew_b] + ambilevelb_fp8 + lightambicol.b * 255;

      lightold_r = vd->Colors[i].r;
      lightold_g = vd->Colors[i].g;
      lightold_b = vd->Colors[i].b;

      vd->Colors[i].r = MIN( 255, 0.9 * lightold_r + 0.1 * lightnew_r );
      vd->Colors[i].g = MIN( 255, 0.9 * lightold_g + 0.1 * lightnew_g );
      vd->Colors[i].b = MIN( 255, 0.9 * lightold_b + 0.1 * lightnew_b );
      vd->Colors[i].a = 1.0f;

      vd->Colors[i].r /= (float)(1 << rs) * FP8_TO_FLOAT(trans);
      vd->Colors[i].g /= (float)(1 << gs) * FP8_TO_FLOAT(trans);
      vd->Colors[i].b /= (float)(1 << bs) * FP8_TO_FLOAT(trans);
    }
  }
  else
  {
    for (i=0; i<numVertices; i++)
    {
      vd->Colors[i].r  = FP8_TO_FLOAT(ambilevelr_fp8);
      vd->Colors[i].g  = FP8_TO_FLOAT(ambilevelg_fp8);
      vd->Colors[i].b  = FP8_TO_FLOAT(ambilevelb_fp8);
      vd->Colors[i].a  = 1.0;

      vd->Colors[i].r /= (float)(1 << rs) * FP8_TO_FLOAT(trans);
      vd->Colors[i].g /= (float)(1 << gs) * FP8_TO_FLOAT(trans);
      vd->Colors[i].b /= (float)(1 << bs) * FP8_TO_FLOAT(trans);
    };

  };

  vd->needs_lighting = bfalse;
}



/* md2_blend_vertices
* Blends the vertices and normals between 2 frames of a md2 model for animation.
*
* NOTE: Only meant to be called from draw_textured_md2, which does the necessary
* checks to make sure that the inputs are valid.  So this function itself assumes
* that they are valid.  User beware!
*/

//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_textured_md2_opengl(CHR_REF ichr)
{
  const MD2_GLCommand * cmd;
  Uint32 cmd_count;
  vect2  off;

  Uint16          imdl = chrmodel[ichr]; 
  VData_Blended * vd   = &chrvdata[ichr];
  MD2_Model     * pmdl = mad_md2[imdl];

  md2_blend_vertices(ichr, -1, -1);
  md2_blend_lighting(ichr);
  
  off.u = FP8_TO_FLOAT(chruoffset_fp8[ichr]);
  off.v = FP8_TO_FLOAT(chrvoffset_fp8[ichr]);

  if(CData.shading != GL_FLAT)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(   GL_FLOAT, 0, vd->Normals[0].v);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vd->Vertices[0].v);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, vd->Colors[0].v);

  cmd  = md2_get_Commands(pmdl);
  for (/*nothing */; NULL!=cmd; cmd = cmd->next)
  {
    Uint32 i;
    glBegin(cmd->gl_mode);

    cmd_count = cmd->command_count;
    for(i=0; i<cmd_count; i++)
    {
      glTexCoord2f( cmd->data[i].s + off.u, cmd->data[i].t + off.v );
      glArrayElement( cmd->data[i].index );
    }
    glEnd();
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  if(CData.shading != GL_FLAT) glDisableClientState(GL_NORMAL_ARRAY);


#if defined(DEBUG_NORMALS) && defined(_DEBUG)
  if ( CData.DevMode )
  {
    int verts = madtransvertices[imdl];

    glBegin( GL_LINES );
    glLineWidth( 2.0f );
    glColor4f( 1, 1, 1, 1 );
    for ( cnt = 0; cnt < verts; cnt++ )
    {
      glVertex3fv( vd->Vertices[cnt].v );
      glVertex3f( vd->Vertices[cnt].x + vd->Normals[cnt].x*10, vd->Vertices[cnt].y + vd->Normals[cnt].y*10, vd->Vertices[cnt].z + vd->Normals[cnt].z*10 );
    };
    glEnd();
  }
#endif

}

//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_enviromapped_md2_opengl(CHR_REF ichr)
{
  const MD2_GLCommand * cmd;
  Uint32 cmd_count;

  Uint16          imdl = chrmodel[ichr];
  VData_Blended * vd   = &chrvdata[ichr];
  MD2_Model     * pmdl = mad_md2[imdl];

  md2_blend_vertices(ichr, -1, -1);
  md2_blend_lighting(ichr);

  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer( GL_FLOAT, 0, vd->Normals[0].v );

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vd->Vertices[0].v);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, vd->Colors[0].v);

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, vd->Texture[0]._v);

  cmd = md2_get_Commands(pmdl);
  for (/*nothing */; NULL!=cmd; cmd = cmd->next)
  {
    Uint32 i;
    glBegin(cmd->gl_mode);

    cmd_count = cmd->command_count;
    for(i=0; i<cmd_count; i++)
    {
      glArrayElement( cmd->data[i].index );
    }
    glEnd();
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);

#if defined(DEBUG_NORMALS) && defined(_DEBUG)
  if ( CData.DevMode )
  {
    int verts = madtransvertices[imdl];

    glBegin( GL_LINES );
    glLineWidth( 2.0f );
    glColor4f( 1, 1, 1, 1 );
    for ( cnt = 0; cnt < verts; cnt++ )
    {
      glVertex3fv( vd->Vertices[cnt].v );
      glVertex3f( vd->Vertices[cnt].x + vd->Normals[cnt].x*10, vd->Vertices[cnt].y + vd->Normals[cnt].y*10, vd->Vertices[cnt].z + vd->Normals[cnt].z*10 );
    };
    glEnd();
  }
#endif
}

//--------------------------------------------------------------------------------------------
void calc_lighting_data( CHR_REF ichr )
{
  Uint8  sheen_fp8       = chrsheen_fp8[ichr];
  float  spekularity_fp8 = FLOAT_TO_FP8(( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL );
  Uint16 model           = chrmodel[ichr];
  Uint16 texture         = chrtexture[ichr];

  Uint8 rs = chrredshift[ichr];
  Uint8 gs = chrgrnshift[ichr];
  Uint8 bs = chrblushift[ichr];

  float ftmp;


  ftmp = (( float )( MAXSPEKLEVEL - sheen_fp8 ) / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( chralpha_fp8[ichr] ) );
  chrldata[ichr].diffuse.r = ftmp * FP8_TO_FLOAT( chralpha_fp8[ichr] ) / ( float )( 1 << rs );
  chrldata[ichr].diffuse.g = ftmp * FP8_TO_FLOAT( chralpha_fp8[ichr] ) / ( float )( 1 << gs );
  chrldata[ichr].diffuse.b = ftmp * FP8_TO_FLOAT( chralpha_fp8[ichr] ) / ( float )( 1 << bs );


  ftmp = (( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( chralpha_fp8[ichr] ) );
  chrldata[ichr].specular.r = ftmp / ( float )( 1 << rs );
  chrldata[ichr].specular.g = ftmp / ( float )( 1 << gs );
  chrldata[ichr].specular.b = ftmp / ( float )( 1 << bs );

  chrldata[ichr].shininess[0] = sheen_fp8 + 2;

  if ( 255 != chrlight_fp8[ichr] )
  {
    ftmp = FP8_TO_FLOAT( chrlight_fp8[ichr] );
    chrldata[ichr].emission.r = ftmp / ( float )( 1 << rs );
    chrldata[ichr].emission.g = ftmp / ( float )( 1 << gs );
    chrldata[ichr].emission.b = ftmp / ( float )( 1 << bs );
  }
}

//--------------------------------------------------------------------------------------------
void render_mad_lit( CHR_REF ichr )
{
  // ZZ> This function draws an environment mapped model

  Uint16 texture;
  GLfloat mat_none[4] = {0,0,0,0};

  if( !VALID_CHR(ichr) ) return;

  calc_lighting_data(ichr);

  glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,  chrldata[ichr].specular.v );
  glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, chrldata[ichr].shininess );
  glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,  chrldata[ichr].emission.v );

  ATTRIB_PUSH( "render_mad_lit", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[ichr]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[ichr].v );
    chrmatrix[ichr]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    if (SDLKEYDOWN(SDLK_F7))
    {
      glBindTexture(GL_TEXTURE_2D, -1);
    }
    else
    {
      texture = chrtexture[ichr];
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    }

    draw_textured_md2_opengl(ichr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

  }
  ATTRIB_POP( "render_mad_lit" );

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    draw_Chr_BBox(ichr);
  }
#endif

}

//--------------------------------------------------------------------------------------------
void render_texmad(CHR_REF ichr, Uint8 trans)
{
  Uint16 texture;
  if(!VALID_CHR(ichr)) return;

  texture = chrtexture[ichr];

  md2_blend_vertices(ichr, -1, -1);
  md2_blend_lighting(ichr);

  ATTRIB_PUSH( "render_texmad", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[ichr]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[ichr].v );
    chrmatrix[ichr]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    if (SDLKEYDOWN(SDLK_F7))
    {
      glBindTexture(GL_TEXTURE_2D, -1);
    }
    else
    {
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    }

    draw_textured_md2_opengl(ichr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  ATTRIB_POP( "render_texmad" );

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    draw_Chr_BBox(ichr);
  }
#endif
}


//--------------------------------------------------------------------------------------------
void render_enviromad(CHR_REF ichr, Uint8 trans)
{
  Uint16 texture;

  if(!VALID_CHR(ichr)) return;

  texture = chrtexture[ichr];

  ATTRIB_PUSH( "render_enviromad", GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    chrmatrix[ichr]_CNV( 3, 2 ) += RAISE;
    glMultMatrixf( chrmatrix[ichr].v );
    chrmatrix[ichr]_CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    if ( SDLKEYDOWN(SDLK_F7) )
    {
      glBindTexture(GL_TEXTURE_2D, -1);
    }
    else
    {
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    }

    glEnable( GL_TEXTURE_GEN_S );     // Enable Texture Coord Generation For S (NEW)
    glEnable( GL_TEXTURE_GEN_T );     // Enable Texture Coord Generation For T (NEW)

    draw_enviromapped_md2_opengl(ichr);

    glDisable( GL_TEXTURE_GEN_S );     // Enable Texture Coord Generation For S (NEW)
    glDisable( GL_TEXTURE_GEN_T );     // Enable Texture Coord Generation For T (NEW)


    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  ATTRIB_POP( "render_enviromad" );
}

//--------------------------------------------------------------------------------------------
void render_mad( CHR_REF ichr, Uint8 trans )
{
  // ZZ> This function picks the actual function to use
  Sint8 hide = caphidestate[chrmodel[ichr]];

  if ( hide == NOHIDE || hide != chraistate[ichr] )
  {
    if ( chrenviro[ichr] )
      render_enviromad( ichr, trans );
    else
      render_texmad( ichr, trans );
  }
}

//--------------------------------------------------------------------------------------------
void render_refmad( int ichr, Uint8 trans_fp8 )
{
  int alphatmp_fp8;
  float level = chrlevel[ichr];
  float zpos = ( chrmatrix[ichr] ) _CNV( 3, 2 ) - level;
  int   model = chrmodel[ichr];
  Uint16 lastframe = chrframelast[ichr];
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

//---------------------------------------------------------------------------------------------
float spek_global_lighting( int rotation, int normal, vect3 lite )
{
  // ZZ> This function helps make_spektable
  float fTmp, flite;
  vect3 nrm;
  float sinrot, cosrot;

  nrm.x = -kMd2Normals[normal][0];
  nrm.y = kMd2Normals[normal][1];
  nrm.z = kMd2Normals[normal][2];

  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];
  fTmp   = cosrot * nrm.x + sinrot * nrm.y;
  nrm.y = cosrot * nrm.y - sinrot * nrm.x;
  nrm.x = fTmp;

  fTmp = DotProduct( nrm, lite );
  flite = 0.0f;
  if ( fTmp > 0 ) flite = fTmp * fTmp;

  return flite;
}

//---------------------------------------------------------------------------------------------
float spek_local_lighting( int rotation, int normal )
{
  // ZZ> This function helps make_spektable
  float fTmp, fLite;
  vect3 nrm;
  float sinrot, cosrot;

  nrm.x = -kMd2Normals[normal][0];
  nrm.y =  kMd2Normals[normal][1];

  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];

  fTmp = cosrot * nrm.x + sinrot * nrm.y;

  fLite = 0.0f;
  if ( fTmp > 0.0f )  fLite = fTmp * fTmp;

  return fLite;
}

//---------------------------------------------------------------------------------------------
float spek_calc_local_lighting( Uint16 turn, vect3 nrm )
{
  // ZZ> This function helps make_spektable
  float fTmp1, fTmp2, fLite;
  Uint16 turn_sin, turn_cos;
  float sinrot, cosrot;

  turn_sin = turn >> 2;
  turn_cos = ( turn_sin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;

  sinrot = turntosin[turn_sin];
  cosrot = turntosin[turn_cos];

  fTmp1 = cosrot * nrm.x + sinrot * nrm.y;
  fTmp2 = nrm.x * nrm.x + nrm.y * nrm.y;

 
  if(fTmp2 == 0.0f)
  {
    fLite = 1.0f;
  }
  else if ( fTmp1 > 0.0f )
  {
    fLite = fTmp1 * fTmp1 / fTmp2;
  }
  else
  {
    fLite = 0.0f;
  }

  return fLite;
}


//---------------------------------------------------------------------------------------------
void make_speklut()
{
  // ZZ > Build a lookup table for sin/cos

  int cnt;

  for ( cnt = 0; cnt < MAXLIGHTROTATION; cnt++ )
  {
    sinlut[cnt] = sin( TWO_PI * cnt / MAXLIGHTROTATION );
    coslut[cnt] = cos( TWO_PI * cnt / MAXLIGHTROTATION );
  }
};

//---------------------------------------------------------------------------------------------
void make_spektable( vect3 lite )
{
  // ZZ> This function makes a light table to fake directional lighting
  int cnt, tnc;
  float flight;
  vect3 loc_lite = lite;

  flight = loc_lite.x * loc_lite.x + loc_lite.y * loc_lite.y + loc_lite.z * loc_lite.z;
  if ( flight > 0 )
  {
    flight = sqrt( flight );
    loc_lite.x /= flight;
    loc_lite.y /= flight;
    loc_lite.z /= flight;
    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
      for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
      {
        spek_global[tnc][cnt] = spek_global_lighting( tnc, cnt, loc_lite );
        spek_local[tnc][cnt]  = spek_local_lighting( tnc, cnt );
      }
    }
  }
  else
  {
    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
      for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
      {
        spek_global[tnc][cnt] = 0;
        spek_local[tnc][cnt]  = spek_local_lighting( tnc, cnt );
      }
    }
  }

  // Fill in index number 162 for the spike mace
  for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
  {
    spek_global[tnc][MD2LIGHTINDICES-1] = 0;
    spek_local[tnc][cnt]                = 0;
  }
}

//---------------------------------------------------------------------------------------------
void make_lighttospek( void )
{
  // ZZ> This function makes a light table to fake directional lighting
  int cnt, tnc;
  //  Uint8 spek;
  //  float fTmp, fPow;


  // New routine
  for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
  {
    for ( tnc = 0; tnc < 256; tnc++ )
    {
      lighttospek[cnt][tnc] = FLOAT_TO_FP8( pow( FP8_TO_FLOAT( tnc ), 1.0 + cnt / 2.0f ) );
      lighttospek[cnt][tnc] = MIN( 255, lighttospek[cnt][tnc] );
    }
  }
}

//--------------------------------------------------------------------------------------------
void draw_Chr_BBox(CHR_REF ichr)
{
  BData * bd;
  CVolume * cv;
  vect3 pos;
  float p1_x, p1_y;
  float p2_x, p2_y;

  if( !VALID_CHR(ichr) ) return;

  bd = &chrbmpdata[ichr];
  cv = &(bd->cv);
  pos = chrpos[ichr];

  ATTRIB_PUSH( "draw_Chr_BBox", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
  {
    // don't write into the depth buffer
    glDepthMask( GL_FALSE );
    glEnable(GL_DEPTH_TEST);

    // fix the poorly chosen normals...
    glDisable( GL_CULL_FACE );

    // make them transparent
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // choose a "white" texture
    glBindTexture(GL_TEXTURE_2D, -1);

    if(cv->level > 1)
    {
      // DIAGONAL BBOX

      glColor4f(0.5,1,1,.1);

      p1_x = 0.5f * (cv->xy_max - cv->yx_max) + pos.x;
      p1_y = 0.5f * (cv->xy_max + cv->yx_max) + pos.y;
      p2_x = 0.5f * (cv->xy_max - cv->yx_min) + pos.x;
      p2_y = 0.5f * (cv->xy_max + cv->yx_min) + pos.y;

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_max + pos.z);
        glVertex3f(p1_x, p1_y, cv->z_max + pos.z);
      glEnd();

      p1_x = 0.5f * (cv->xy_max - cv->yx_min) + pos.x;
      p1_y = 0.5f * (cv->xy_max + cv->yx_min) + pos.y;
      p2_x = 0.5f * (cv->xy_min - cv->yx_min) + pos.x;
      p2_y = 0.5f * (cv->xy_min + cv->yx_min) + pos.y;

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_max + pos.z);
        glVertex3f(p1_x, p1_y, cv->z_max + pos.z);
      glEnd();

      p1_x = 0.5f * (cv->xy_min - cv->yx_min) + pos.x;
      p1_y = 0.5f * (cv->xy_min + cv->yx_min) + pos.y;
      p2_x = 0.5f * (cv->xy_min - cv->yx_max) + pos.x;
      p2_y = 0.5f * (cv->xy_min + cv->yx_max) + pos.y;

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_max + pos.z);
        glVertex3f(p1_x, p1_y, cv->z_max + pos.z);
      glEnd();

      p1_x = 0.5f * (cv->xy_min - cv->yx_max) + pos.x;
      p1_y = 0.5f * (cv->xy_min + cv->yx_max) + pos.y;
      p2_x = 0.5f * (cv->xy_max - cv->yx_max) + pos.x;
      p2_y = 0.5f * (cv->xy_max + cv->yx_max) + pos.y;

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_min + pos.z);
        glVertex3f(p2_x, p2_y, cv->z_max + pos.z);
        glVertex3f(p1_x, p1_y, cv->z_max + pos.z);
      glEnd();
    }

  //------------------------------------------------

    // SQUARE BBOX

    glColor4f(1,0.5,1,.1);

    // XZ FACE, min Y
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
    glEnd();

    // YZ FACE, min X
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
    glEnd();

  // XZ FACE, max Y
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
    glEnd();

    // YZ FACE, max X
    glBegin(GL_QUADS);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
    glEnd();

    // XY FACE, min Z
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_min + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_min + pos.z);
    glEnd();

    // XY FACE, max Z
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_min + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_max + pos.y, cv->z_max + pos.z);
      glVertex3f(cv->x_max + pos.x, cv->y_min + pos.y, cv->z_max + pos.z);
    glEnd();
  }
  ATTRIB_POP( "draw_Chr_BBox" );
};



//--------------------------------------------------------------------------------------------
void draw_CVolume( CVolume * cv )
{
  if( NULL == cv ) return;

  ATTRIB_PUSH( "draw_CVolume", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
  {
    // don't write into the depth buffer
    glDepthMask( GL_FALSE );
    glEnable(GL_DEPTH_TEST);

    // fix the poorly chosen normals...
    glDisable( GL_CULL_FACE );

    // make them transparent
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // choose a "white" texture
    glBindTexture(GL_TEXTURE_2D, -1);

    ////if(cv->level > 1)
    ////{
    //  // DIAGONAL BBOX

    //  float p1_x, p1_y;
    //  float p2_x, p2_y;

    //  glColor4f(0.5,1,1,.1);

    //  p1_x = 0.5f * (cv->xy_max - cv->yx_max);
    //  p1_y = 0.5f * (cv->xy_max + cv->yx_max);
    //  p2_x = 0.5f * (cv->xy_max - cv->yx_min);
    //  p2_y = 0.5f * (cv->xy_max + cv->yx_min);

    //  glBegin(GL_QUADS);
    //    glVertex3f(p1_x, p1_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_max);
    //    glVertex3f(p1_x, p1_y, cv->z_max);
    //  glEnd();

    //  p1_x = 0.5f * (cv->xy_max - cv->yx_min);
    //  p1_y = 0.5f * (cv->xy_max + cv->yx_min);
    //  p2_x = 0.5f * (cv->xy_min - cv->yx_min);
    //  p2_y = 0.5f * (cv->xy_min + cv->yx_min);

    //  glBegin(GL_QUADS);
    //    glVertex3f(p1_x, p1_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_max);
    //    glVertex3f(p1_x, p1_y, cv->z_max);
    //  glEnd();

    //  p1_x = 0.5f * (cv->xy_min - cv->yx_min);
    //  p1_y = 0.5f * (cv->xy_min + cv->yx_min);
    //  p2_x = 0.5f * (cv->xy_min - cv->yx_max);
    //  p2_y = 0.5f * (cv->xy_min + cv->yx_max);

    //  glBegin(GL_QUADS);
    //    glVertex3f(p1_x, p1_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_max);
    //    glVertex3f(p1_x, p1_y, cv->z_max);
    //  glEnd();

    //  p1_x = 0.5f * (cv->xy_min - cv->yx_max);
    //  p1_y = 0.5f * (cv->xy_min + cv->yx_max);
    //  p2_x = 0.5f * (cv->xy_max - cv->yx_max);
    //  p2_y = 0.5f * (cv->xy_max + cv->yx_max);

    //  glBegin(GL_QUADS);
    //    glVertex3f(p1_x, p1_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_min);
    //    glVertex3f(p2_x, p2_y, cv->z_max);
    //    glVertex3f(p1_x, p1_y, cv->z_max);
    //  glEnd();
    ////}

  //------------------------------------------------

    // SQUARE BBOX

    glColor4f(1,0.5,1,.1);

    // XZ FACE, min Y
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min, cv->y_min, cv->z_min);
      glVertex3f(cv->x_min, cv->y_min, cv->z_max);
      glVertex3f(cv->x_max, cv->y_min, cv->z_max);
      glVertex3f(cv->x_max, cv->y_min, cv->z_min);
    glEnd();

    // YZ FACE, min X
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min, cv->y_min, cv->z_min);
      glVertex3f(cv->x_min, cv->y_min, cv->z_max);
      glVertex3f(cv->x_min, cv->y_max, cv->z_max);
      glVertex3f(cv->x_min, cv->y_max, cv->z_min);
    glEnd();

  // XZ FACE, max Y
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min, cv->y_max, cv->z_min);
      glVertex3f(cv->x_min, cv->y_max, cv->z_max);
      glVertex3f(cv->x_max, cv->y_max, cv->z_max);
      glVertex3f(cv->x_max, cv->y_max, cv->z_min);
    glEnd();

    // YZ FACE, max X
    glBegin(GL_QUADS);
      glVertex3f(cv->x_max, cv->y_min, cv->z_min);
      glVertex3f(cv->x_max, cv->y_min, cv->z_max);
      glVertex3f(cv->x_max, cv->y_max, cv->z_max);
      glVertex3f(cv->x_max, cv->y_max, cv->z_min);
    glEnd();

    // XY FACE, min Z
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min, cv->y_min, cv->z_min);
      glVertex3f(cv->x_min, cv->y_max, cv->z_min);
      glVertex3f(cv->x_max, cv->y_max, cv->z_min);
      glVertex3f(cv->x_max, cv->y_min, cv->z_min);
    glEnd();

    // XY FACE, max Z
    glBegin(GL_QUADS);
      glVertex3f(cv->x_min, cv->y_min, cv->z_max);
      glVertex3f(cv->x_min, cv->y_max, cv->z_max);
      glVertex3f(cv->x_max, cv->y_max, cv->z_max);
      glVertex3f(cv->x_max, cv->y_min, cv->z_max);
    glEnd();
  }
  ATTRIB_POP( "draw_CVolume" );
};

