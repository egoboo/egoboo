/* Egoboo - id_md2.h
 * Md2 model file loading structures & constants.  This is not currently in use.
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

#include "egoboo_types.h"

#ifndef Egoboo_id_md2_h
#define Egoboo_id_md2_h

#pragma pack(push,1)
typedef struct id_md2_header_t
{
  int magic; // should be 844121161( or "IDP2")
  int version; // should be 8
  int skinWidth;
  int skinHeight;
  int frameSize;
  int numSkins;
  int numVertices;
  int numTexCoords;
  int numTriangles;
  Uint32 numGlCommands;
  int numFrames;
  int offsetSkins;
  int offsetTexCoords;
  int offsetTriangles;
  int offsetFrames;
  int offsetGlCommands;
  int offsetEnd;
} md2_header;

typedef struct id_md2_skinname_t
{
  char name[64];
} md2_skinname;

typedef struct id_md2_vertex_t
{
  unsigned char vertex[3];
  unsigned char lightNormalIndex;
} md2_vertex;

typedef struct id_md2_texcoord_t
{
  short s, t;
} md2_texcoord;

typedef struct id_md2_triangle
{
  short vertexIndices[3];
  short texCoordIndices[3];
} md2_triangle;

typedef struct id_md2_frame_t
{
  float scale[3];
  float translate[3];
  char name[16];
  md2_vertex vertices[];
} md2_frame;

typedef struct id_md2_gldata_t
{
  float  s,t;
  int    index;
} md2_gldata;
#pragma pack(pop)

enum id_md2_constant_e
{
  MD2_MAX_TRIANGLES = 4096,
  MD2_MAX_VERTICES = 2048,
  MD2_MAX_TEXCOORDS = 2048,
  MD2_MAX_FRAMES  = 512,
  MD2_MAX_SKINS  = 32,
  MD2_MAX_FRAMESIZE = ( MD2_MAX_VERTICES * 4 + 128 ),
  MD2_MAGIC_NUMBER = 844121161, // "IDP2"
  MD2_VERSION   = 8
};

extern const float kMd2Normals[][3];

#endif
