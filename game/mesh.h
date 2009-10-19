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

#include "mpd_file.h"
#include "ogl_include.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define BLOCK_BITS    9
#define BLOCK_SIZE    ((float)(1<<(BLOCK_BITS)))

#define MAXMESHBLOCKY             (( MAXMESHTILEY >> (BLOCK_BITS-TILE_BITS) )+1)  ///< max blocks in the y direction

/// mesh physics
#define SLIDE                           0.04f         ///< Acceleration for steep hills
#define SLIDEFIX                        0.08f         ///< To make almost flat surfaces flat
#define TWIST_FLAT                      119

#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )

#define TILE_IS_FANOFF(XX)              ( FANOFF == (XX).img )

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LIGHTING_VEC_SIZE       7
typedef float lighting_vector_t[LIGHTING_VEC_SIZE];     ///< light from +x,-x, +y,-y, +z,-z, ambient

//--------------------------------------------------------------------------------------------
struct s_lighting_cache_base
{
    float max_light;              ///< max amplitude of direct light
    lighting_vector_t lighting;   ///< light from +x,-x, +y,-y, +z,-z, ambient
};
typedef struct s_lighting_cache_base lighting_cache_base_t;

//--------------------------------------------------------------------------------------------
struct s_lighting_cache
{
    float max_light;              ///< max amplitude of direct light

    lighting_cache_base_t low;
    lighting_cache_base_t hgh;
};
typedef struct s_lighting_cache lighting_cache_t;

typedef GLXvector3f normal_cache_t[4];
typedef float       light_cache_t[4];

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
    size_t            grid_count;                       ///< how many grids

    grid_lighting_t * light;                            ///< the lighting info for this grid

    Uint32          * blockstart;                       ///< list of blocks that start each row
    Uint32          * tilestart;                        ///< list of tiles  that start each row
};
typedef struct s_grid_mem grid_mem_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A wrapper for the dynamically allocated mesh memory
struct s_mesh_mem
{
    aabb_t           bbox;

    // the per-tile info
    size_t           tile_count;                       ///< number of tiles
    tile_info_t    * tile_list;                        ///< tile command info
    aabb_t         * bb_list;                          ///< the bounding box for the tile
    normal_cache_t * ncache;                           ///< the normals at the corners of a tile
    light_cache_t  * lcache;                           ///< the light at the corners of a tile

    // the per-vertex info to be presented to OpenGL
    size_t          vert_count;                        ///< number of vertices
    GLXvector3f   * plst;                              ///< the position list
    GLXvector2f   * tlst;                              ///< the texture coordinate list
    GLXvector3f   * clst;                              ///< the color list (for lighting the mesh)
    GLXvector3f   * nlst;                              ///< the normal list
};
typedef struct s_mesh_mem mesh_mem_t;

//--------------------------------------------------------------------------------------------
/// The generic parameters describing an ego_mpd
struct s_ego_mpd_info
{
    size_t          vertcount;                         ///< For malloc

    int             tiles_x;                          ///< Size in tiles
    int             tiles_y;
    Uint32          tiles_count;                      ///< Number of tiles

    int             blocks_x;                         ///< Size in blocks
    int             blocks_y;
    Uint32          blocks_count;                     ///< Number of blocks (collision areas)

    float           edge_x;                           ///< Limits
    float           edge_y;
};
typedef struct s_ego_mpd_info ego_mpd_info_t;

//--------------------------------------------------------------------------------------------
/// egoboo's representation of the .mpd mesh file
struct s_ego_mpd
{
    ego_mpd_info_t  info;
    mesh_mem_t      mmem;
    grid_mem_t      gmem;

    fvec2_t         tileoff[MAXTILETYPE];     ///< Tile texture offset
};
typedef struct s_ego_mpd ego_mpd_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern fvec3_t   map_twist_nrm[256];
extern Uint32    map_twist_y[256];            ///< For surface normal of mesh
extern Uint32    map_twist_x[256];
extern float     map_twistvel_x[256];            ///< For sliding down steep hills
extern float     map_twistvel_y[256];
extern float     map_twistvel_z[256];
extern Uint8     map_twist_flat[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ego_mpd_t * mesh_new( ego_mpd_t * pmesh );
ego_mpd_t * mesh_renew( ego_mpd_t * pmesh );
ego_mpd_t * mesh_delete( ego_mpd_t * pmesh );
ego_mpd_t * mesh_create( ego_mpd_t * pmesh, int tiles_x, int tiles_y );

/// loading/saving
ego_mpd_t * mesh_load( const char *modname, ego_mpd_t * pmesh );

float  mesh_get_level( ego_mpd_t * pmesh, float x, float y );
Uint32 mesh_get_block( ego_mpd_t * pmesh, float pos_x, float pos_y );
Uint32 mesh_get_tile ( ego_mpd_t * pmesh, float pos_x, float pos_y );

Uint32 mesh_get_block_int( ego_mpd_t * pmesh, int block_x, int block_y );
Uint32 mesh_get_tile_int( ego_mpd_t * pmesh, int tile_x,  int tile_y );

Uint32 mesh_test_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags );

void   mesh_make_twist();

bool_t mesh_light_corners( ego_mpd_t * pmesh, int fan1 );
bool_t mesh_interpolate_vertex( mesh_mem_t * pmem, int fan, float pos[], float * plight );

float evaluate_lighting_cache_base( lighting_cache_base_t * lvec, GLfloat nrm[], float * amb  );
float evaluate_lighting_cache( lighting_cache_t * src, GLfloat nrm[], float z, aabb_t bbox, float * light_amb, float * light_dir );

bool_t grid_light_one_corner( ego_mpd_t * pmesh, int fan, float height, float nrm[], float * plight );

bool_t mesh_set_texture( ego_mpd_t * pmesh, Uint16 tile, Uint16 image );
bool_t mesh_update_texture( ego_mpd_t * pmesh, Uint16 tile );
