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

#include "ogl_include.h"
#include "graphic.h"
#include "Log.h"
#include "mesh.h"
#include "Frustum.h"
#include "particle.h"
#include "graphic.h"

#include "egoboo.h"

#include <assert.h>

RENDERLIST renderlist;

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

  Uint16 tile = Mesh_Fan[fan].tile;               // Tile
  Uint8  fx   = Mesh_Fan[fan].fx;                 // Fx bits
  Uint16 type = Mesh_Fan[fan].type;               // Command type ( index to points in fan )

  if ( tile == INVALID_TILE )
    return;

  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & GTile_Anim.bigbaseand;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & GTile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = ( tile & GTile_Anim.frameand ) + basetile;
    }
  }

  offu = Mesh_Tile[tile].off_u;          // Texture offsets
  offv = Mesh_Tile[tile].off_v;          //

  texture = ( tile >> 6 ) + TX_TILE_0;              // 64 tiles in each 256x256 texture
  vertices = Mesh_Cmd[type].vrt_count;// Number of vertices
  commands = Mesh_Cmd[type].count;          // Number of commands

  // Original points
  badvertex = Mesh_Fan[fan].vrt_start;          // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat_r = 0;
  light_flat_g = 0;
  light_flat_b = 0;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    float ftmp;
    light_flat_r += Mesh_Mem.vrt_lr_fp8[badvertex] * 0.5f;
    light_flat_g += Mesh_Mem.vrt_lg_fp8[badvertex] * 0.5f;
    light_flat_b += Mesh_Mem.vrt_lb_fp8[badvertex] * 0.5f;

    ftmp = Mesh_Mem.vrt_z[badvertex] - level;
    v[cnt].pos.x = Mesh_Mem.vrt_x[badvertex];
    v[cnt].pos.y = Mesh_Mem.vrt_y[badvertex];
    v[cnt].pos.z = level - ftmp;
    v[cnt].col.r = FP8_TO_FLOAT( Mesh_Mem.vrt_lr_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( Mesh_Mem.vrt_lg_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( Mesh_Mem.vrt_lb_fp8[badvertex] );
    v[cnt].s = Mesh_Cmd[type].u[badvertex] + offu;
    v[cnt].t = Mesh_Cmd[type].v[badvertex] + offv;

    badvertex++;
  }
  light_flat_r /= vertices;
  light_flat_g /= vertices;
  light_flat_b /= vertices;


  // Change texture if need be
  if ( mesh.last_texture != texture )
  {
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    mesh.last_texture = texture;
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
        for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
        {
          vertex = Mesh_Cmd[type].vrt[entry];
          glTexCoord2f( Mesh_Cmd[type].u[vertex] + offu, Mesh_Cmd[type].v[vertex] + offv );
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
        for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
        {
          vertex = Mesh_Cmd[type].vrt[entry];
          glColor3fv( v[vertex].col.v );
          glTexCoord2f( Mesh_Cmd[type].u[vertex] + offu, Mesh_Cmd[type].v[vertex] + offv );
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
  GLvector light_flat;
  vect3 nrm, pos;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = Mesh_Fan[fan].tile;               // Tile
  Uint8  fx   = Mesh_Fan[fan].fx;                 // Fx bits
  Uint16 type = Mesh_Fan[fan].type;               // Command type ( index to points in fan )

  if ( tile == INVALID_TILE )
    return;

  mesh_calc_normal_fan( fan, &nrm, &pos );


  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & GTile_Anim.bigbaseand;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & GTile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = ( tile & GTile_Anim.frameand ) + basetile;
    }
  }

  offu = Mesh_Tile[tile].off_u;          // Texture offsets
  offv = Mesh_Tile[tile].off_v;          //

  texture = ( tile >> 6 ) + TX_TILE_0;              // 64 tiles in each 256x256 texture
  vertices = Mesh_Cmd[type].vrt_count;      // Number of vertices
  commands = Mesh_Cmd[type].count;          // Number of commands

  // Original points
  badvertex = Mesh_Fan[fan].vrt_start;          // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat.r =
    light_flat.g =
      light_flat.b =
        light_flat.a = 0.0f;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {

    v[cnt].pos.x = Mesh_Mem.vrt_x[badvertex];
    v[cnt].pos.y = Mesh_Mem.vrt_y[badvertex];
    v[cnt].pos.z = Mesh_Mem.vrt_z[badvertex];

    v[cnt].col.r = FP8_TO_FLOAT( Mesh_Mem.vrt_lr_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( Mesh_Mem.vrt_lg_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( Mesh_Mem.vrt_lb_fp8[badvertex] );
    v[cnt].col.a = 1.0f;

#if DEBUG_MESHFX && defined(_DEBUG)
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


    v[cnt].s = Mesh_Cmd[type].u[cnt] + offu;
    v[cnt].t = Mesh_Cmd[type].v[cnt] + offv;

#if DEBUG_NORMALS && defined(_DEBUG)
    if(CData.DevMode)
    {
      vect3 * pv3 = (vect3 *) &(v[cnt].pos.v);
      mesh_calc_normal_pos( fan, *pv3, &(v[cnt].nrm) );
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
  if ( mesh.last_texture != texture )
  {
    GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
    mesh.last_texture = texture;
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
        for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
        {
          vertex = Mesh_Cmd[type].vrt[entry];
          glTexCoord2f( Mesh_Cmd[type].u[vertex] + offu, Mesh_Cmd[type].v[vertex] + offv );
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
        for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
        {
          vertex = Mesh_Cmd[type].vrt[entry];
          glColor4fv( v[vertex].col.v );
          glTexCoord2f( Mesh_Cmd[type].u[vertex] + offu, Mesh_Cmd[type].v[vertex] + offv );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }


#if DEBUG_NORMALS && defined(_DEBUG)
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
/*  if(GFog.on)
{
// The full fog value
GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
for (cnt = 0; cnt < vertices; cnt++)
{
v[cnt].pos.x = (float) Mesh_Mem.vrt_x[badvertex];
v[cnt].pos.y = (float) Mesh_Mem.vrt_y[badvertex];
v[cnt].pos.z = (float) Mesh_Mem.vrt_z[badvertex];
z = v[cnt].pos.z;


// Figure out the fog coloring
if(z < GFog.top)
{
if(z < GFog.bottom)
{
v[cnt].dcSpecular = GFog.spec;  // Full fog
}
else
{
z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of fog to keep
red = (GFog.red * z);
grn = (GFog.grn * z);
blu = (GFog.blu * z);
ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
v[cnt].dcSpecular = ambi;
}
}
else
{
v[cnt].dcSpecular = 0;  // No fog
}

ambi = (DWORD) Mesh_Mem.vrt_l_fp8[badvertex];
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
  offu = GWater.layer[layer].u;          // Texture offsets
  offv = GWater.layer[layer].v;          //
  frame = GWater.layer[layer].frame;     // Frame

  texture = layer + TX_WATER_TOP;                    // Water starts at texture 5
  vertices = Mesh_Cmd[type].vrt_count;// Number of vertices
  commands = Mesh_Cmd[type].count;          // Number of commands


  // figure the ambient light
  badvertex = Mesh_Fan[fan].vrt_start;          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = Mesh_Mem.vrt_x[badvertex];
    v[cnt].pos.y = Mesh_Mem.vrt_y[badvertex];
    v[cnt].pos.z = GWater.layer[layer].zadd[frame][mode][cnt] + GWater.layer[layer].z;

    if ( !GWater.light )
    {
      v[cnt].col.r = FP8_TO_FLOAT( Mesh_Mem.vrt_lr_fp8[badvertex] );
      v[cnt].col.g = FP8_TO_FLOAT( Mesh_Mem.vrt_lg_fp8[badvertex] );
      v[cnt].col.b = FP8_TO_FLOAT( Mesh_Mem.vrt_lb_fp8[badvertex] );
      v[cnt].col.a = FP8_TO_FLOAT( GWater.layer[layer].alpha_fp8 );
    }
    else
    {
      v[cnt].col.r = FP8_TO_FLOAT( FP8_MUL( Mesh_Mem.vrt_lr_fp8[badvertex], GWater.layer[layer].alpha_fp8 ) );
      v[cnt].col.g = FP8_TO_FLOAT( FP8_MUL( Mesh_Mem.vrt_lg_fp8[badvertex], GWater.layer[layer].alpha_fp8 ) );
      v[cnt].col.b = FP8_TO_FLOAT( FP8_MUL( Mesh_Mem.vrt_lb_fp8[badvertex], GWater.layer[layer].alpha_fp8 ) );
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
      for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
      {
        vertex = Mesh_Cmd[type].vrt[entry];
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
  // DWORD GFog.spec;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type  = 0;                           // Command type ( index to points in fan )
  offu  = GWater.layer[layer].u;          // Texture offsets
  offv  = GWater.layer[layer].v;          //
  frame = GWater.layer[layer].frame;     // Frame

  texture  = layer + TX_WATER_TOP;                    // Water starts at texture 5
  vertices = Mesh_Cmd[type].vrt_count;// Number of vertices
  commands = Mesh_Cmd[type].count;          // Number of commands


  badvertex = Mesh_Fan[fan].vrt_start;          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = Mesh_Mem.vrt_x[badvertex];
    v[cnt].pos.y = Mesh_Mem.vrt_y[badvertex];
    v[cnt].pos.z = GWater.layer[layer].zadd[frame][mode][cnt] + GWater.layer[layer].z;

    v[cnt].col.r = v[cnt].col.g = v[cnt].col.b = 1.0f;
    v[cnt].col.a = FP8_TO_FLOAT( GWater.layer[layer].alpha_fp8 );

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
    if ( mesh.last_texture != texture )
    {
      GLTexture_Bind( &TxTexture[texture], CData.texturefilter );
      mesh.last_texture = texture;
    }

    entry = 0;
    for ( cnt = 0; cnt < commands; cnt++ )
    {
      glBegin( GL_TRIANGLE_FAN );
      for ( tnc = 0; tnc < Mesh_Cmd[type].size[cnt]; tnc++ )
      {
        vertex = Mesh_Cmd[type].vrt[entry];
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
//  for (cnt = 0; cnt < renderlist.num_totl; cnt++)
//  {
//    fan = renderlist.totl[cnt];
//    mesh_remove_renderlist(fan);
//  }
//  renderlist.num_totl = 0;
//  renderlist.num_shine = 0;
//  renderlist.num_reflc = 0;
//  renderlist.num_norm = 0;
//  renderlist.num_watr = 0;
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
//      if (fany >= 0 && fany < mesh.size_y)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= mesh.size_x)  fanx = mesh.size_x - 1;
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
//      if (fany >= 0 && fany < mesh.size_y)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= mesh.size_x - 1)  fanx = mesh.size_x - 1;//-2
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
//  if (fany >= mesh.size_y) fany = mesh.size_y - 1;
//  row = 0;
//  while (row < numrow)
//  {
//    cnt = mesh_convert_fan(fanrowstart[row], fany);
//    run = fanrowrun[row];
//    fanx = 0;
//    while (fanx < run)
//    {
//      if (renderlist.num_totl<MAXMESHRENDER)
//      {
//        bool_t is_shine, is_noref, is_norml, is_water;
//
//        is_shine = mesh_has_some_bits(cnt, MESHFX_SHINY);
//        is_noref = mesh_has_no_bits(cnt, MESHFX_NOREFLECT);
//        is_norml = !is_shine;
//        is_water = mesh_has_some_bits(cnt, MESHFX_WATER);
//
//        // Put each tile in basic list
//        renderlist.totl[renderlist.num_totl] = cnt;
//        renderlist.num_totl++;
//        mesh_add_renderlist(cnt);
//
//        // Put each tile in one other list, for shadows and relections
//        if (!is_noref)
//        {
//          renderlist.reflc[renderlist.num_reflc] = cnt;
//          renderlist.num_reflc++;
//        }
//
//        if (is_shine)
//        {
//          renderlist.shine[renderlist.num_shine] = cnt;
//          renderlist.num_shine++;
//        }
//
//        if (is_norml)
//        {
//          renderlist.norm[renderlist.num_norm] = cnt;
//          renderlist.num_norm++;
//        }
//
//        if (is_water)
//        {
//          renderlist.watr[renderlist.num_watr] = cnt;
//          renderlist.num_watr++;
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

  renderlist.num_totl = 0;
  renderlist.num_shine = 0;
  renderlist.num_reflc = 0;
  renderlist.num_norm = 0;
  renderlist.num_watr = 0;

  fan_count = mesh.size_x * mesh.size_y;
  for ( fan = 0; fan < fan_count; fan++ )
  {
    inview = Frustum_BBoxInFrustum( &gFrustum, Mesh_Fan[fan].bbox.mins.v, Mesh_Fan[fan].bbox.maxs.v );

    Mesh_Fan[fan].inrenderlist = bfalse;

    if ( inview && renderlist.num_totl < MAXMESHRENDER )
    {
      bool_t is_shine, is_noref, is_norml, is_water;

      Mesh_Fan[fan].inrenderlist = btrue;

      is_shine = mesh_has_all_bits( fan, MESHFX_SHINY );
      is_noref = mesh_has_all_bits( fan, MESHFX_NOREFLECT );
      is_norml = !is_shine;
      is_water = mesh_has_some_bits( fan, MESHFX_WATER );

      // Put each tile in basic list
      renderlist.totl[renderlist.num_totl] = fan;
      renderlist.num_totl++;
      mesh_add_renderlist( fan );

      // Put each tile
      if ( !is_noref )
      {
        renderlist.reflc[renderlist.num_reflc] = fan;
        renderlist.num_reflc++;
      }

      if ( is_shine )
      {
        renderlist.shine[renderlist.num_shine] = fan;
        renderlist.num_shine++;
      }

      if ( is_norml )
      {
        renderlist.norm[renderlist.num_norm] = fan;
        renderlist.num_norm++;
      }

      if ( is_water )
      {
        renderlist.watr[renderlist.num_watr] = fan;
        renderlist.num_watr++;
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


  if ( fanx >= 0 && fanx < mesh.size_x && fany >= 0 && fany < mesh.size_y )
  {
    fan = mesh_convert_fan( fanx, fany );
    vertex = Mesh_Fan[fan].vrt_start;
    lastvertex = vertex + Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
    while ( vertex < lastvertex )
    {
      light_r0 = light_r = Mesh_Mem.vrt_ar_fp8[vertex];
      light_g0 = light_g = Mesh_Mem.vrt_ag_fp8[vertex];
      light_b0 = light_b = Mesh_Mem.vrt_ab_fp8[vertex];

      dif.x = PrtList[particle].pos.x - Mesh_Mem.vrt_x[vertex];
      dif.y = PrtList[particle].pos.y - Mesh_Mem.vrt_y[vertex];
      dif.z = PrtList[particle].pos.z - Mesh_Mem.vrt_z[vertex];

      dist2 = DotProduct( dif, dif );

      flight = PrtList[particle].dyna.level;
      flight *= 127 * PrtList[particle].dyna.falloff / ( 127 * PrtList[particle].dyna.falloff + dist2 );

      if ( dist2 > 0.0f )
      {
        float ftmp, dist = sqrt( dist2 );
        vect3 pos = {Mesh_Mem.vrt_x[vertex], Mesh_Mem.vrt_y[vertex], Mesh_Mem.vrt_z[vertex]};

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

      Mesh_Mem.vrt_lr_fp8[vertex] = 0.9 * light_r0 + 0.1 * light_r;
      Mesh_Mem.vrt_lg_fp8[vertex] = 0.9 * light_g0 + 0.1 * light_g;
      Mesh_Mem.vrt_lb_fp8[vertex] = 0.9 * light_b0 + 0.1 * light_b;

      if ( mesh.exploremode )
      {
        if ( Mesh_Mem.vrt_lr_fp8[vertex] > light_r0 ) Mesh_Mem.vrt_ar_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ar_fp8[vertex] + 0.1 * Mesh_Mem.vrt_lr_fp8[vertex];
        if ( Mesh_Mem.vrt_lg_fp8[vertex] > light_g0 ) Mesh_Mem.vrt_ag_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ag_fp8[vertex] + 0.1 * Mesh_Mem.vrt_lg_fp8[vertex];
        if ( Mesh_Mem.vrt_lb_fp8[vertex] > light_b0 ) Mesh_Mem.vrt_ab_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ab_fp8[vertex] + 0.1 * Mesh_Mem.vrt_lb_fp8[vertex];
      };

      vertex++;
    }
  }
}


//--------------------------------------------------------------------------------------------
void do_dynalight()
{
  // ZZ> This function does GDyna.mic lighting of visible fans

  int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
  float light_r, light_g, light_b;
  float dist2;
  vect3 dif, nrm;


  // Do each floor tile
  if ( mesh.exploremode )
  {
    // Set base light level in explore mode...  Don't need to do every frame
    if (( allframe & 7 ) == 0 )
    {

      for ( cnt = 0; cnt < MAXPRT; cnt++ )
      {
        if ( !VALID_PRT( cnt ) || !PrtList[cnt].dyna.on ) continue;

        fanx = MESH_FLOAT_TO_FAN( PrtList[cnt].pos.x );
        fany = MESH_FLOAT_TO_FAN( PrtList[cnt].pos.y );

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
      while ( entry < renderlist.num_totl )
      {
        fan = renderlist.totl[entry];
        vertex = Mesh_Fan[fan].vrt_start;
        lastvertex = vertex + Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        while ( vertex < lastvertex )
        {
          vect3 pos = {Mesh_Mem.vrt_x[vertex], Mesh_Mem.vrt_y[vertex], Mesh_Mem.vrt_z[vertex]};

          // Do light particles
          light_r = Mesh_Mem.vrt_ar_fp8[vertex];
          light_g = Mesh_Mem.vrt_ag_fp8[vertex];
          light_b = Mesh_Mem.vrt_ab_fp8[vertex];

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
          while ( cnt < GDyna.count )
          {
            float flight;

            dif.x = GDynaLight[cnt].pos.x - Mesh_Mem.vrt_x[vertex];
            dif.y = GDynaLight[cnt].pos.y - Mesh_Mem.vrt_y[vertex];
            dif.z = GDynaLight[cnt].pos.z - Mesh_Mem.vrt_z[vertex];

            dist2 = DotProduct( dif, dif );

            flight = GDynaLight[cnt].level;
            flight *= 127 * GDynaLight[cnt].falloff / ( 127 * GDynaLight[cnt].falloff + dist2 );

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
          Mesh_Mem.vrt_lr_fp8[vertex] = 0.9 * Mesh_Mem.vrt_lr_fp8[vertex] + 0.1 * light_r;

          if ( light_g > 255 ) light_g = 255;
          if ( light_g <   0 ) light_g = 0;
          Mesh_Mem.vrt_lg_fp8[vertex] = 0.9 * Mesh_Mem.vrt_lg_fp8[vertex] + 0.1 * light_g;

          if ( light_b > 255 ) light_b = 255;
          if ( light_b <   0 ) light_b = 0;
          Mesh_Mem.vrt_lb_fp8[vertex] = 0.9 * Mesh_Mem.vrt_lb_fp8[vertex] + 0.1 * light_b;

          if ( mesh.exploremode )
          {
            if ( light_r > Mesh_Mem.vrt_ar_fp8[vertex] ) Mesh_Mem.vrt_ar_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ar_fp8[vertex] + 0.1 * light_r;
            if ( light_g > Mesh_Mem.vrt_ag_fp8[vertex] ) Mesh_Mem.vrt_ag_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ag_fp8[vertex] + 0.1 * light_g;
            if ( light_b > Mesh_Mem.vrt_ab_fp8[vertex] ) Mesh_Mem.vrt_ab_fp8[vertex] = 0.9 * Mesh_Mem.vrt_ab_fp8[vertex] + 0.1 * light_b;
          }

          vertex++;
        }
        entry++;
      }
    }
    else
    {
      entry = 0;
      while ( entry < renderlist.num_totl )
      {
        fan = renderlist.totl[entry];
        vertex = Mesh_Fan[fan].vrt_start;
        lastvertex = vertex + Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        while ( vertex < lastvertex )
        {
          // Do light particles
          Mesh_Mem.vrt_lr_fp8[vertex] = Mesh_Mem.vrt_ar_fp8[vertex];
          Mesh_Mem.vrt_lg_fp8[vertex] = Mesh_Mem.vrt_ag_fp8[vertex];
          Mesh_Mem.vrt_lb_fp8[vertex] = Mesh_Mem.vrt_ab_fp8[vertex];

          vertex++;
        }
        entry++;
      }
    }
  }
};

