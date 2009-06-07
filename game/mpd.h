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

#include "egoboo_typedef.h"
#include "egoboo_math.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define TILE_BITS     7
#define TILE_SIZE     ((float)(1<<(TILE_BITS)))
#define BLOCK_BITS    9
#define BLOCK_SIZE    ((float)(1<<(BLOCK_BITS)))

#define MAPID                     0x4470614d                   // The string... MapD
#define MESH_MAXTOTALVERTRICES    1024*100
#define MAXMESHFAN                (512*512)                  // Terrain mesh size
#define MAXMESHTILEY              1024                       // Max tiles in y direction
#define MAXMESHBLOCKY             (( MAXMESHTILEY >> (BLOCK_BITS-TILE_BITS) )+1)  // max blocks in the y direction
#define BYTESFOREACHVERTEX        14                         // 14 bytes each
#define MAXMESHVERTICES           16                         // Fansquare vertices
#define MAXMESHTYPE               64                         // Number of fansquare command types
#define MAXMESHCOMMAND            4                          // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES     32                         // Fansquare command list size
#define MAXMESHCOMMANDSIZE        32                         // Max trigs in each command
#define MAXTILETYPE               256                        // Max number of tile images
#define MAXMESHRENDER             1024                       // Max number of tiles to draw
#define FANOFF                    0xFFFF                     // Don't draw the fansquare if tile = this

// mesh physics
#define SLOPE                           50            // increments for terrain slope
#define SLIDE                           0.04f         // Acceleration for steep hills
#define SLIDEFIX                        0.08f         // To make almost flat surfaces flat
#define TWIST_FLAT                      119

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define VALID_TILE(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

enum e_mpd_fx
{
    MPDFX_REF             =       0,                // 0 This tile is drawn 1st
    MPDFX_SHA             = (1 << 0),               // 0 This tile is drawn 2nd
    MPDFX_DRAWREF         = (1 << 1),               // 1 Draw reflection of characters
    MPDFX_ANIM            = (1 << 2),               // 2 Animated tile ( 4 frame )
    MPDFX_WATER           = (1 << 3),               // 3 Render water above surface ( Water details are set per module )
    MPDFX_WALL            = (1 << 4),               // 4 Wall ( Passable by ghosts, particles )
    MPDFX_IMPASS          = (1 << 5),               // 5 Impassable
    MPDFX_DAMAGE          = (1 << 6),               // 6 Damage
    MPDFX_SLIPPY          = (1 << 7),               // 7 Ice or normal
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_light_cache
{
    float max_light;
    float lighting[6];   // light from +x,-x, +y,-y, +z,-z
};
typedef struct s_light_cache light_cache_t;

//--------------------------------------------------------------------------------------------
struct s_mesh_info
{
    Uint8           exploremode;                      // Explore mode?

    size_t          vertcount;                         // For malloc

    int             tiles_x;                          // Size in tiles
    int             tiles_y;
    Uint32          tiles_count;                      // Number of tiles

    int             blocks_x;                         // Size in blocks
    int             blocks_y;
    Uint32          blocks_count;                     // Number of blocks (collision areas)

    float           edge_x;                           // Limits
    float           edge_y;
};
typedef struct s_mesh_info mesh_info_t;

//--------------------------------------------------------------------------------------------
struct s_tile_info
{
    Uint8   type;                              // Tile type
    Uint16  img;                               // Get texture from this
    Uint8   fx;                                 // Special effects flags
    Uint8   twist;
    Uint32  vrtstart;                           // Which vertex to start at

    bool_t  inrenderlist;
};
typedef struct s_tile_info tile_info_t;

//--------------------------------------------------------------------------------------------
struct s_mesh_mem
{
    size_t          vertcount;                                      // For malloc

    tile_info_t *   tile_list;                               // Command type

    Uint32*         blockstart;
    Uint32*         tilestart;                         // Which fan to start a row with

    float*          vrt_x;                                 // Vertex position
    float*          vrt_y;
    float*          vrt_z;                                 // Vertex elevation
    Uint8*          vrt_a;                                 // Vertex base light
    Uint8*          vrt_l;                                 // Vertex light

    light_cache_t * cache;
    GLvector3     * nrm;
};
typedef struct s_mesh_mem mesh_mem_t;

//--------------------------------------------------------------------------------------------
struct s_mesh
{
    mesh_info_t info;
    mesh_mem_t  mem;

    float       tileoff_u[MAXTILETYPE];                          // Tile texture offset
    float       tileoff_v[MAXTILETYPE];
};
typedef struct s_mesh mesh_t;

//--------------------------------------------------------------------------------------------
struct s_tile_definition
{
    Uint8           numvertices;                // Number of vertices
    float           u[MAXMESHVERTICES];         // Vertex texture posi
    float           v[MAXMESHVERTICES];

    Uint8           command_count;                        // Number of commands
    Uint8           command_entries[MAXMESHCOMMAND];      // Entries in each command
    Uint16          command_verts[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
};
typedef struct s_tile_definition tile_definition_t;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern GLvector3 map_twist_nrm[256];
extern Uint32 map_twist_y[256];            // For surface normal of mesh
extern Uint32 map_twist_x[256];
extern float  map_twistvel_x[256];            // For sliding down steep hills
extern float  map_twistvel_y[256];
extern float  map_twistvel_z[256];
extern Uint8  map_twist_flat[256];

extern tile_definition_t tile_dict[MAXMESHTYPE];

extern mesh_t * PMesh;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mesh_t * mesh_new( mesh_t * pmesh );
mesh_t * mesh_renew( mesh_t * pmesh );
mesh_t * mesh_delete( mesh_t * pmesh );
mesh_t * mesh_create( mesh_t * pmesh, int tiles_x, int tiles_y );

// loading/saving
mesh_t * mesh_load( const char *modname, mesh_t * pmesh );

float get_level( mesh_t * pmesh, float x, float y, bool_t waterwalk );
Uint32 mesh_get_block( mesh_t * pmesh, float pos_x, float pos_y );
Uint32 mesh_get_tile ( mesh_t * pmesh, float pos_x, float pos_y );

Uint32 mesh_get_block_int( mesh_t * pmesh, int block_x, int block_y );
Uint32 mesh_get_tile_int( mesh_t * pmesh, int tile_x,  int tile_y );

Uint32 mesh_test_fx( mesh_t * pmesh, Uint32 itile, Uint32 flags );