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

#include "mpd.h"
#include "ogl_include.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define BLOCK_BITS    9
#define BLOCK_SIZE    ((float)(1<<(BLOCK_BITS)))

#define MAXMESHBLOCKY             (( MAXMESHTILEY >> (BLOCK_BITS-TILE_BITS) )+1)  // max blocks in the y direction

// mesh physics
#define SLIDE                           0.04f         // Acceleration for steep hills
#define SLIDEFIX                        0.08f         // To make almost flat surfaces flat
#define TWIST_FLAT                      119

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// axis aligned bounding box
struct s_aabb
{
    float mins[3];
    float maxs[3];
};
typedef struct s_aabb aabb_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_lighting_cache
{
    float max_light;

    float lighting_low[6];   // light from +x,-x, +y,-y, +z,-z
    float lighting_hgh[6];   // light from +x,-x, +y,-y, +z,-z
};
typedef struct s_lighting_cache lighting_cache_t;

typedef GLXvector3f normal_cache_t[4];
typedef Uint8       light_cache_t[4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_grid_lighting
{
    // the lighting info in the upper left hand corner of a grid
    Uint8         a, l;
    lighting_cache_t cache;
};
typedef struct s_grid_lighting grid_lighting_t;

//--------------------------------------------------------------------------------------------
struct s_grid_mem
{
    size_t            grid_count;                       // how many grids

    grid_lighting_t * light;                            // the lighting info for this grid

    Uint32          * blockstart;                       // list of blocks that start each row
    Uint32          * tilestart;                        // list of tiles  that start each row
};
typedef struct s_grid_mem grid_mem_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh memory
struct s_mesh_mem
{
    aabb_t           bbox;

    // the per-tile info
    size_t           tile_count;                       // number of tiles
    tile_info_t    * tile_list;                        // tile command info
    aabb_t         * bb_list;                          // the bounding box for the tile
    normal_cache_t * ncache;                           // the normals at the corners of a tile
    light_cache_t  * lcache;                           // the light at the corners of a tile

    // the per-vertex info to be presented to OpenGL
    size_t          vert_count;                        // number of vertices
    GLXvector3f   * plst;                              // the position list
    GLXvector2f   * tlst;                              // the texture coordinate list
    GLXvector3f   * clst;                              // the color list
    GLXvector3f   * nlst;                              // the normal list
};
typedef struct s_mesh_mem mesh_mem_t;

//--------------------------------------------------------------------------------------------
struct s_ego_mpd_info
{
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
typedef struct s_ego_mpd_info ego_mpd_info_t;

//--------------------------------------------------------------------------------------------
struct s_ego_mpd
{
    ego_mpd_info_t  info;
    mesh_mem_t      mmem;
    grid_mem_t      gmem;

    GLvector2       tileoff[MAXTILETYPE];     // Tile texture offset
};
typedef struct s_ego_mpd ego_mpd_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern GLvector3 map_twist_nrm[256];
extern Uint32    map_twist_y[256];            // For surface normal of mesh
extern Uint32    map_twist_x[256];
extern float     map_twistvel_x[256];            // For sliding down steep hills
extern float     map_twistvel_y[256];
extern float     map_twistvel_z[256];
extern Uint8     map_twist_flat[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ego_mpd_t * mesh_new( ego_mpd_t * pmesh );
ego_mpd_t * mesh_renew( ego_mpd_t * pmesh );
ego_mpd_t * mesh_delete( ego_mpd_t * pmesh );
ego_mpd_t * mesh_create( ego_mpd_t * pmesh, int tiles_x, int tiles_y );

// loading/saving
ego_mpd_t * mesh_load( const char *modname, ego_mpd_t * pmesh );

float  mesh_get_level( ego_mpd_t * pmesh, float x, float y );
Uint32 mesh_get_block( ego_mpd_t * pmesh, float pos_x, float pos_y );
Uint32 mesh_get_tile ( ego_mpd_t * pmesh, float pos_x, float pos_y );

Uint32 mesh_get_block_int( ego_mpd_t * pmesh, int block_x, int block_y );
Uint32 mesh_get_tile_int( ego_mpd_t * pmesh, int tile_x,  int tile_y );

Uint32 mesh_test_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags );

void   mesh_make_twist();

void tile_dictionary_load(tile_definition_t dict[], size_t dict_size);

bool_t mesh_light_corners( ego_mpd_t * pmesh, int fan1 );
bool_t mesh_interpolate_vertex( mesh_mem_t * pmem, int fan, GLXvector3f pos, float * plight );
