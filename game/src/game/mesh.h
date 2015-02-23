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

/// @file game/mesh.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/lighting.h"

//--------------------------------------------------------------------------------------------
// external types
//--------------------------------------------------------------------------------------------

struct ego_mesh_info_t;
struct oglx_texture_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define GRID_BITS      7
#define GRID_ISIZE     (1<<(GRID_BITS))
#define GRID_FSIZE     ((float)GRID_ISIZE)
#define GRID_MASK      (GRID_ISIZE - 1)

#define BLOCK_BITS      9
#define BLOCK_ISIZE     (1<<(BLOCK_BITS))
#define BLOCK_FSIZE     ((float)BLOCK_ISIZE)
#define BLOCK_MASK      (BLOCK_ISIZE - 1)

#define GRID_BLOCKY_MAX             (( MAP_TILEY_MAX >> (BLOCK_BITS-GRID_BITS) )+1)  ///< max blocks in the y direction

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define VALID_GRID(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

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

#define TILE_IS_FANOFF(XX)              ( MAP_FANOFF == (XX).img )

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
typedef GLXvector3f normal_cache_t[4];
typedef float       light_cache_t[4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data describing an Egoboo tile
struct ego_tile_info_t
{
    // the "inherited" tile info
    Uint8   type;                              ///< Tile type
    Uint16  img;                               ///< Get texture from this
    size_t  vrtstart;                          ///< Which vertex to start at

    // some extra flags
    bool  fanoff;                            ///< display this tile?

    // some info about the renderlist
    bool  inrenderlist;                      ///< Is the tile going to be rendered this frame?
    int   inrenderlist_frame;                ///< What was the frame number the last time this tile was rendered?

    // tile corner lighting parameters
    normal_cache_t ncache;                     ///< the normals at the corners of this tile
    light_cache_t  lcache;                     ///< the light at the corners of this tile
    bool           request_lcache_update;      ///< has this tile been tagged for a lcache update?
    int            lcache_frame;               ///< the last frame in which the lighting cache was updated

    // tile vertex lighting parameters
    bool           request_clst_update;        ///< has this tile been tagged for a color list update?
    int            clst_frame;                 ///< the last frame in which the color list was updated
    light_cache_t  d1_cache;                   ///< the estimated change in the light at the corner of the tile
    light_cache_t  d2_cache;                   ///< the estimated change in the light at the corner of the tile

    // the bounding boc of this tile
    oct_bb_t       oct;                        ///< the octagonal bounding box for this tile
    BSP_leaf_t     bsp_leaf;                   ///< the octree node for this object
};

ego_tile_info_t * ego_tile_info_ctor( ego_tile_info_t * ptr, int index );
ego_tile_info_t * ego_tile_info_dtor( ego_tile_info_t * ptr );
ego_tile_info_t * ego_tile_info_free( ego_tile_info_t * ptr );
ego_tile_info_t * ego_tile_info_create( int index );
ego_tile_info_t * ego_tile_info_destroy( ego_tile_info_t * ptr );

ego_tile_info_t * ego_tile_info_ctor_ary( ego_tile_info_t * ptr, size_t count );
ego_tile_info_t * ego_tile_info_dtor_ary( ego_tile_info_t * ptr, size_t count );
ego_tile_info_t * ego_tile_info_create_ary( size_t count );
ego_tile_info_t * ego_tile_info_destroy_ary( ego_tile_info_t * ary, size_t count );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef BIT_FIELD GRID_FX_BITS;

/// The data describing an Egoboo grid
struct ego_grid_info_t
{
    // MODIFY THESE FLAGS
    GRID_FX_BITS    base_fx;                   ///< the special effects flags in the mpd
    GRID_FX_BITS    wall_fx;                   ///< the working copy of base_fx, which might be modified by digging
    GRID_FX_BITS    pass_fx;                   ///< fx added by passages. used this way, passages cannot remove the wall flags.
    Uint8           twist;                     ///< The orientation of the tile

    // the lighting info in the upper left hand corner of a grid
    Uint8            a, l;                     ///< the raw mesh lighting... pretty much ignored
    lighting_cache_t cache;                    ///< the per-grid lighting info
    int              cache_frame;              ///< the last frame in which the cache was calculated
};

ego_grid_info_t * ego_grid_info_ctor( ego_grid_info_t * ptr );
ego_grid_info_t * ego_grid_info_dtor( ego_grid_info_t * ptr );
ego_grid_info_t * ego_grid_info_free( ego_grid_info_t * ptr );
ego_grid_info_t * ego_grid_info_create();
ego_grid_info_t * ego_grid_info_destroy( ego_grid_info_t * ptr );

ego_grid_info_t * ego_grid_info_ctor_ary( ego_grid_info_t * ptr, size_t count );
ego_grid_info_t * ego_grid_info_dtor_ary( ego_grid_info_t * ptr, size_t count );
ego_grid_info_t * ego_grid_info_create_ary( size_t count );
ego_grid_info_t * ego_grid_info_destroy_ary( ego_grid_info_t * ary, size_t count );

#if 0
template <typename Type>
class Size
{
private:
    /// The size in the x-direction.
    Type _x;
    /// the size in the y-direction.
    Type _y;
public:
    /**
     * @brief
     *  Construct a size.
     * @param x
     *  the size in the x-direction
     * @param y
     *  the size in the y-direction
     */
    Size(const Type& x,const Type& y) :
        _x(x), _y(y)
    {}
    /**
     * @brief
     *  Construct a size (copy-constructor).
     * @param other
     *  the source of the copy operation
     */
    Size(const Size<Type>& other) :
        _x(other.x), _y(other._y)
    {}
    /// @brief Get the size along the x-axis.
    /// @return the size along the x-axis
    const Type& getX() const {
        return _x;
    }
    /// @brief Get the size along the y-axis.
    /// @return the size along the y-axis
    const Type& getY() const {
        return _y;
    }
};
#endif

//--------------------------------------------------------------------------------------------
struct grid_mem_t
{
    int grids_x;         ///< Size in grids
    int grids_y;
    size_t grid_count;   ///< How many grids.

    int blocks_x;        ///< Size in blocks
    int blocks_y;
    Uint32 blocks_count; ///< Number of blocks (collision areas)

    float edge_x; ///< Limits.
    float edge_y;

    Uint32 *blockstart; ///< List of blocks that start each row.
    Uint32 *tilestart;  ///< List of tiles  that start each row.

    // the per-grid info
    ego_grid_info_t* grid_list;                       ///< tile command info

    static grid_mem_t *ctor(grid_mem_t *self);
    static grid_mem_t *dtor(grid_mem_t *self);
    static bool alloc(grid_mem_t *self, const ego_mesh_info_t *info);
    static bool free(grid_mem_t *self);
    /**
     * @brief
     *  This function builds a look up table to ease calculating the fan number given an x,y pair.
     */
    static void make_fanstart(grid_mem_t *self, const ego_mesh_info_t *info);
};




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated mesh memory
struct tile_mem_t
{
    aabb_t           bbox;                             ///< bounding box for the entire mesh

    // the per-tile info
    size_t           tile_count;                       ///< number of tiles
    ego_tile_info_t* tile_list;                        ///< tile command info

    // the per-vertex info to be presented to OpenGL
    size_t vert_count;                        ///< number of vertices
    GLXvector3f *plst;                        ///< the position list
    GLXvector2f *tlst;                        ///< the texture coordinate list
    GLXvector3f *nlst;                        ///< the normal list
    GLXvector3f *clst;                        ///< the color list (for lighting the mesh)

    static tile_mem_t *ctor(tile_mem_t *self);
    static tile_mem_t *dtor(tile_mem_t *self);
    static bool free(tile_mem_t *self);
    static bool alloc(tile_mem_t *self, const ego_mesh_info_t *info);

};



//--------------------------------------------------------------------------------------------

struct mpdfx_list_ary_t
{
    size_t   cnt;

    size_t   idx;
    size_t * lst;
};

mpdfx_list_ary_t * mpdfx_list_ary_ctor( mpdfx_list_ary_t * ptr );
mpdfx_list_ary_t * mpdfx_list_ary_dtor( mpdfx_list_ary_t * ptr );
mpdfx_list_ary_t * mpdfx_list_ary_alloc( mpdfx_list_ary_t * ptr, size_t count );
mpdfx_list_ary_t * mpdfx_list_ary_dealloc( mpdfx_list_ary_t * ptr );
mpdfx_list_ary_t * mpdfx_list_ary_reset( mpdfx_list_ary_t * ptr );
bool mpdfx_list_ary_push( mpdfx_list_ary_t * ptr, size_t value );

//--------------------------------------------------------------------------------------------
struct mpdfx_lists_t
{
    bool   dirty;

    mpdfx_list_ary_t sha;
    mpdfx_list_ary_t drf;
    mpdfx_list_ary_t anm;
    mpdfx_list_ary_t wat;
    mpdfx_list_ary_t wal;
    mpdfx_list_ary_t imp;
    mpdfx_list_ary_t dam;
    mpdfx_list_ary_t slp;
};

mpdfx_lists_t *mpdfx_lists_ctor(mpdfx_lists_t *self);
mpdfx_lists_t *mpdfx_lists_dtor(mpdfx_lists_t *self);
bool mpdfx_lists_alloc(mpdfx_lists_t *self, const ego_mesh_info_t *info);
bool mpdfx_lists_dealloc(mpdfx_lists_t *self);
bool mpdfx_lists_reset(mpdfx_lists_t *self);
int mpdfx_lists_push(mpdfx_lists_t *self, GRID_FX_BITS fx_bits, size_t value);
bool mpdfx_lists_synch( mpdfx_lists_t * plst, const grid_mem_t * pgmem, bool force );

//--------------------------------------------------------------------------------------------

/// The generic parameters describing an ego_mesh
struct ego_mesh_info_t
{
    size_t vertcount;    ///< For malloc

    /**
     * @brief
     *  The size, in tiles, along the x-axis.
     * @todo
     *  Rename to @a sizeX. The type should be @a size_t.
     */
    int tiles_x;
    /**
     * @brief
     *  The size, in tiles, along the y-axis.
     * @todo
     *  Rename to @a sizeY. The type should be @a size_t.
     */
    int tiles_y;
    /**
     * @brief
     *  The number of tiles in the mesh.
     * @invariant
     *  <tt>size = sizeX * sizeY</tt>
     * @todo
     *  Rename to @a size. The type should be @a size_t.
     */
    Uint32 tiles_count;

    static ego_mesh_info_t *ctor(ego_mesh_info_t *self);
    static ego_mesh_info_t *dtor(ego_mesh_info_t *self);
    static void init(ego_mesh_info_t *self, int numvert, size_t tiles_x, size_t tiles_y);
};




//--------------------------------------------------------------------------------------------

/// Egoboo's representation of the .mpd mesh file
struct ego_mesh_t
{
    ego_mesh_info_t info;
    tile_mem_t tmem;
    grid_mem_t gmem;
    mpdfx_lists_t fxlists;
    static ego_mesh_t *ctor(ego_mesh_t *self);
    static ego_mesh_t *dtor(ego_mesh_t *self);
    /// @todo Needs to be removed or re-coded as it invokes ctor and dtor which will become
    ///       proper constructors and destructors.
    static ego_mesh_t *renew(ego_mesh_t *self); 
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// struct for caching fome values for wall collisions

struct mesh_wall_data_t
{
    int   ix_min, ix_max, iy_min, iy_max;
    float fx_min, fx_max, fy_min, fy_max;

    ego_mesh_info_t  * pinfo;
    ego_tile_info_t * tlist;
    ego_grid_info_t * glist;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern fvec3_t   map_twist_nrm[256];
extern FACING_T  map_twist_facing_y[256];              ///< For surface normal of mesh
extern FACING_T  map_twist_facing_x[256];
extern fvec3_t   map_twist_vel[256];            ///< For sliding down steep hills
extern Uint8     map_twist_flat[256];

extern int mesh_mpdfx_tests;
extern int mesh_bound_tests;
extern int mesh_pressure_tests;

// variables to optimize calls to bind the textures
extern bool  mesh_tx_none;           ///< use blank textures?
extern TX_REF  mesh_tx_image;          ///< Last texture used
extern Uint8   mesh_tx_size;           ///< what size texture?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_t *ego_mesh_create( ego_mesh_t * pmesh, int tiles_x, int tiles_y );
bool ego_mesh_destroy( ego_mesh_t ** pmesh );

/// loading/saving
ego_mesh_t *ego_mesh_load( const char *modname, ego_mesh_t * pmesh );

void   ego_mesh_make_twist();

bool ego_mesh_test_corners(ego_mesh_t *mesh, ego_tile_info_t * ptile, float threshold );
float ego_mesh_light_corners(ego_mesh_t *mesh, ego_tile_info_t * ptile, bool reflective, float mesh_lighting_keep );
bool ego_mesh_interpolate_vertex(tile_mem_t *mem, ego_tile_info_t * ptile, float pos[], float * plight );

bool grid_light_one_corner(const ego_mesh_t *mesh, int fan, float height, float nrm[], float * plight );

BIT_FIELD ego_mesh_hit_wall(const ego_mesh_t *mesh, const fvec3_t& pos, const float radius, const BIT_FIELD bits, float nrm[], float *pressure, mesh_wall_data_t * private_data);
BIT_FIELD ego_mesh_test_wall(const ego_mesh_t *mesh, const fvec3_t& pos, const float radius, const BIT_FIELD bits, mesh_wall_data_t *private_data);

float ego_mesh_get_max_vertex_0(const ego_mesh_t *mesh, int grid_x, int grid_y);
float ego_mesh_get_max_vertex_1(const ego_mesh_t *mesh, int grid_x, int grid_y, float xmin, float ymin, float xmax, float ymax);

bool ego_mesh_set_texture(ego_mesh_t *mesh, Uint32 tile, Uint16 image);
bool ego_mesh_update_texture(ego_mesh_t *mesh, Uint32 tile);

fvec3_t ego_mesh_get_diff(const ego_mesh_t *mesh, const fvec3_t& pos, float radius, float center_pressure, const BIT_FIELD bits);
float ego_mesh_get_pressure(const ego_mesh_t *mesh, const fvec3_t& pos, float radius, const BIT_FIELD bits);

bool ego_mesh_update_water_level(ego_mesh_t *mesh);

void mesh_texture_invalidate();
oglx_texture_t * mesh_texture_bind( const ego_tile_info_t * ptile );

//Previously inlined
ego_mesh_t * ego_mesh_ctor_1(ego_mesh_t * pmesh, int tiles_x, int tiles_y);
bool ego_mesh_remove_ambient(ego_mesh_t * pmesh);
bool ego_mesh_recalc_twist(ego_mesh_t * pmesh);
bool ego_mesh_make_texture(ego_mesh_t * pmesh);
ego_mesh_t * ego_mesh_finalize(ego_mesh_t * pmesh);
bool ego_mesh_test_one_corner(ego_mesh_t * pmesh, GLXvector3f pos, float * pdelta);
bool ego_mesh_light_one_corner(const ego_mesh_t * pmesh, ego_tile_info_t * ptile, const bool reflective, const fvec3_t& pos, const fvec3_t& nrm, float * plight);

float  ego_mesh_get_level( const ego_mesh_t * pmesh, float pos_x, float pos_y );
Uint32 ego_mesh_get_block( const ego_mesh_t * pmesh, float pos_x, float pos_y );
Uint32 ego_mesh_get_grid( const ego_mesh_t * pmesh, float pos_x, float pos_y );

Uint32 ego_mesh_get_block_int( const ego_mesh_t * pmesh, int block_x, int block_y );
Uint32 ego_mesh_get_tile_int( const ego_mesh_t * pmesh, int grid_x,  int grid_y );

Uint32 ego_mesh_test_fx( const ego_mesh_t * pmesh, Uint32 itile, const BIT_FIELD flags );
bool ego_mesh_clear_fx( ego_mesh_t * pmesh, Uint32 itile, const BIT_FIELD flags );
bool ego_mesh_add_fx( ego_mesh_t * pmesh, Uint32 itile, const BIT_FIELD flags );

Uint32 ego_mesh_has_some_mpdfx( const BIT_FIELD mpdfx, const BIT_FIELD test );
bool ego_mesh_grid_is_valid( const ego_mesh_t *, Uint32 id );

bool ego_mesh_tile_has_bits( const ego_mesh_t *, const int ix, const int iy, const BIT_FIELD bits );

ego_tile_info_t * ego_mesh_get_ptile( const ego_mesh_t *, const Uint32 itile );
ego_grid_info_t * ego_mesh_get_pgrid( const ego_mesh_t *, const Uint32 itile );
Uint8             ego_mesh_get_twist( ego_mesh_t * pmesh, const Uint32 igrid );

GRID_FX_BITS ego_grid_info_get_all_fx( const ego_grid_info_t * );
GRID_FX_BITS ego_grid_info_test_all_fx( const ego_grid_info_t *, const GRID_FX_BITS bits );
bool       ego_grid_info_add_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );
bool       ego_grid_info_sub_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );
bool       ego_grid_info_set_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );