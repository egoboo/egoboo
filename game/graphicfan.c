/* Egoboo - graphicfan.c
* World mesh drawing.
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

#include "Log.h"
#include "egoboo.h"
#include "mesh.h"
#include "Frustum.h"
#include <assert.h>

int     numrenderlist;                                  // Number to render, total
int     numrenderlist_shine;                            // ..., reflective
int     numrenderlist_reflc;                            // ..., has reflection
int     numrenderlist_norm;                             // ..., no reflect, no reflection
int     numrenderlist_watr;                             // ..., water
Uint32  renderlist[MAXMESHRENDER];                      // List of which to render, total
Uint32  renderlist_shine[MAXMESHRENDER];                // ..., reflective
Uint32  renderlist_reflc[MAXMESHRENDER];                // ..., has reflection
Uint32  renderlist_norm[MAXMESHRENDER];                 // ..., no reflect, no reflection
Uint32  renderlist_watr[MAXMESHRENDER];                 // ..., water

//--------------------------------------------------------------------------------------------
void render_fan_ref( Uint32 fan, char tex_loaded, float level )
{
  // ZZ> This function draws a mesh fan

  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  float offu, offv;
  int light_flat_r, light_flat_g, light_flat_b;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = meshtile[fan];               // Tile
  Uint8  fx   = meshfx[fan];                 // Fx bits
  Uint16 type = meshtype[fan];               // Command type ( index to points in fan )

  if ( tile == INVALID_TILE )
    return;

  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & animtilebigbaseand;// Animation set
      tile += animtileframeadd << 1;         // Animated tile
      tile = ( tile & animtilebigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & animtilebaseand;// Animation set
      tile += animtileframeadd;         // Animated tile
      tile = ( tile & animtileframeand ) + basetile;
    }
  }

  offu = meshtileoffu[tile];          // Texture offsets
  offv = meshtileoffv[tile];          //

  texture = ( tile >> 6 ) + 1;              // 64 tiles in each 256x256 texture
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands

  // Original points
  badvertex = meshvrtstart[fan];          // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat_r = 0;
  light_flat_g = 0;
  light_flat_b = 0;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    float ftmp;
    light_flat_r += meshvrtlr_fp8[badvertex] * 0.5f;
    light_flat_g += meshvrtlg_fp8[badvertex] * 0.5f;
    light_flat_b += meshvrtlb_fp8[badvertex] * 0.5f;

    ftmp = meshvrtz[badvertex] - level;
    v[cnt].pos.x = meshvrtx[badvertex];
    v[cnt].pos.y = meshvrty[badvertex];
    v[cnt].pos.z = level - ftmp;
    v[cnt].col.r = FP8_TO_FLOAT( meshvrtlr_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( meshvrtlg_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( meshvrtlb_fp8[badvertex] );
    v[cnt].s = meshcommandu[type][badvertex] + offu;
    v[cnt].t = meshcommandv[type][badvertex] + offv;

    badvertex++;
  }
  light_flat_r /= vertices;
  light_flat_g /= vertices;
  light_flat_b /= vertices;


  // Change texture if need be
  if ( meshlasttexture != texture )
  {
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    meshlasttexture = texture;
  }

  ATTRIB_PUSH( "render_fan", GL_CURRENT_BIT );
  {
    // Render each command
    if ( CData.shading == GL_FLAT )
    {
      // use the average lighting
      glColor4f( FP8_TO_FLOAT( light_flat_r ), FP8_TO_FLOAT( light_flat_g ), FP8_TO_FLOAT( light_flat_b ), 1 );
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
        {
          vertex = meshcommandvrt[type][entry];
          glTexCoord2f( meshcommandu[type][vertex] + offu, meshcommandv[type][vertex] + offv );
          glVertex3fv( v[vertex].pos.v );
          entry++;
        }
        glEnd();
      }
    }
    else
    {
      // use per-vertex lighting
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
        {
          vertex = meshcommandvrt[type][entry];
          glColor3fv( v[vertex].col.v );
          glTexCoord2f( meshcommandu[type][vertex] + offu, meshcommandv[type][vertex] + offv );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }
  }
  ATTRIB_POP( "render_fan" );
}


//--------------------------------------------------------------------------------------------
void render_fan( Uint32 fan, char tex_loaded )
{
  // ZZ> This function draws a mesh fan

  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  float offu, offv;
  GLVector light_flat;
  vect3 nrm, pos;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = meshtile[fan];               // Tile
  Uint8  fx   = meshfx[fan];                 // Fx bits
  Uint16 type = meshtype[fan];               // Command type ( index to points in fan )

  if ( tile == INVALID_TILE )
    return;

  mesh_calc_normal_fan( fan, &nrm, &pos );


  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & animtilebigbaseand;// Animation set
      tile += animtileframeadd << 1;         // Animated tile
      tile = ( tile & animtilebigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & animtilebaseand;// Animation set
      tile += animtileframeadd;         // Animated tile
      tile = ( tile & animtileframeand ) + basetile;
    }
  }

  offu = meshtileoffu[tile];          // Texture offsets
  offv = meshtileoffv[tile];          //

  texture = ( tile >> 6 ) + 1;              // 64 tiles in each 256x256 texture
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands

  // Original points
  badvertex = meshvrtstart[fan];          // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat.r =
    light_flat.g =
      light_flat.b =
        light_flat.a = 0.0f;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {

    v[cnt].pos.x = meshvrtx[badvertex];
    v[cnt].pos.y = meshvrty[badvertex];
    v[cnt].pos.z = meshvrtz[badvertex];

    v[cnt].col.r = FP8_TO_FLOAT( meshvrtlr_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( meshvrtlg_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( meshvrtlb_fp8[badvertex] );
    v[cnt].col.a = 1.0f;

#if defined(DEBUG_MESHFX) && defined(_DEBUG)

    if(CData.DevMode)
    {

      if ( HAS_SOME_BITS( fx, MESHFX_WALL ) )
      {
        v[cnt].col.r /= 5.0f;
        v[cnt].col.r += 0.8f;
      }

      if ( HAS_SOME_BITS( fx, MESHFX_IMPASS ) )
      {
        v[cnt].col.g /= 5.0f;
        v[cnt].col.g += 0.8f;
      }

      if ( HAS_SOME_BITS( fx, MESHFX_SLIPPY ) )
      {
        v[cnt].col.b /= 5.0f;
        v[cnt].col.b += 0.8f;
      }
    }

#endif

    light_flat.r += v[cnt].col.r;
    light_flat.g += v[cnt].col.g;
    light_flat.b += v[cnt].col.b;
    light_flat.a += v[cnt].col.a;


    v[cnt].s = meshcommandu[type][badvertex] + offu;
    v[cnt].t = meshcommandv[type][badvertex] + offv;

#if defined(DEBUG_NORMALS) && defined(_DEBUG)
    if(CData.DevMode)
    {
      mesh_calc_normal_pos( fan, v[cnt].pos.x, v[cnt].pos.y, &v[cnt].n );
    }
#endif

    badvertex++;
  }

  if ( vertices > 1 )
  {
    light_flat.r /= vertices;
    light_flat.g /= vertices;
    light_flat.b /= vertices;
    light_flat.a /= vertices;
  };



  // Change texture if need be
  if ( meshlasttexture != texture )
  {
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    meshlasttexture = texture;
  }

  ATTRIB_PUSH( "render_fan", GL_CURRENT_BIT );
  {
    // Render each command
    if ( CData.shading == GL_FLAT )
    {
      // use the average lighting
      glColor4fv( light_flat.v );
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
        {
          vertex = meshcommandvrt[type][entry];
          glTexCoord2f( meshcommandu[type][vertex] + offu, meshcommandv[type][vertex] + offv );
          glVertex3fv( v[vertex].pos.v );
          entry++;
        }
        glEnd();
      }
    }
    else
    {
      // use per-vertex lighting
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
        {
          vertex = meshcommandvrt[type][entry];
          glColor4fv( v[vertex].col.v );
          glTexCoord2f( meshcommandu[type][vertex] + offu, meshcommandv[type][vertex] + offv );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }


#if defined(DEBUG_NORMALS) && defined(_DEBUG)
    if ( CData.DevMode )
    {

      glBegin( GL_LINES );
      {
        glLineWidth( 1.5f );
        glColor4f( 1, 1, 1, 1 );
        for ( cnt = 0; cnt < 4; cnt++ )
        {
          glVertex3fv( v[cnt].pos.v );
          glVertex3f( v[cnt].pos.x + v[cnt].nrm.x*64, v[cnt].pos.y + v[cnt].nrm.y*64, v[cnt].pos.z + v[cnt].nrm.z*64 );
        }
      }
      glEnd();

      glBegin( GL_LINES );
      glLineWidth( 3.0f );
      glColor4f( 1, 1, 1, 1 );
      glVertex3fv( pos.v );
      glVertex3f( pos.x + nrm.x, pos.y + nrm.y, pos.z + nrm.z );
      glEnd();
    }
#endif
  }
  ATTRIB_POP( "render_fan" );


}

// float z;
// Uint8 red, grn, blu;
//TODO: Implement OpenGL fog effects
/*  if(fogon)
{
// The full fog value
fogspec = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);
for (cnt = 0; cnt < vertices; cnt++)
{
v[cnt].pos.x = (float) meshvrtx[badvertex];
v[cnt].pos.y = (float) meshvrty[badvertex];
v[cnt].pos.z = (float) meshvrtz[badvertex];
z = v[cnt].pos.z;


// Figure out the fog coloring
if(z < fogtop)
{
if(z < fogbottom)
{
v[cnt].dcSpecular = fogspec;  // Full fog
}
else
{
z = 1.0 - ((z - fogbottom)/fogdistance);  // 0.0 to 1.0... Amount of fog to keep
red = (fogred * z);
grn = (foggrn * z);
blu = (fogblu * z);
ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
v[cnt].dcSpecular = ambi;
}
}
else
{
v[cnt].dcSpecular = 0;  // No fog
}

ambi = (DWORD) meshvrtl_fp8[badvertex];
ambi = (ambi<<8)|ambi;
ambi = (ambi<<8)|ambi;
//                v[cnt].dcColor = ambi;
//                v[cnt].dwReserved = 0;
badvertex++;
}
}
*/

//--------------------------------------------------------------------------------------------
void render_water_fan( Uint32 fan, Uint8 layer, Uint8 mode )
{
  // ZZ> This function draws a water fan
  GLVertex v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  float offu, offv;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type = 0;                           // Command type ( index to points in fan )
  offu = waterlayeru[layer];          // Texture offsets
  offv = waterlayerv[layer];          //
  frame = waterlayerframe[layer];     // Frame

  texture = layer + 5;                    // Water starts at texture 5
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands


  // figure the ambient light
  badvertex = meshvrtstart[fan];          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = meshvrtx[badvertex];
    v[cnt].pos.y = meshvrty[badvertex];
    v[cnt].pos.z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

    if ( !waterlight )
    {
      v[cnt].col.r = FP8_TO_FLOAT( meshvrtlr_fp8[badvertex] );
      v[cnt].col.g = FP8_TO_FLOAT( meshvrtlg_fp8[badvertex] );
      v[cnt].col.b = FP8_TO_FLOAT( meshvrtlb_fp8[badvertex] );
      v[cnt].col.a = FP8_TO_FLOAT( waterlayeralpha_fp8[layer] );
    }
    else
    {
      v[cnt].col.r = FP8_TO_FLOAT( FP8_MUL( meshvrtlr_fp8[badvertex], waterlayeralpha_fp8[layer] ) );
      v[cnt].col.g = FP8_TO_FLOAT( FP8_MUL( meshvrtlg_fp8[badvertex], waterlayeralpha_fp8[layer] ) );
      v[cnt].col.b = FP8_TO_FLOAT( FP8_MUL( meshvrtlb_fp8[badvertex], waterlayeralpha_fp8[layer] ) );
      v[cnt].col.a = 1.0f;
    }


    // !!!BAD!!!  Debug code for show what mode means...
    //red = 50;
    //grn = 50;
    //blu = 50;
    //switch(mode)
    //{
    //    case 0:
    //      red = 255;
    //      break;

    //    case 1:
    //      grn = 255;
    //      break;

    //    case 2:
    //      blu = 255;
    //      break;

    //    case 3:
    //      red = 255;
    //      grn = 255;
    //      blu = 255;
    //      break;

    //}
    //ambi = 0xbf000000 | (red<<16) | (grn<<8) | (blu);
    // !!!BAD!!!

    badvertex++;
  };

  // Render each command
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;

  ATTRIB_PUSH( "render_water_fan", GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );

    entry = 0;
    for ( cnt = 0; cnt < commands; cnt++ )
    {
      glBegin( GL_TRIANGLE_FAN );
      for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
      {
        vertex = meshcommandvrt[type][entry];
        glColor4fv( v[vertex].col.v );
        glTexCoord2fv( &v[vertex].s );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP( "render_water_fan" );
}


//--------------------------------------------------------------------------------------------
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode )
{
  // ZZ> This function draws a water fan
  GLVertex v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  // Uint8 red, grn, blu;
  float offu, offv;
  // float z;
  //Uint32 ambi, spek;
  // DWORD fogspec;



  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type = 0;                           // Command type ( index to points in fan )
  offu = waterlayeru[layer];          // Texture offsets
  offv = waterlayerv[layer];          //
  frame = waterlayerframe[layer];     // Frame

  texture = layer + 5;                    // Water starts at texture 5
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands


  badvertex = meshvrtstart[fan];          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = meshvrtx[badvertex];
    v[cnt].pos.y = meshvrty[badvertex];
    v[cnt].pos.z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

    v[cnt].col.r = v[cnt].col.g = v[cnt].col.b = 1.0f;
    v[cnt].col.a = FP8_TO_FLOAT( waterlayeralpha_fp8[layer] );

    badvertex++;
  };

  // Render each command
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;

  ATTRIB_PUSH( "render_water_fan_lit", GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {
    // Change texture if need be
    if ( meshlasttexture != texture )
    {
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      meshlasttexture = texture;
    }

    entry = 0;
    for ( cnt = 0; cnt < commands; cnt++ )
    {
      glBegin( GL_TRIANGLE_FAN );
      for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
      {
        vertex = meshcommandvrt[type][entry];
        glColor4fv( v[vertex].col.v );
        glTexCoord2fv( &v[vertex].s );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP( "render_water_fan_lit" );
}

//--------------------------------------------------------------------------------------------
//void make_renderlist()
//{
//  // ZZ> This function figures out which mesh fans to draw
//  int cnt, fan, fanx, fany;
//  int row, run, numrow;
//  int xlist[4], ylist[4];
//  int leftnum, leftlist[4];
//  int rightnum, rightlist[4];
//  int fanrowstart[128], fanrowrun[128];
//  int x, stepx, divx, basex;
//  int from, to;
//
//
//  // Clear old render lists
//  for (cnt = 0; cnt < numrenderlist; cnt++)
//  {
//    fan = renderlist[cnt];
//    mesh_remove_renderlist(fan);
//  }
//  numrenderlist = 0;
//  numrenderlist_shine = 0;
//  numrenderlist_reflc = 0;
//  numrenderlist_norm = 0;
//  numrenderlist_watr = 0;
//
//  // It works better this way...
//  cornery[cornerlistlowtohighy[3]] += 256;
//
//  // Make life simpler
//  xlist[0] = cornerx[cornerlistlowtohighy[0]];
//  xlist[1] = cornerx[cornerlistlowtohighy[1]];
//  xlist[2] = cornerx[cornerlistlowtohighy[2]];
//  xlist[3] = cornerx[cornerlistlowtohighy[3]];
//  ylist[0] = cornery[cornerlistlowtohighy[0]];
//  ylist[1] = cornery[cornerlistlowtohighy[1]];
//  ylist[2] = cornery[cornerlistlowtohighy[2]];
//  ylist[3] = cornery[cornerlistlowtohighy[3]];
//
//  // Find the center line
//  divx = ylist[3] - ylist[0]; if (divx < 1) return;
//  stepx = xlist[3] - xlist[0];
//  basex = xlist[0];
//
//
//  // Find the points in each edge
//  leftlist[0] = 0;  leftnum = 1;
//  rightlist[0] = 0;  rightnum = 1;
//  if (xlist[1] < (stepx*(ylist[1] - ylist[0]) / divx) + basex)
//  {
//    leftlist[leftnum] = 1;  leftnum++;
//    cornerx[1] -= 512;
//  }
//  else
//  {
//    rightlist[rightnum] = 1;  rightnum++;
//    cornerx[1] += 512;
//  }
//  if (xlist[2] < (stepx*(ylist[2] - ylist[0]) / divx) + basex)
//  {
//    leftlist[leftnum] = 2;  leftnum++;
//    cornerx[2] -= 512;
//  }
//  else
//  {
//    rightlist[rightnum] = 2;  rightnum++;
//    cornerx[2] += 512;
//  }
//  leftlist[leftnum] = 3;  leftnum++;
//  rightlist[rightnum] = 3;  rightnum++;
//
//
//  // Make the left edge ( rowstart )
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  row = 0;
//  cnt = 1;
//  while (cnt < leftnum)
//  {
//    from = leftlist[cnt-1];  to = leftlist[cnt];
//    x = xlist[from];
//    divx = ylist[to] - ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = MESH_FAN_TO_INT(xlist[to] - xlist[from]) / divx;
//    }
//    x -= 256;
//
//    run = MESH_INT_TO_FAN(ylist[to]);
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < meshsizey)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= meshsizex)  fanx = meshsizex - 1;
//        fanrowstart[row] = fanx;
//        row++;
//      }
//      x += stepx;
//      fany++;
//    }
//    cnt++;
//  }
//  numrow = row;
//
//
//  // Make the right edge ( rowrun )
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  row = 0;
//  cnt = 1;
//  while (cnt < rightnum)
//  {
//    from = rightlist[cnt-1];  to = rightlist[cnt];
//    x = xlist[from];
//    //x+=128;
//    divx = ylist[to] - ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = MESH_FAN_TO_INT(xlist[to] - xlist[from]) / divx;
//    }
//
//    run = MESH_INT_TO_FAN(ylist[to]);
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < meshsizey)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= meshsizex - 1)  fanx = meshsizex - 1;//-2
//        fanrowrun[row] = ABS(fanx - fanrowstart[row]) + 1;
//        row++;
//      }
//      x += stepx;
//      fany++;
//    }
//    cnt++;
//  }
//
//  if (numrow != row)
//  {
//    log_error("ROW error (%i, %i)\n", numrow, row);
//  }
//
//  // Fill 'em up again
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  if (fany < 0) fany = 0;
//  if (fany >= meshsizey) fany = meshsizey - 1;
//  row = 0;
//  while (row < numrow)
//  {
//    cnt = mesh_convert_fan(fanrowstart[row], fany);
//    run = fanrowrun[row];
//    fanx = 0;
//    while (fanx < run)
//    {
//      if (numrenderlist<MAXMESHRENDER)
//      {
//        bool_t is_shine, is_noref, is_norml, is_water;
//
//        is_shine = mesh_has_some_bits(cnt, MESHFX_SHINY);
//        is_noref = mesh_has_no_bits(cnt, MESHFX_NOREFLECT);
//        is_norml = !is_shine;
//        is_water = mesh_has_some_bits(cnt, MESHFX_WATER);
//
//        // Put each tile in basic list
//        renderlist[numrenderlist] = cnt;
//        numrenderlist++;
//        mesh_add_renderlist(cnt);
//
//        // Put each tile in one other list, for shadows and relections
//        if (!is_noref)
//        {
//          renderlist_reflc[numrenderlist_reflc] = cnt;
//          numrenderlist_reflc++;
//        }
//
//        if (is_shine)
//        {
//          renderlist_shine[numrenderlist_shine] = cnt;
//          numrenderlist_shine++;
//        }
//
//        if (is_norml)
//        {
//          renderlist_norm[numrenderlist_norm] = cnt;
//          numrenderlist_norm++;
//        }
//
//        if (is_water)
//        {
//          renderlist_watr[numrenderlist_watr] = cnt;
//          numrenderlist_watr++;
//        }
//      };
//
//      cnt++;
//      fanx++;
//    }
//    row++;
//    fany++;
//  }
//}

void make_renderlist()
{
  int fan, fan_count;
  bool_t inview;
  static Uint32 next_wldframe = 0;

  // make a delay
  if(wldframe < next_wldframe) return;
  next_wldframe = wldframe + 7;

  numrenderlist = 0;
  numrenderlist_shine = 0;
  numrenderlist_reflc = 0;
  numrenderlist_norm = 0;
  numrenderlist_watr = 0;

  fan_count = meshsizex * meshsizey;
  for ( fan = 0; fan < fan_count; fan++ )
  {
    inview = Frustum_BBoxInFrustum( &gFrustum, meshvrtmins[fan].v, meshvrtmaxs[fan].v );

    meshinrenderlist[fan] = bfalse;

    if ( inview && numrenderlist < MAXMESHRENDER )
    {
      bool_t is_shine, is_noref, is_norml, is_water;

      meshinrenderlist[fan] = btrue;

      is_shine = mesh_has_all_bits( fan, MESHFX_SHINY );
      is_noref = mesh_has_all_bits( fan, MESHFX_NOREFLECT );
      is_norml = !is_shine;
      is_water = mesh_has_some_bits( fan, MESHFX_WATER );

      // Put each tile in basic list
      renderlist[numrenderlist] = fan;
      numrenderlist++;
      mesh_add_renderlist( fan );

      // Put each tile
      if ( !is_noref )
      {
        renderlist_reflc[numrenderlist_reflc] = fan;
        numrenderlist_reflc++;
      }

      if ( is_shine )
      {
        renderlist_shine[numrenderlist_shine] = fan;
        numrenderlist_shine++;
      }

      if ( is_norml )
      {
        renderlist_norm[numrenderlist_norm] = fan;
        numrenderlist_norm++;
      }

      if ( is_water )
      {
        renderlist_watr[numrenderlist_watr] = fan;
        numrenderlist_watr++;
      }
    };
  };

}

//--------------------------------------------------------------------------------------------
void set_fan_light( int fanx, int fany, PRT_REF particle )
{
  // ZZ> This function is a little helper, lighting the selected fan
  //     with the chosen particle
  vect3 dif, nrm;
  int fan, vertex, lastvertex;
  float flight, dist2;
  float light_r, light_g, light_b;
  float light_r0, light_g0, light_b0;


  if ( fanx >= 0 && fanx < meshsizex && fany >= 0 && fany < meshsizey )
  {
    fan = mesh_convert_fan( fanx, fany );
    vertex = meshvrtstart[fan];
    lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
    while ( vertex < lastvertex )
    {
      light_r0 = light_r = meshvrtar_fp8[vertex];
      light_g0 = light_g = meshvrtag_fp8[vertex];
      light_b0 = light_b = meshvrtab_fp8[vertex];

      dif.x = prtpos[particle].x - meshvrtx[vertex];
      dif.y = prtpos[particle].y - meshvrty[vertex];
      dif.z = prtpos[particle].z - meshvrtz[vertex];

      dist2 = DotProduct( dif, dif );

      flight = prtdynalightlevel[particle];
      flight *= 127 * prtdynalightfalloff[particle] / ( 127 * prtdynalightfalloff[particle] + dist2 );

      if ( dist2 > 0.0f )
      {
        float ftmp, dist = sqrt( dist2 );
        vect3 pos = {meshvrtx[vertex], meshvrty[vertex], meshvrtz[vertex]};

        dif.x /= dist;
        dif.y /= dist;
        dif.z /= dist;

        mesh_calc_normal( pos, &nrm );

        ftmp = DotProduct( dif, nrm );
        if ( ftmp > 0 )
        {
          light_r += 255 * flight * ftmp * ftmp;
          light_g += 255 * flight * ftmp * ftmp;
          light_b += 255 * flight * ftmp * ftmp;
        };
      }
      else
      {
        light_r += 255 * flight;
        light_g += 255 * flight;
        light_b += 255 * flight;
      }

      meshvrtlr_fp8[vertex] = 0.9 * light_r0 + 0.1 * light_r;
      meshvrtlg_fp8[vertex] = 0.9 * light_g0 + 0.1 * light_g;
      meshvrtlb_fp8[vertex] = 0.9 * light_b0 + 0.1 * light_b;

      if ( meshexploremode )
      {
        if ( meshvrtlr_fp8[vertex] > light_r0 ) meshvrtar_fp8[vertex] = 0.9 * meshvrtar_fp8[vertex] + 0.1 * meshvrtlr_fp8[vertex];
        if ( meshvrtlg_fp8[vertex] > light_g0 ) meshvrtag_fp8[vertex] = 0.9 * meshvrtag_fp8[vertex] + 0.1 * meshvrtlg_fp8[vertex];
        if ( meshvrtlb_fp8[vertex] > light_b0 ) meshvrtab_fp8[vertex] = 0.9 * meshvrtab_fp8[vertex] + 0.1 * meshvrtlb_fp8[vertex];
      };

      vertex++;
    }
  }
}


//--------------------------------------------------------------------------------------------
void do_dynalight()
{
  // ZZ> This function does dynamic lighting of visible fans

  int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
  float light_r, light_g, light_b;
  float dist2;
  vect3 dif, nrm;


  // Do each floor tile
  if ( meshexploremode )
  {
    // Set base light level in explore mode...  Don't need to do every frame
    if (( allframe & 7 ) == 0 )
    {

      for ( cnt = 0; cnt < MAXPRT; cnt++ )
      {
        if ( !VALID_PRT( cnt ) || !prtdynalighton[cnt] ) continue;

        fanx = MESH_FLOAT_TO_FAN( prtpos[cnt].x );
        fany = MESH_FLOAT_TO_FAN( prtpos[cnt].y );

        for ( addy = -DYNAFANS; addy <= DYNAFANS; addy++ )
        {
          for ( addx = -DYNAFANS; addx <= DYNAFANS; addx++ )
          {
            set_fan_light( fanx + addx, fany + addy, cnt );
          }
        }
      }
    }
  }
  else
  {
    float ftmp;
    if ( CData.shading != GL_FLAT )
    {
      // Add to base light level in normal mode
      entry = 0;
      while ( entry < numrenderlist )
      {
        fan = renderlist[entry];
        vertex = meshvrtstart[fan];
        lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
        while ( vertex < lastvertex )
        {
          vect3 pos = {meshvrtx[vertex], meshvrty[vertex], meshvrtz[vertex]};

          // Do light particles
          light_r = meshvrtar_fp8[vertex];
          light_g = meshvrtag_fp8[vertex];
          light_b = meshvrtab_fp8[vertex];

          mesh_calc_normal( pos, &nrm );
          light_r += lightambicol.r * 255;
          light_g += lightambicol.g * 255;
          light_b += lightambicol.b * 255;


          ftmp = DotProduct( nrm, lightspekdir );
          if ( ftmp > 0 )
          {
            light_r += lightspekcol.r * 255 * ftmp * ftmp;
            light_g += lightspekcol.g * 255 * ftmp * ftmp;
            light_b += lightspekcol.b * 255 * ftmp * ftmp;
          };

          cnt = 0;
          while ( cnt < numdynalight )
          {
            float flight;

            dif.x = dynalightlist[cnt].x - meshvrtx[vertex];
            dif.y = dynalightlist[cnt].y - meshvrty[vertex];
            dif.z = dynalightlist[cnt].z - meshvrtz[vertex];

            dist2 = DotProduct( dif, dif );

            flight = dynalightlevel[cnt];
            flight *= 127 * dynalightfalloff[cnt] / ( 127 * dynalightfalloff[cnt] + dist2 );

            if ( dist2 > 0.0f )
            {
              float dist = sqrt( dist2 );

              dif.x /= dist;
              dif.y /= dist;
              dif.z /= dist;

              ftmp = DotProduct( dif, nrm );
              if ( ftmp > 0 )
              {
                light_r += 255 * flight * ftmp * ftmp;
                light_g += 255 * flight * ftmp * ftmp;
                light_b += 255 * flight * ftmp * ftmp;
              };
            }
            else
            {
              light_r += 255 * flight;
              light_g += 255 * flight;
              light_b += 255 * flight;
            }

            cnt++;
          }

          if ( light_r > 255 ) light_r = 255;
          if ( light_r <   0 ) light_r = 0;
          meshvrtlr_fp8[vertex] = 0.9 * meshvrtlr_fp8[vertex] + 0.1 * light_r;

          if ( light_g > 255 ) light_g = 255;
          if ( light_g <   0 ) light_g = 0;
          meshvrtlg_fp8[vertex] = 0.9 * meshvrtlg_fp8[vertex] + 0.1 * light_g;

          if ( light_b > 255 ) light_b = 255;
          if ( light_b <   0 ) light_b = 0;
          meshvrtlb_fp8[vertex] = 0.9 * meshvrtlb_fp8[vertex] + 0.1 * light_b;

          if ( meshexploremode )
          {
            if ( light_r > meshvrtar_fp8[vertex] ) meshvrtar_fp8[vertex] = 0.9 * meshvrtar_fp8[vertex] + 0.1 * light_r;
            if ( light_g > meshvrtag_fp8[vertex] ) meshvrtag_fp8[vertex] = 0.9 * meshvrtag_fp8[vertex] + 0.1 * light_g;
            if ( light_b > meshvrtab_fp8[vertex] ) meshvrtab_fp8[vertex] = 0.9 * meshvrtab_fp8[vertex] + 0.1 * light_b;
          }

          vertex++;
        }
        entry++;
      }
    }
    else
    {
      entry = 0;
      while ( entry < numrenderlist )
      {
        fan = renderlist[entry];
        vertex = meshvrtstart[fan];
        lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
        while ( vertex < lastvertex )
        {
          // Do light particles
          meshvrtlr_fp8[vertex] = meshvrtar_fp8[vertex];
          meshvrtlg_fp8[vertex] = meshvrtag_fp8[vertex];
          meshvrtlb_fp8[vertex] = meshvrtab_fp8[vertex];

          vertex++;
        }
        entry++;
      }
    }
  }
};

