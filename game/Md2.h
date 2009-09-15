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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - Md2.h
 *
 */

#include "id_md2.h"
#include "egoboo_typedef.h"
#include "egoboo.h"

#define MAXVERTICES                     2048        // Max number of points in a model
#define MAXFRAME                        (128*32)    // Max number of frames in all models

#define MAXCOMMAND                      512         // Max number of commands
#define MAXCOMMANDSIZE                  128          // Max number of points in a command
#define MAXCOMMANDENTRIES               512         // Max entries in a command list ( trigs )

#define MD2_MAGIC_NUMBER                        0x32504449      // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)      // Don't load any models bigger than 512k

#define MADLIGHTINDICES                 (MD2_MAX_NORMALS + 1) // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162                // I added an extra index to do the spikey mace...

struct s_md2_ogl_commandlist
{
    Uint16  count;                  // Number of commands
    Uint16  entries;                // Number of command entries

    GLenum  type[MAXCOMMAND];       // Fan or strip
    Uint16  size[MAXCOMMAND];       // Entries used by command

    Uint16  vrt[MAXCOMMANDENTRIES]; // Which vertex
    float   u[MAXCOMMANDENTRIES];   // Texture position
    float   v[MAXCOMMANDENTRIES];
};
typedef struct s_md2_ogl_commandlist md2_ogl_commandlist_t;

struct s_ego_md2
{
    md2_ogl_commandlist_t cmd;

    Uint16  frames;                        // Number of frames
    Uint16  framestart;                    // Starting frame of model
    Uint16  vertices;                      // Number of vertices
};
typedef struct s_ego_md2 ego_md2_t;

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

struct s_md2_frame
{
    Uint8   framelip;                      // 0-15, How far into action is each frame
    Uint16  framefx;                       // Invincibility, Spawning

    float   vrtx[MAXVERTICES];             // Vertex position
    float   vrty[MAXVERTICES];
    float   vrtz[MAXVERTICES];
    Uint8   vrta[MAXVERTICES];             // Light index of vertex
};
typedef struct s_md2_frame md2_frame_t;

extern md2_frame_t Md2FrameList[MAXFRAME];

extern Uint16 md2_loadframe;                               // Where to load next

extern float kMd2Normals[MADLIGHTINDICES][3];

// Function prototypes
int    md2_rip_frame_name( int frame );
void   md2_rip_frames( ego_md2_t * pflist );
bool_t md2_load_one( const char* szLoadname, ego_md2_t * pmd2 );

Md2Model *md2_loadFromFile( const char *fileName );
void      md2_freeModel( Md2Model *model );

#define EGOBOO_MD2_H
