#pragma once

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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - Md2.h
 * 
 */

/* Typedefs for various platforms */
#include "egobootypedef.h"

typedef struct Md2Vertex
{
    float x, y, z;
    unsigned normal;  // index to id-normal array
} Md2Vertex;

typedef struct Md2TexCoord
{
    float s, t;
} Md2TexCoord;

typedef struct Md2Triangle
{
    short vertexIndices[3];
    short texCoordIndices[3];
} Md2Triangle;

typedef struct Md2Frame
{
    char name[16];
    float min[3], max[3];    // axis-aligned bounding box limits
    Md2Vertex *vertices;
} Md2Frame;

typedef struct Md2SkinName
{
    char name[64];
} Md2SkinName;

typedef struct Md2Model
{
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numSkins;
    int numFrames;

    Md2SkinName *skins;
    Md2TexCoord *texCoords;
    Md2Triangle *triangles;
    Md2Frame    *frames;
} Md2Model;

//Function prototypes
int rip_md2_header();
void fix_md2_normals( Uint16 modelindex );
void rip_md2_commands( Uint16 modelindex );
int rip_md2_frame_name( int frame );
void rip_md2_frames( Uint16 modelindex );
int load_one_md2(  const char* szLoadname, Uint16 modelindex );
void get_madtransvertices( Uint16 modelindex );

extern Md2Model *md2_loadFromFile( const char *fileName );
extern void      md2_freeModel( Md2Model *model );



#define EGOBOO_MD2_H
