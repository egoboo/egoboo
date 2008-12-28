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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "graphic.h"

//--------------------------------------------------------------------------------------------
void render_fan( Uint32 fan )
{
  // ZZ> This function draws a mesh fan
  GLVERTEX v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32  badvertex;
  float offu, offv;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = meshtile[fan];               // Tile
  Uint8  fx = meshfx[fan];                   // Fx bits
  Uint16 type = meshtype[fan];               // Command type ( index to points in fan )

  if ( tile == FANOFF || tile == INVALID_TILE )
    return;

  // Animate the tiles
  if ( fx & MESHFXANIM )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & biganimtilebaseand;// Animation set
      tile += animtileframeadd << 1;         // Animated tile
      tile = ( tile & biganimtileframeand ) + basetile;
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

  texture = ( tile >> 6 ) + TX_TILE_0;    // 64 tiles in each 256x256 texture
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands

  // Original points
  badvertex = meshvrtstart[fan];          // Get big reference value

  //[claforte] Put this in an initialization function.
  glEnableClientState( GL_VERTEX_ARRAY );

  glVertexPointer( 3, GL_FLOAT, sizeof( GLfloat )*7 + 4, &v[0].x );
  glTexCoordPointer( 2, GL_FLOAT, sizeof( GLVERTEX ) - 2*sizeof( GLfloat ), &v[0].s );

  {
    for ( cnt = 0; cnt < vertices; cnt++ )
    {
      v[cnt].x = meshvrtx[badvertex];
      v[cnt].y = meshvrty[badvertex];
      v[cnt].z = meshvrtz[badvertex];

      v[cnt].r = 
	  v[cnt].g = 
	  v[cnt].b = FP8_TO_FLOAT(meshvrtl[badvertex]);

      v[cnt].s = meshcommandu[type][badvertex] + offu;
      v[cnt].t = meshcommandv[type][badvertex] + offv;
      badvertex++;
    }
  }

  // Change texture if need be
  if ( meshlasttexture != texture )
  {
    GLTexture_Bind( &txTexture[texture] );
	meshlasttexture = texture;
  }

  // Make new ones so we can index them and not transform 'em each time
  // if(transform_vertices(vertices, v, vt))
  //  return;


  // Render each command
  entry = 0;
  for ( cnt = 0; cnt < commands; cnt++ )
  {
    glBegin ( GL_TRIANGLE_FAN );
    for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
    {
      vertex = meshcommandvrt[type][entry];
      glColor3fv( &v[vertex].r );
      glTexCoord2f ( meshcommandu[type][vertex] + offu, meshcommandv[type][vertex] + offv );
      glVertex3fv ( &v[vertex].x );
      entry++;
    }
    glEnd();
  }

}

//--------------------------------------------------------------------------------------------
void render_water_fan( Uint32 fan, Uint8 layer, Uint8 mode )
{
  // ZZ> This function draws a water fan
  GLVERTEX v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32  badvertex;
  float offu, offv;
  Uint32  ambi;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number


  // To make life easier
  type = 0;                           // Command type ( index to points in fan )
  offu = waterlayeru[layer];          // Texture offsets
  offv = waterlayerv[layer];          //
  frame = waterlayerframe[layer];     // Frame

  texture = layer + TX_WATER_TOP;         // Water starts at texture 5
  vertices = meshcommandnumvertices[type];// Number of vertices
  commands = meshcommands[type];          // Number of commands


  // Original points
  badvertex = meshvrtstart[fan];          // Get big reference value
  {
    for ( cnt = 0; cnt < vertices; cnt++ )
    {
      v[cnt].x = meshvrtx[badvertex];
      v[cnt].y = meshvrty[badvertex];
      v[cnt].z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

	  ambi = ( Uint32 ) meshvrtl[badvertex] >> 1;
      ambi += waterlayercolor[layer][frame][mode][cnt];
      v[cnt].r = FP8_TO_FLOAT( ambi );
      v[cnt].g = FP8_TO_FLOAT( ambi );
      v[cnt].b = FP8_TO_FLOAT( ambi );
      v[cnt].a = FP8_TO_FLOAT( waterlayeralpha[layer] );
	  
      badvertex++;
    }
  }
 
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;


  // Change texture if need be
  if ( meshlasttexture != texture )
  {
    GLTexture_Bind(&txTexture[texture]);
	meshlasttexture = texture;
  }



  // Make new ones so we can index them and not transform 'em each time
  // if(transform_vertices(vertices, v, vt))
  //    return;


  // Render each command
  entry = 0;
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;
  for ( cnt = 0; cnt < commands; cnt++ )
  {
    glBegin ( GL_TRIANGLE_FAN );
    for ( tnc = 0; tnc < meshcommandsize[type][cnt]; tnc++ )
    {
      vertex = meshcommandvrt[type][entry];
      glColor4fv( &v[vertex].r );
      glTexCoord2fv ( &v[vertex].s );
      glVertex3fv ( &v[vertex].x );
      entry++;
    }
    glEnd ();
  }
}
