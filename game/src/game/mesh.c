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

/// @file game/mesh.c
/// @brief Functions for creating, reading, and writing Egoboo's .mpd mesh file
/// @details

#include "egolib/_math.inl"
#include "egolib/bbox.inl"
#include "game/mesh.inl"
#include "game/mesh_functions.h"
#include "game/graphic.h"
#include "game/graphic_texture.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh initialization - not accessible by scripts
static void   ego_mesh_make_vrtstart( ego_mesh_t * pmesh );

static grid_mem_t *  grid_mem_ctor( grid_mem_t * pmem );
static grid_mem_t *  grid_mem_dtor( grid_mem_t * pmem );
static bool        grid_mem_alloc( grid_mem_t * pmem, const ego_mesh_info_t * pinfo );
static bool        grid_mem_free( grid_mem_t * pmem );
static void          grid_make_fanstart( grid_mem_t * pmesh, const ego_mesh_info_t * pinfo );

static tile_mem_t *    tile_mem_ctor( tile_mem_t * pmem );
static tile_mem_t *    tile_mem_dtor( tile_mem_t * pmem );
static bool          tile_mem_free( tile_mem_t * pmem );
static bool          tile_mem_alloc( tile_mem_t * pmem, const ego_mesh_info_t * pinfo );

static ego_mesh_info_t * ego_mesh_info_ctor( ego_mesh_info_t * pinfo );
static ego_mesh_info_t * ego_mesh_info_dtor( ego_mesh_info_t * pinfo );
static void             ego_mesh_info_init( ego_mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y );

static bool mpdfx_lists_alloc( mpdfx_lists_t * plst, const ego_mesh_info_t * pinfo );
static bool mpdfx_lists_dealloc( mpdfx_lists_t * plst );
static bool mpdfx_lists_reset( mpdfx_lists_t * plst );
static int    mpdfx_lists_push( mpdfx_lists_t * plst, GRID_FX_BITS fx_bits, size_t value );

// some twist/normal functions
static bool ego_mesh_make_normals( ego_mesh_t * pmesh );

static bool ego_mesh_convert( ego_mesh_t * pmesh_dst, map_t * pmesh_src );
static bool ego_mesh_make_bbox( ego_mesh_t * pmesh );

static float grid_get_mix( float u0, float u, float v0, float v );

static ego_mesh_t * ego_mesh_ctor_1( ego_mesh_t * pmesh, int tiles_x, int tiles_y );
static bool ego_mesh_remove_ambient( ego_mesh_t * pmesh );
static bool ego_mesh_recalc_twist( ego_mesh_t * pmesh );
static bool ego_mesh_make_texture( ego_mesh_t * pmesh );
static ego_mesh_t * ego_mesh_finalize( ego_mesh_t * pmesh );
static bool ego_mesh_test_one_corner( ego_mesh_t * pmesh, GLXvector3f pos, float * pdelta );
static bool ego_mesh_light_one_corner( const ego_mesh_t * pmesh, ego_tile_info_t * ptile, const bool reflective, const fvec3_base_t pos, fvec3_base_t nrm, float * plight );

static oglx_texture_t * ego_mesh_get_texture( Uint8 image, Uint8 size );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_t   mesh;

int mesh_mpdfx_tests = 0;
int mesh_bound_tests = 0;
int mesh_pressure_tests = 0;

fvec3_t   map_twist_nrm[256];
FACING_T  map_twist_facing_y[256];            // For surface normal of mesh
FACING_T  map_twist_facing_x[256];
fvec3_t   map_twist_vel[256];            // For sliding down steep hills
Uint8     map_twist_flat[256];

// variables to optimize calls to bind the textures
bool    mesh_tx_none   = false;
TX_REF    mesh_tx_image  = MESH_IMG_COUNT;
Uint8     mesh_tx_size   = 0xFF;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_info_t * ego_mesh_info_ctor( ego_mesh_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
void ego_mesh_info_init( ego_mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y )
{
    // set the desired number of tiles
    pinfo->tiles_x = tiles_x;
    pinfo->tiles_y = tiles_y;
    pinfo->tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // set the desired number of vertices
    if ( numvert < 0 )
    {
        numvert = MAP_FAN_VERTICES_MAX * pinfo->tiles_count;
    };
    pinfo->vertcount = numvert;
}

//--------------------------------------------------------------------------------------------
ego_mesh_info_t * ego_mesh_info_dtor( ego_mesh_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
tile_mem_t * tile_mem_ctor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
tile_mem_t * tile_mem_dtor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    tile_mem_free( pmem );

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * ego_mesh_get_texture( Uint8 image, Uint8 size )
{
    oglx_texture_t * tx_ptr = NULL;

    if ( 0 == size )
    {
        tx_ptr = gfx_get_mesh_tx_sml( image );
    }
    else if ( 1 == size )
    {
        tx_ptr = gfx_get_mesh_tx_big( image );
    }

    return tx_ptr;
}

//--------------------------------------------------------------------------------------------
void mesh_texture_invalidate()
{
    mesh_tx_image = MESH_IMG_COUNT;
    mesh_tx_size  = 0xFF;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * mesh_texture_bind( const ego_tile_info_t * ptile )
{
    Uint8  tx_image, tx_size;
    oglx_texture_t  * tx_ptr = NULL;
    bool needs_bind = false;

    // bind a NULL texture if we are in that mode
    if ( mesh_tx_none )
    {
        tx_ptr = NULL;
        needs_bind = true;

        mesh_texture_invalidate();
    }
    else if ( NULL == ptile )
    {
        tx_ptr = NULL;
        needs_bind = true;

        mesh_texture_invalidate();
    }
    else
    {
        tx_image = TILE_GET_LOWER_BITS( ptile->img );
        tx_size  = ( ptile->type < tile_dict.offset ) ? 0 : 1;

        if (( mesh_tx_image != tx_image ) || ( mesh_tx_size != tx_size ) )
        {
            tx_ptr = ego_mesh_get_texture( tx_image, tx_size );
            needs_bind = true;

            mesh_tx_image = tx_image;
            mesh_tx_size  = tx_size;
        }
    }

    if ( needs_bind )
    {
        oglx_texture_Bind( tx_ptr );
    }

    return tx_ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_ctor( ego_mesh_t * pmesh )
{
    /// @author BB
    /// @details initialize the ego_mesh_t structure

    if ( NULL != pmesh )
    {
        BLANK_STRUCT_PTR( pmesh )

        tile_mem_ctor( &( pmesh->tmem ) );
        grid_mem_ctor( &( pmesh->gmem ) );
        ego_mesh_info_ctor( &( pmesh->info ) );
        mpdfx_lists_ctor( &( pmesh->fxlists ) );
    }

    // global initialization
    ego_mesh_make_twist();

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_dtor( ego_mesh_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == tile_mem_dtor( &( pmesh->tmem ) ) ) return NULL;
    if ( NULL == grid_mem_dtor( &( pmesh->gmem ) ) ) return NULL;
    if ( NULL == ego_mesh_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_renew( ego_mesh_t * pmesh )
{
    pmesh = ego_mesh_dtor( pmesh );

    return ego_mesh_ctor( pmesh );
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_ctor_1( ego_mesh_t * pmesh, int tiles_x, int tiles_y )
{

    if ( NULL == pmesh ) return pmesh;

    BLANK_STRUCT_PTR( pmesh )

    // intitalize the mesh info using the max number of vertices for each tile
    ego_mesh_info_init( &( pmesh->info ), -1, tiles_x, tiles_y );

    // allocate the mesh memory
    tile_mem_alloc( &( pmesh->tmem ), &( pmesh->info ) );
    grid_mem_alloc( &( pmesh->gmem ), &( pmesh->info ) );
    mpdfx_lists_alloc( &( pmesh->fxlists ), &( pmesh->info ) );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_create( ego_mesh_t * pmesh, int tiles_x, int tiles_y )
{
    if ( NULL == pmesh )
    {
        pmesh = EGOBOO_NEW( ego_mesh_t );
        pmesh = ego_mesh_ctor( pmesh );
    }

    return ego_mesh_ctor_1( pmesh, tiles_x, tiles_y );
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_destroy( ego_mesh_t ** ppmesh )
{
    if ( NULL == ppmesh || NULL == *ppmesh ) return false;

    ego_mesh_dtor( *ppmesh );

    *ppmesh = NULL;

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_remove_ambient( ego_mesh_t * pmesh )
{
    /// @author BB
    /// @details remove extra ambient light in the lightmap
    Uint8 min_vrt_a = 255;

    if ( NULL == pmesh ) return false;

    for ( Uint32 cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        min_vrt_a = std::min( min_vrt_a, pmesh->gmem.grid_list[cnt].a );
    }

    for ( Uint32 cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        pmesh->gmem.grid_list[cnt].a = pmesh->gmem.grid_list[cnt].a - min_vrt_a;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_recalc_twist( ego_mesh_t * pmesh )
{
    ego_mesh_info_t * pinfo;
    tile_mem_t  * ptmem;
    grid_mem_t  * pgmem;

    if ( NULL == pmesh ) return false;
    pinfo = &( pmesh->info );
    ptmem  = &( pmesh->tmem );
    pgmem  = &( pmesh->gmem );

    // recalculate the twist
    for ( Uint32 fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        Uint8 twist = cartman_get_fan_twist( pmesh, fan );
        pgmem->grid_list[fan].twist = twist;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_set_texture( ego_mesh_t * pmesh, Uint32 tile, Uint16 image )
{
    ego_tile_info_t * ptile;
    Uint16 tile_value, tile_upper, tile_lower;

    if ( !ego_mesh_grid_is_valid( pmesh, tile ) ) return false;
    ptile = pmesh->tmem.tile_list + tile;

    // get the upper and lower bits for this tile image
    tile_value = ptile->img;
    tile_lower = image      & TILE_LOWER_MASK;
    tile_upper = tile_value & TILE_UPPER_MASK;

    // set the actual image
    ptile->img = tile_upper | tile_lower;

    // update the pre-computed texture info
    return ego_mesh_update_texture( pmesh, tile );
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_update_texture( ego_mesh_t * pmesh, Uint32 tile )
{
    size_t mesh_vrt;
    int    tile_vrt;
    Uint16 vertices;
    Uint16 image;
    Uint8  type;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mesh_info_t * pinfo;
    ego_tile_info_t * ptile;
    tile_definition_t * pdef;

    ptmem  = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );
    pinfo = &( pmesh->info );

    if ( !ego_mesh_grid_is_valid( pmesh, tile ) ) return false;
    ptile = ptmem->tile_list + tile;

    image = TILE_GET_LOWER_BITS( ptile->img );
    type  = ptile->type & 0x3F;

    pdef = TILE_DICT_PTR( tile_dict, type );
    if ( NULL == pdef ) return false;

    mesh_vrt = ptile->vrtstart;
    vertices = pdef->numvertices;
    for ( tile_vrt = 0; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
    {
        ptmem->tlst[mesh_vrt][SS] = pdef->u[tile_vrt];
        ptmem->tlst[mesh_vrt][TT] = pdef->v[tile_vrt];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_make_texture( ego_mesh_t * pmesh )
{
    Uint32 cnt;
    ego_mesh_info_t * pinfo;

    if ( NULL == pmesh ) return false;
    pinfo = &( pmesh->info );

    // set the texture coordinate for every vertex
    for ( cnt = 0; cnt < pinfo->tiles_count; cnt++ )
    {
        ego_mesh_update_texture( pmesh, cnt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_finalize( ego_mesh_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    ego_mesh_make_vrtstart( pmesh );
    ego_mesh_remove_ambient( pmesh );
    ego_mesh_recalc_twist( pmesh );
    ego_mesh_make_normals( pmesh );
    ego_mesh_make_bbox( pmesh );
    ego_mesh_make_texture( pmesh );

    // create some lists to make searching the mesh tiles easier
    mpdfx_lists_synch( &( pmesh->fxlists ), &( pmesh->gmem ), true );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_convert( ego_mesh_t * pmesh_dst, map_t * pmesh_src )
{
    Uint32 cnt;

    map_mem_t  * pmem_src;
    map_info_t * pinfo_src;

    tile_mem_t * ptmem_dst;
    grid_mem_t * pgmem_dst;
    mpdfx_lists_t * plists_dst;
    ego_mesh_info_t * pinfo_dst;
    bool allocated_dst;

    if ( NULL == pmesh_src ) return false;
    pmem_src = &( pmesh_src->mem );
    pinfo_src = &( pmesh_src->info );

    // clear out all data in the destination mesh
    if ( NULL == ego_mesh_renew( pmesh_dst ) ) return false;
    ptmem_dst  = &( pmesh_dst->tmem );
    pgmem_dst  = &( pmesh_dst->gmem );
    plists_dst = &( pmesh_dst->fxlists );
    pinfo_dst  = &( pmesh_dst->info );

    // set up the destination mesh from the source mesh
    ego_mesh_info_init( pinfo_dst, pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y );

    allocated_dst = tile_mem_alloc( ptmem_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    allocated_dst = grid_mem_alloc( pgmem_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    allocated_dst = mpdfx_lists_alloc( plists_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    // copy all the per-tile info
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        tile_info_t     * ptile_src = pmem_src->tile_list  + cnt;
        ego_tile_info_t * ptile_dst = ptmem_dst->tile_list + cnt;
        ego_grid_info_t * pgrid_dst = pgmem_dst->grid_list + cnt;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        // BLANK_STRUCT_PTR( ptile_dst )
        ptile_dst->type         = ptile_src->type;
        ptile_dst->img          = ptile_src->img;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        // BLANK_STRUCT_PTR( pgrid_dst )
        pgrid_dst->base_fx = ptile_src->fx;
        pgrid_dst->twist   = ptile_src->twist;

        // set the local fx flags
        pgrid_dst->wall_fx = pgrid_dst->base_fx;
        pgrid_dst->pass_fx = 0;

        // lcache is set in the constructor
        // nlst is set in the constructor
    }

    // copy all the per-vertex info
    for ( cnt = 0; cnt < pinfo_src->vertcount; cnt++ )
    {
        GLXvector3f     * ppos_dst = ptmem_dst->plst + cnt;
        GLXvector3f     * pcol_dst = ptmem_dst->clst + cnt;
        map_vertex_t    * pvrt_src = pmem_src->vlst + cnt;

        // copy all info from map_mem_t
        ( *ppos_dst )[XX] = pvrt_src->pos.x;
        ( *ppos_dst )[YY] = pvrt_src->pos.y;
        ( *ppos_dst )[ZZ] = pvrt_src->pos.z;

        // default color
        ( *pcol_dst )[RR] = ( *pcol_dst )[GG] = ( *pcol_dst )[BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        size_t vertex = ptmem_dst->tile_list[cnt].vrtstart;
        ego_grid_info_t * pgrid_dst = pgmem_dst->grid_list + cnt;
        map_vertex_t    * pvrt_src = pmem_src->vlst + vertex;

        pgrid_dst->a = pvrt_src->a;
        pgrid_dst->l = 0.0f;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_load( const char *modname, ego_mesh_t * pmesh )
{
    // trap bad module names
    if ( !VALID_CSTR( modname ) ) return pmesh;

    // initialize the mesh
    {
        // create a new mesh if we are passed a NULL pointer
        if ( NULL == pmesh )
        {
            pmesh = ego_mesh_ctor( pmesh );
        }

        if ( NULL == pmesh ) return pmesh;

        // free any memory that has been allocated
        pmesh = ego_mesh_renew( pmesh );
    }

    // actually do the loading
    {
        map_t  local_mpd, * pmpd;

        // load a raw mpd
        map_ctor( &local_mpd );
        tile_dictionary_load_vfs( "mp_data/fans.txt", &tile_dict, -1 );
        pmpd = map_load( vfs_resolveReadFilename( "mp_data/level.mpd" ), &local_mpd );

        // convert it into a convenient version for Egoboo
        if ( !ego_mesh_convert( pmesh, pmpd ) )
        {
            pmesh = NULL;
        }

        // delete the now useless mpd data
        map_dtor( &local_mpd );
    }

    // do some calculation to set up the mpd as a game mesh
    pmesh = ego_mesh_finalize( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_ctor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_dtor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    grid_mem_free( pmem );

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool grid_mem_alloc( grid_mem_t * pgmem, const ego_mesh_info_t * pinfo )
{

    if ( NULL == pgmem || NULL == pinfo || 0 == pinfo->vertcount ) return false;

    // free any memory already allocated
    if ( !grid_mem_free( pgmem ) ) return false;

    if ( pinfo->vertcount > MAP_VERTICES_MAX )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MAP_VERTICES_MAX );
        return false;
    }

    // set the desired blocknumber of grids
    pgmem->grids_x = pinfo->tiles_x;
    pgmem->grids_y = pinfo->tiles_y;
    pgmem->grid_count = pgmem->grids_x * pgmem->grids_y;

    // set the mesh edge info
    pgmem->edge_x = ( pgmem->grids_x + 1 ) * GRID_ISIZE;
    pgmem->edge_y = ( pgmem->grids_y + 1 ) * GRID_ISIZE;

    // set the desired blocknumber of blocks
    // this only works if BLOCK_BITS = GRID_BITS + 2
    pgmem->blocks_x = ( pinfo->tiles_x >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_x, 0x03 ) ) pgmem->blocks_x++;

    pgmem->blocks_y = ( pinfo->tiles_y >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_y, 0x03 ) ) pgmem->blocks_y++;

    pgmem->blocks_count = pgmem->blocks_x * pgmem->blocks_y;

    // allocate per-grid memory
    pgmem->grid_list  = ego_grid_info_create_ary( pinfo->tiles_count );
    if ( NULL == pgmem->grid_list ) goto grid_mem_alloc_fail;

    // helper info
    pgmem->blockstart = EGOBOO_NEW_ARY( Uint32, pgmem->blocks_y );
    if ( NULL == pgmem->blockstart ) goto grid_mem_alloc_fail;

    pgmem->tilestart  = EGOBOO_NEW_ARY( Uint32, pinfo->tiles_y );
    if ( NULL == pgmem->tilestart ) goto grid_mem_alloc_fail;

    // initialize the fanstart/blockstart data
    grid_make_fanstart( pgmem, pinfo );

    return true;

grid_mem_alloc_fail:

    grid_mem_free( pgmem );
    log_error( "grid_mem_alloc() - reduce the maximum number of vertices! (Check MAP_VERTICES_MAX)\n" );

    return false;
}

//--------------------------------------------------------------------------------------------
bool grid_mem_free( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return false;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->blockstart );
    EGOBOO_DELETE_ARY( pmem->tilestart );

    // deconstruct the grid list
    pmem->grid_list = ego_grid_info_destroy_ary( pmem->grid_list, pmem->grid_count );

    // reset some values to safe values
    BLANK_STRUCT_PTR( pmem )

    return true;
}

//--------------------------------------------------------------------------------------------
bool tile_mem_alloc( tile_mem_t * pmem, const ego_mesh_info_t * pinfo )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return false;

    // free any memory already allocated
    if ( !tile_mem_free( pmem ) ) return false;

    if ( pinfo->vertcount > MAP_VERTICES_MAX )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MAP_VERTICES_MAX );
        return false;
    }

    // allocate per-vertex memory
    pmem->plst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->plst ) goto mesh_mem_alloc_fail;

    pmem->tlst = EGOBOO_NEW_ARY( GLXvector2f, pinfo->vertcount );
    if ( NULL == pmem->tlst ) goto mesh_mem_alloc_fail;

    pmem->clst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->clst ) goto mesh_mem_alloc_fail;

    pmem->nlst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->nlst ) goto mesh_mem_alloc_fail;

    // allocate per-tile memory
    pmem->tile_list  = ego_tile_info_create_ary( pinfo->tiles_count );
    if ( NULL == pmem->tile_list ) goto mesh_mem_alloc_fail;

    pmem->vert_count = pinfo->vertcount;
    pmem->tile_count = pinfo->tiles_count;

    return true;

mesh_mem_alloc_fail:

    tile_mem_free( pmem );
    log_error( "tile_mem_alloc() - reduce the maximum number of vertices! (Check MAP_VERTICES_MAX)\n" );

    return false;
}

//--------------------------------------------------------------------------------------------
bool tile_mem_free( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return false;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->plst );
    EGOBOO_DELETE_ARY( pmem->nlst );
    EGOBOO_DELETE_ARY( pmem->clst );
    EGOBOO_DELETE_ARY( pmem->tlst );

    // per-tile values
    pmem->tile_list = ego_tile_info_destroy_ary( pmem->tile_list, pmem->tile_count );

    // reset some values to safe values
    pmem->vert_count  = 0;
    pmem->tile_count = 0;

    return true;
}

//--------------------------------------------------------------------------------------------
void grid_make_fanstart( grid_mem_t * pgmem, const ego_mesh_info_t * pinfo )
{
    /// @author ZZ
    /// @details This function builds a look up table to ease calculating the
    ///    fan number given an x,y pair

    int cnt;

    if ( NULL == pgmem || NULL == pinfo ) return;

    // do the tilestart
    for ( cnt = 0; cnt < pinfo->tiles_y; cnt++ )
    {
        pgmem->tilestart[cnt] = pinfo->tiles_x * cnt;
    }

    // calculate some of the block info
    if ( pgmem->blocks_x >= GRID_BLOCKY_MAX )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", pgmem->blocks_x, GRID_BLOCKY_MAX );
    }

    if ( pgmem->blocks_y >= GRID_BLOCKY_MAX )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", pgmem->blocks_y, GRID_BLOCKY_MAX );
    }

    // do the blockstart
    for ( cnt = 0; cnt < pgmem->blocks_y; cnt++ )
    {
        pgmem->blockstart[cnt] = pgmem->blocks_x * cnt;
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_make_vrtstart( ego_mesh_t * pmesh )
{
    size_t vert;
    Uint32 tile;

    ego_mesh_info_t * pinfo;
    tile_mem_t  * ptmem;
    tile_definition_t * pdef;

    if ( NULL == pmesh ) return;

    pinfo = &( pmesh->info );
    ptmem  = &( pmesh->tmem );

    vert = 0;
    for ( tile = 0; tile < pinfo->tiles_count; tile++ )
    {
        Uint8 ttype;

        ptmem->tile_list[tile].vrtstart = vert;

        ttype = ptmem->tile_list[tile].type;

        // throw away any remaining upper bits
        ttype &= 0x3F;

        pdef = TILE_DICT_PTR( tile_dict, ttype );
        if ( NULL == pdef ) continue;

        vert += pdef->numvertices;
    }

    if ( vert != pinfo->vertcount )
    {
        log_warning( "ego_mesh_make_vrtstart() - unexpected number of vertices %d of %d\n", vert, pinfo->vertcount );
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_make_twist()
{
    /// @author ZZ
    /// @details This function precomputes surface normals and steep hill acceleration for
    ///    the mesh

    int     cnt;
    float   gdot;
    fvec3_t grav = fvec3_t::zero;

    grav.z = gravity;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)
        fvec3_t   nrm;

        twist_to_normal( cnt, nrm.v, 1.0f );

        map_twist_nrm[cnt] = nrm;

        map_twist_facing_x[cnt] = ( FACING_T )( - vec_to_facing( nrm.z, nrm.y ) );
        map_twist_facing_y[cnt] = vec_to_facing( nrm.z, nrm.x );

        // this is about 5 degrees off of vertical
        map_twist_flat[cnt] = false;
        if ( nrm.z > 0.9945f )
        {
            map_twist_flat[cnt] = true;
        }

        // projection of the gravity parallel to the surface
        gdot = grav.z * nrm.z;

        gperp.x = gdot * nrm.x;
        gperp.y = gdot * nrm.y;
        gperp.z = gdot * nrm.z;

        gpara.x = grav.x - gperp.x;
        gpara.y = grav.y - gperp.y;
        gpara.z = grav.z - gperp.z;

        map_twist_vel[cnt] = gpara;
    }
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_make_bbox( ego_mesh_t * pmesh )
{
    /// @author BB
    /// @details set the bounding box for each tile, and for the entire mesh

    size_t mesh_vrt;
    int tile_vrt;
    Uint32 cnt;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mesh_info_t * pinfo;
    tile_definition_t * pdef;

    if ( NULL == pmesh ) return false;
    ptmem  = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );
    pinfo = &( pmesh->info );

    ptmem->bbox.mins[XX] = ptmem->bbox.maxs[XX] = ptmem->plst[0][XX];
    ptmem->bbox.mins[YY] = ptmem->bbox.maxs[YY] = ptmem->plst[0][YY];
    ptmem->bbox.mins[ZZ] = ptmem->bbox.maxs[ZZ] = ptmem->plst[0][ZZ];

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        oct_bb_t       * poct;
        ego_tile_info_t * ptile;
        Uint16 vertices;
        Uint8 type;
        oct_vec_t ovec;

        ptile = ptmem->tile_list + cnt;
        poct   = &( ptile->oct );

        type = ptile->type;
        type &= 0x3F;

        pdef = TILE_DICT_PTR( tile_dict, type );
        if ( NULL == pdef ) continue;

        mesh_vrt = ptmem->tile_list[cnt].vrtstart;    // Number of vertices
        vertices = pdef->numvertices;                 // Number of vertices

        // initialize the bounding box
        oct_vec_ctor( ovec, ptmem->plst[mesh_vrt] );
        oct_bb_set_ovec( poct, ovec );
        mesh_vrt++;

        // add the rest of the points into the bounding box
        for ( tile_vrt = 1; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
        {
            oct_vec_ctor( ovec, ptmem->plst[mesh_vrt] );
            oct_bb_self_sum_ovec( poct, ovec );
        }

        // ensure that NO tile has zero volume.
        // if a tile is declared to have all the same height, it will accidentally be called "empty".
        if ( poct->empty )
        {
            if ( ABS( poct->maxs[OCT_X] - poct->mins[OCT_X] ) +
                 ABS( poct->maxs[OCT_Y] - poct->mins[OCT_Y] ) +
                 ABS( poct->maxs[OCT_Z] - poct->mins[OCT_Z] ) > 0.0f )
            {
                ovec[OCT_X] = ovec[OCT_Y] = ovec[OCT_Z] = 1e-6;
                ovec[OCT_XY] = ovec[OCT_YX] = SQRT_TWO * ovec[OCT_X];
                oct_bb_self_grow( poct, ovec );
            }
        }

        // extend the mesh bounding box
        ptmem->bbox.mins[XX] = std::min( ptmem->bbox.mins[XX], poct->mins[XX] );
        ptmem->bbox.mins[YY] = std::min( ptmem->bbox.mins[YY], poct->mins[YY] );
        ptmem->bbox.mins[ZZ] = std::min( ptmem->bbox.mins[ZZ], poct->mins[ZZ] );

        ptmem->bbox.maxs[XX] = std::max( ptmem->bbox.maxs[XX], poct->maxs[XX] );
        ptmem->bbox.maxs[YY] = std::max( ptmem->bbox.maxs[YY], poct->maxs[YY] );
        ptmem->bbox.maxs[ZZ] = std::max( ptmem->bbox.maxs[ZZ], poct->maxs[ZZ] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_make_normals( ego_mesh_t * pmesh )
{
    /// @author BB
    /// @details this function calculates a set of normals for the 4 corners
    ///               of a given tile. It is supposed to generate smooth normals for
    ///               most tiles, but where there is a creas (i.e. between the floor and
    ///               a wall) the normals should not be smoothed.

    int ix, iy;
    Uint32 fan0, fan1;
    tile_mem_t * ptmem;
    grid_mem_t * pgmem;

    int     edge_is_crease[4];
    fvec3_t nrm_lst[4], vec_sum;
    float   weight_lst[4];

    // test for mesh
    if ( NULL == pmesh ) return false;
    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

    // set the default normal for each fan, based on the calculated twist value
    for ( fan0 = 0; fan0 < ptmem->tile_count; fan0++ )
    {
        Uint8 twist = pgmem->grid_list[fan0].twist;

        ptmem->nlst[fan0][XX] = map_twist_nrm[twist].x;
        ptmem->nlst[fan0][YY] = map_twist_nrm[twist].y;
        ptmem->nlst[fan0][ZZ] = map_twist_nrm[twist].z;
    }

    // find an "average" normal of each corner of the tile
    for ( iy = 0; iy < pmesh->info.tiles_y; iy++ )
    {
        for ( ix = 0; ix < pmesh->info.tiles_x; ix++ )
        {
            int ix_off[4] = {0, 1, 1, 0};
            int iy_off[4] = {0, 0, 1, 1};
            int i, j, k;

            fan0 = ego_mesh_get_tile_int( pmesh, ix, iy );
            if ( !ego_mesh_grid_is_valid( pmesh, fan0 ) ) continue;

            nrm_lst[0].x = ptmem->nlst[fan0][XX];
            nrm_lst[0].y = ptmem->nlst[fan0][YY];
            nrm_lst[0].z = ptmem->nlst[fan0][ZZ];

            // for each corner of this tile
            for ( i = 0; i < 4; i++ )
            {
                int dx, dy;
                int loc_ix_off[4];
                int loc_iy_off[4];

                // the offset list needs to be shifted depending on what i is
                j = ( 6 - i ) % 4;

                if ( 1 == ix_off[4-j] ) dx = -1; else dx = 0;
                if ( 1 == iy_off[4-j] ) dy = -1; else dy = 0;

                for ( k = 0; k < 4; k++ )
                {
                    loc_ix_off[k] = ix_off[( 4-j + k ) % 4 ] + dx;
                    loc_iy_off[k] = iy_off[( 4-j + k ) % 4 ] + dy;
                }

                // cache the normals
                // nrm_lst[0] is already known.
                for ( j = 1; j < 4; j++ )
                {
                    int jx, jy;

                    jx = ix + loc_ix_off[j];
                    jy = iy + loc_iy_off[j];

                    fan1 = ego_mesh_get_tile_int( pmesh, jx, jy );

                    if ( ego_mesh_grid_is_valid( pmesh, fan1 ) )
                    {
                        nrm_lst[j].x = ptmem->nlst[fan1][XX];
                        nrm_lst[j].y = ptmem->nlst[fan1][YY];
                        nrm_lst[j].z = ptmem->nlst[fan1][ZZ];

                        if ( nrm_lst[j].z < 0 )
                        {
                            nrm_lst[j].x *= -1.0f;
                            nrm_lst[j].y *= -1.0f;
                            nrm_lst[j].z *= -1.0f;
                        }
                    }
                    else
                    {
                        nrm_lst[j].x = 0;
                        nrm_lst[j].y = 0;
                        nrm_lst[j].z = 1;
                    }
                }

                // find the creases
                for ( j = 0; j < 4; j++ )
                {
                    float vdot;
                    int m = ( j + 1 ) % 4;

                    vdot = fvec3_dot_product( nrm_lst[j].v, nrm_lst[m].v );

                    edge_is_crease[j] = ( vdot < INV_SQRT_TWO );

                    weight_lst[j] = fvec3_dot_product( nrm_lst[j].v, nrm_lst[0].v );
                }

                weight_lst[0] = 1.0f;
                if ( edge_is_crease[0] )
                {
                    // this means that there is a crease between tile 0 and 1
                    weight_lst[1] = 0.0f;
                }

                if ( edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 3
                    weight_lst[3] = 0.0f;
                }

                if ( edge_is_crease[0] && edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 1
                    // and a crease between tile 0 and 3, isolating tile 2
                    weight_lst[2] = 0.0f;
                }

                vec_sum = nrm_lst[0];
                for ( j = 1; j < 4; j++ )
                {
                    if ( weight_lst[j] > 0.0f )
                    {
                        vec_sum.x += nrm_lst[j].x * weight_lst[j];
                        vec_sum.y += nrm_lst[j].y * weight_lst[j];
                        vec_sum.z += nrm_lst[j].z * weight_lst[j];
                    }
                }

                fvec3_self_normalize( vec_sum.v );

                ptmem->tile_list[fan0].ncache[i][XX] = vec_sum.x;
                ptmem->tile_list[fan0].ncache[i][YY] = vec_sum.y;
                ptmem->tile_list[fan0].ncache[i][ZZ] = vec_sum.z;
            }
        }
    }

    //            dy_min = iy_off[i] - 1;
    //            dy_max = iy_off[i];

    //            wt_cnt = 0;
    //            vec_sum.x = vec_sum.y = vec_sum.z = 0.0f;
    //            for (dy = dy_min; dy <= dy_max; dy++)
    //            {
    //                jy = iy + dy;
    //                for (dx = dx_min; dx <= dx_max; dx++)
    //                {
    //                    jx = ix + dx;

    //                    fan1 = ego_mesh_get_tile_int(pmesh, jx, jy);
    //                    if ( ego_mesh_grid_is_valid(pmesh, fan1) )
    //                    {
    //                        float wt;

    //                        vec1.x = ptmem->nlst[fan1][XX];
    //                        vec1.y = ptmem->nlst[fan1][YY];
    //                        vec1.z = ptmem->nlst[fan1][ZZ];
    //                        if ( vec1.z < 0 )
    //                        {
    //                            vec1.x *= -1.0f;
    //                            vec1.y *= -1.0f;
    //                            vec1.z *= -1.0f;
    //                        }

    //                        wt = fvec3_dot_product( vec0.v, vec1.v );
    //                        if ( wt > 0 )
    //                        {
    //                            vec_sum.x += wt * vec1.x;
    //                            vec_sum.y += wt * vec1.y;
    //                            vec_sum.z += wt * vec1.z;

    //                            wt_cnt += 1;
    //                        }
    //                    }
    //                }
    //            }

    //            if ( wt_cnt > 1 )
    //            {
    //                fvec3_self_normalize( vec_sum.v );

    //                ptmem->ncache[fan0][i][XX] = vec_sum.x;
    //                ptmem->ncache[fan0][i][YY] = vec_sum.y;
    //                ptmem->ncache[fan0][i][ZZ] = vec_sum.z;
    //            }
    //            else
    //            {
    //                ptmem->ncache[fan0][i][XX] = vec0.x;
    //                ptmem->ncache[fan0][i][YY] = vec0.y;
    //                ptmem->ncache[fan0][i][ZZ] = vec0.z;
    //            }
    //        }
    //    }
    //}

    return true;
}

//--------------------------------------------------------------------------------------------
bool grid_light_one_corner( const ego_mesh_t * pmesh, int fan, float height, float nrm[], float * plight )
{
    bool             reflective;
    lighting_cache_t * lighting  = NULL;
    ego_grid_info_t  * pgrid = NULL;

    // valid parameters?
    if ( NULL == pmesh || NULL == plight )
    {
        // not updated
        return false;
    }

    // valid grid?
    pgrid = ego_mesh_get_pgrid( pmesh, fan );
    if ( NULL == pgrid )
    {
        // not updated
        return false;
    }

    // max update speed is once per game frame
    if ( pgrid->cache_frame >= 0 && ( Uint32 )pgrid->cache_frame >= game_frame_all )
    {
        // not updated
        return false;
    }

    // get the grid lighting
    lighting = &( pgrid->cache );

    reflective = ( 0 != ego_grid_info_test_all_fx( pgrid, MAPFX_DRAWREF ) );

    // evaluate the grid lighting at this node
    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( lighting, nrm, height, pmesh->tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( lighting, nrm, height, pmesh->tmem.bbox, NULL, NULL );
    }

    // clip the light to a reasonable value
    ( *plight ) = CLIP(( *plight ), 0.0f, 255.0f );

    // update the cache frame
#if 0
    // figure out the correct way to cache *plight
    pgrid->cache_frame = game_frame_all;
#endif

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_test_one_corner( ego_mesh_t * pmesh, GLXvector3f pos, float * pdelta )
{
    float loc_delta, low_delta, hgh_delta;
    float hgh_wt, low_wt;

    if ( NULL == pdelta ) pdelta = &loc_delta;

    // interpolate the lighting for the given corner of the mesh
    *pdelta = grid_lighting_test( pmesh, pos, &low_delta, &hgh_delta );

    // determine the weighting
    hgh_wt = ( pos[ZZ] - pmesh->tmem.bbox.mins[kZ] ) / ( pmesh->tmem.bbox.maxs[kZ] - pmesh->tmem.bbox.mins[kZ] );
    hgh_wt = CLIP( hgh_wt, 0.0f, 1.0f );
    low_wt = 1.0f - hgh_wt;

    *pdelta = low_wt * low_delta + hgh_wt * hgh_delta;

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_light_one_corner( const ego_mesh_t * pmesh, ego_tile_info_t * ptile, const bool reflective, const fvec3_base_t pos, fvec3_base_t nrm, float * plight )
{
    lighting_cache_t grid_light;

    if ( NULL == pmesh || NULL == ptile ) return false;

    // interpolate the lighting for the given corner of the mesh
    grid_lighting_interpolate( pmesh, &grid_light, pos );

    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], pmesh->tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], pmesh->tmem.bbox, NULL, NULL );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_test_corners( ego_mesh_t * pmesh, ego_tile_info_t * ptile, float threshold )
{
    bool retval;
    int corner;

    tile_mem_t      * ptmem;
    light_cache_t   * lcache;
    light_cache_t   * d1_cache;

    // validate the parameters
    if ( NULL == pmesh || NULL == ptile ) return false;

    if ( threshold < 0.0f ) threshold = 0.0f;

    // get the normal and lighting cache for this tile
    ptmem    = &( pmesh->tmem );
    lcache   = &( ptile->lcache );
    d1_cache = &( ptile->d1_cache );

    retval = false;
    for ( corner = 0; corner < 4; corner++ )
    {
        float            delta;
        float          * pdelta;
        float          * plight;
        GLXvector3f    * ppos;

        pdelta = ( *d1_cache ) + corner;
        plight = ( *lcache ) + corner;
        ppos   = ptmem->plst + ptile->vrtstart + corner;

        ego_mesh_test_one_corner( pmesh, *ppos, &delta );

        if ( 0.0f == *plight )
        {
            delta = 10.0f;
        }
        else
        {
            delta /= *plight;
            delta = CLIP( delta, 0.0f, 10.0f );
        }

        *pdelta += delta;

        if ( *pdelta > threshold )
        {
            retval = true;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_light_corners( ego_mesh_t * pmesh, ego_tile_info_t * ptile, bool reflective, float mesh_lighting_keep )
{
    int corner;
    float max_delta;

    tile_mem_t      * ptmem;
    normal_cache_t  * ncache;
    light_cache_t   * lcache;
    light_cache_t   * d1_cache, * d2_cache;

    // check for valid pointers
    if ( NULL == pmesh || NULL == ptile )
    {
        return -1.0f;
    }

    // if no update is requested, return an "error value"
    if ( !ptile->request_lcache_update )
    {
        return -1.0f;
    };

    // has the lighting already been calculated this frame?
    if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame >= game_frame_all )
    {
        return -1.0f;
    }

    // get the normal and lighting cache for this tile
    ptmem    = &( pmesh->tmem );
    ncache   = &( ptile->ncache );
    lcache   = &( ptile->lcache );
    d1_cache = &( ptile->d1_cache );
    d2_cache = &( ptile->d2_cache );

    max_delta = 0.0f;
    for ( corner = 0; corner < 4; corner++ )
    {
        float light_new, light_old, delta, light_tmp;

        GLXvector3f    * pnrm;
        float          * plight;
        float          * pdelta1, * pdelta2;
        GLXvector3f    * ppos;

        pnrm    = ( *ncache ) + corner;
        plight  = ( *lcache ) + corner;
        pdelta1 = ( *d1_cache ) + corner;
        pdelta2 = ( *d2_cache ) + corner;
        ppos    = ptmem->plst + ptile->vrtstart + corner;

        light_new = 0.0f;
        ego_mesh_light_one_corner( pmesh, ptile, reflective, *ppos, *pnrm, &light_new );

        if ( *plight != light_new )
        {
            light_old = *plight;
            *plight = light_old * mesh_lighting_keep + light_new * ( 1.0f - mesh_lighting_keep );

            // measure the actual delta
            delta = ABS( light_old - *plight );

            // measure the relative change of the lighting
            light_tmp = 0.5f * ( ABS( *plight ) + ABS( light_old ) );
            if ( 0.0f == light_tmp )
            {
                delta = 10.0f;
            }
            else
            {
                delta /= light_tmp;
                delta = CLIP( delta, 0.0f, 10.0f );
            }

            // add in the actual change this update
            *pdelta2 += ABS( delta );

            // update the estimate to match the actual change
            *pdelta1 = *pdelta2;
        }

        max_delta = std::max( max_delta, *pdelta1 );
    }

    // un-mark the lcache
    ptile->request_lcache_update = false;
    ptile->lcache_frame        = game_frame_all;

    return max_delta;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_interpolate_vertex( tile_mem_t * pmem, ego_tile_info_t * ptile, float pos[], float * plight )
{
    int cnt;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    float u, v, wt, weight_sum;
    float loc_light;

    oct_bb_t        * poct;
    light_cache_t   * lc;

    // check the parameters
    if ( NULL == pmem || NULL == ptile ) return false;

    // handle optional parameters
    if ( NULL == plight ) plight = &loc_light;

    ( *plight ) = 0.0f;

    // alias some variables
    poct = &( ptile->oct );
    lc   = &( ptile->lcache );

    // determine a u,v coordinate for the vertex
    u = ( pos[XX] - poct->mins[OCT_X] ) / ( poct->maxs[OCT_X] - poct->mins[OCT_X] );
    v = ( pos[YY] - poct->mins[OCT_Y] ) / ( poct->maxs[OCT_Y] - poct->mins[OCT_Y] );

    // average the cached data on the 4 corners of the mesh
    weight_sum = 0.0f;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        wt = grid_get_mix( ix_off[cnt], u, iy_off[cnt], v );

        weight_sum += wt;
        ( *plight )  += wt * ( *lc )[cnt];
    }

    if (( *plight ) > 0.0f && weight_sum > 0.0f )
    {
        ( *plight ) /= weight_sum;
        ( *plight ) = CLIP(( *plight ), 0.0f, 255.0f );
    }
    else
    {
        ( *plight ) = 0.0f;
    }

    return true;
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float grid_get_mix( float u0, float u, float v0, float v )
{
    float wt_u, wt_v;
    float du = u - u0;
    float dv = v - v0;

    // du *= 1.0f;
    if ( ABS( du ) > 1.0f ) return 0.0f;
    wt_u = ( 1.0f - du ) * ( 1.0f + du );

    // dv *= 1.0f;
    if ( ABS( dv ) > 1.0f ) return 0.0f;
    wt_v = ( 1.0f - dv ) * ( 1.0f + dv );

    return wt_u * wt_v;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD ego_mesh_test_wall( const ego_mesh_t * pmesh, const float pos[], const float radius, const BIT_FIELD bits, mesh_wall_data_t * pdata )
{
    /// @author BB
    /// @details an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    mesh_wall_data_t loc_data;

    BIT_FIELD pass;
    int   ix, iy;
    float loc_radius;

    ego_irect_t bound;

    // deal with the optional parameters
    if ( NULL == pdata ) pdata = &loc_data;

    // if there is no interaction with the mesh, return 0
    if ( EMPTY_BIT_FIELD == bits ) return EMPTY_BIT_FIELD;

    // require a valid position
    if ( NULL == pos ) return EMPTY_BIT_FIELD;

    // if the mesh is empty, return 0
    if ( NULL == pmesh || 0 == pmesh->info.tiles_count || 0 == pmesh->tmem.tile_count ) return EMPTY_BIT_FIELD;
    pdata->pinfo = ( ego_mesh_info_t* ) & ( pmesh->info );
    pdata->tlist = pmesh->tmem.tile_list;
    pdata->glist = pmesh->gmem.grid_list;

    // make an alias for the radius
    loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = GRID_FSIZE * 0.5f;
    }

    // make sure it is positive
    loc_radius = ABS( loc_radius );

    pdata->fx_min = pos[kX] - loc_radius;
    pdata->fx_max = pos[kX] + loc_radius;

    pdata->fy_min = pos[kY] - loc_radius;
    pdata->fy_max = pos[kY] + loc_radius;

    // make a large limit in case the pos is so large that it cannot be represented by an int
    pdata->fx_min = std::max( pdata->fx_min, -9.0f * pmesh->gmem.edge_x );
    pdata->fx_max = std::min( pdata->fx_max, 10.0f * pmesh->gmem.edge_x );

    pdata->fy_min = std::max( pdata->fy_min, -9.0f * pmesh->gmem.edge_y );
    pdata->fy_max = std::min( pdata->fy_max, 10.0f * pmesh->gmem.edge_y );

    // find an integer bound.
    // we need to know about out of range values below clamp these to valid values
    bound.xmin = FLOOR( pdata->fx_min / GRID_FSIZE );
    bound.xmax = FLOOR( pdata->fx_max / GRID_FSIZE );
    bound.ymin = FLOOR( pdata->fy_min / GRID_FSIZE );
    bound.ymax = FLOOR( pdata->fy_max / GRID_FSIZE );

    // limit the test values to be in-bounds
    pdata->fx_min = std::max( pdata->fx_min, 0.0f );
    pdata->fx_max = std::min( pdata->fx_max, pmesh->gmem.edge_x );
    pdata->fy_min = std::max( pdata->fy_min, 0.0f );
    pdata->fy_max = std::min( pdata->fy_max, pmesh->gmem.edge_y );

    pdata->ix_min = std::max( bound.xmin, 0 );
    pdata->ix_max = std::min( bound.xmax, pmesh->info.tiles_x - 1 );
    pdata->iy_min = std::max( bound.ymin, 0 );
    pdata->iy_max = std::min( bound.ymax, pmesh->info.tiles_y - 1 );

    // clear the bit accumulator
    pass = 0;

    // detect out of bounds in the y-direction
    if ( bound.ymin < 0 || bound.ymax >= pdata->pinfo->tiles_y )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    // detect out of bounds in the x-direction
    if ( bound.xmin < 0 || bound.xmax >= pdata->pinfo->tiles_x )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        // since we KNOW that this is in range, allow raw access to the data strucutre
        int irow = pmesh->gmem.tilestart[iy];

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            int itile = ix + irow;

            // since we KNOW that this is in range, allow raw access to the data strucutre
            pass = ego_grid_info_test_all_fx( pdata->glist + itile, bits );
            if ( 0 != pass )
            {
                return pass;
            }

            mesh_mpdfx_tests++;
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_get_pressure( const ego_mesh_t * pmesh, const float pos[], float radius, const BIT_FIELD bits )
{
    const float tile_area = GRID_FSIZE * GRID_FSIZE;

    Uint32 itile;
    int   ix_min, ix_max, iy_min, iy_max;
    float fx_min, fx_max, fy_min, fy_max, obj_area;
    int ix, iy;

    float  loc_pressure, loc_radius;

    const ego_mesh_info_t  * pinfo;
    const ego_tile_info_t * tlist;
    const ego_grid_info_t * glist;

    // deal with the optional parameters
    loc_pressure = 0.0f;

    if ( NULL == pos || 0 == bits ) return 0;

    if ( NULL == pmesh || 0 == pmesh->info.tiles_count || 0 == pmesh->tmem.tile_count ) return 0;
    pinfo = &( pmesh->info );
    tlist = pmesh->tmem.tile_list;
    glist = pmesh->gmem.grid_list;

    // make an alias for the radius
    loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = GRID_FSIZE * 0.5f;
    }

    // make sure it is positive
    loc_radius = ABS( loc_radius );

    fx_min = pos[kX] - loc_radius;
    fx_max = pos[kX] + loc_radius;

    fy_min = pos[kY] - loc_radius;
    fy_max = pos[kY] + loc_radius;

    obj_area = ( fx_max - fx_min ) * ( fy_max - fy_min );

    ix_min = FLOOR( fx_min / GRID_FSIZE );
    ix_max = FLOOR( fx_max / GRID_FSIZE );

    iy_min = FLOOR( fy_min / GRID_FSIZE );
    iy_max = FLOOR( fy_max / GRID_FSIZE );

    for ( iy = iy_min; iy <= iy_max; iy++ )
    {
        float ty_min, ty_max;

        bool tile_valid = true;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pinfo->tiles_y )
        {
            tile_valid = false;
        }

        for ( ix = ix_min; ix <= ix_max; ix++ )
        {
            bool is_blocked = false;
            float tx_min, tx_max;

            float area_ratio;
            float ovl_x_min, ovl_x_max;
            float ovl_y_min, ovl_y_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pinfo->tiles_x )
            {
                tile_valid = false;
            }

            if ( tile_valid )
            {
                itile = ego_mesh_get_tile_int( pmesh, ix, iy );
                tile_valid = ego_mesh_grid_is_valid( pmesh, itile );
                if ( !tile_valid )
                {
                    is_blocked = true;
                }
                else
                {
                    is_blocked = ( 0 != ego_grid_info_test_all_fx( glist + itile, bits ) );
                }
            }

            if ( !tile_valid )
            {
                is_blocked = true;
            }

            if ( is_blocked )
            {
                // hiting the mesh
                float min_area;

                // determine the area overlap of the tile with the
                // object's bounding box
                ovl_x_min = std::max( fx_min, tx_min );
                ovl_x_max = std::min( fx_max, tx_max );

                ovl_y_min = std::max( fy_min, ty_min );
                ovl_y_max = std::min( fy_max, ty_max );

                min_area = std::min( tile_area, obj_area );

                area_ratio = 0.0f;
                if ( ovl_x_min <= ovl_x_max && ovl_y_min <= ovl_y_max )
                {
                    if ( 0.0f == min_area )
                    {
                        area_ratio = 1.0f;
                    }
                    else
                    {
                        area_ratio  = ( ovl_x_max - ovl_x_min ) * ( ovl_y_max - ovl_y_min ) / min_area;
                    }
                }

                loc_pressure += area_ratio;

                mesh_pressure_tests++;
            }
        }
    }

    return loc_pressure;
}

//--------------------------------------------------------------------------------------------
fvec2_t ego_mesh_get_diff( const ego_mesh_t * pmesh, const float pos[], float radius, float center_pressure, const BIT_FIELD bits )
{
    /// @author BB
    /// @details determine the shortest "way out", but creating an array of "pressures"
    /// with each element representing the pressure when the object is moved in different directions
    /// by 1/2 a tile.

    const float jitter_size = GRID_FSIZE * 0.5f;
    float pressure_ary[9];
    float fx, fy;
    fvec2_t diff = fvec2_t::zero;
    float   sum_diff = 0.0f;
    float   dpressure;

    int cnt;

    // find the pressure for the 9 points of jittering around the current position
    pressure_ary[4] = center_pressure;
    for ( cnt = 0, fy = pos[kY] - jitter_size; fy <= pos[kY] + jitter_size; fy += jitter_size )
    {
        for ( fx = pos[kX] - jitter_size; fx <= pos[kX] + jitter_size; fx += jitter_size, cnt++ )
        {
            fvec2_t jitter_pos;

            jitter_pos.x = fx;
            jitter_pos.y = fy;

            if ( 4 == cnt ) continue;

            pressure_ary[cnt] = ego_mesh_get_pressure( pmesh, jitter_pos.v, radius, bits );
        }
    }

    // determine the "minimum number of tiles to move" to get into a clear area
    diff.x = diff.y = 0.0f;
    sum_diff = 0.0f;
    for ( cnt = 0, fy = -0.5f; fy <= 0.5f; fy += 0.5f )
    {
        for ( fx = -0.5f; fx <= 0.5f; fx += 0.5f, cnt++ )
        {
            if ( 4 == cnt ) continue;

            dpressure = ( pressure_ary[cnt] - center_pressure );

            // find the maximal pressure gradient == the minimal distance to move
            if ( 0.0f != dpressure )
            {
                float   weight;
                fvec2_t tmp;
                float   dist = pressure_ary[4] / dpressure;

                tmp.x = dist * fx;
                tmp.y = dist * fy;

                weight = 1.0f / dist;

                diff.x += tmp.y * weight;
                diff.y += tmp.x * weight;
                sum_diff += ABS( weight );
            }
        }
    }
    // normalize the displacement by dividing by the weight...
    // unnecessary if the following normalization is kept in
    //if( sum_diff > 0.0f )
    //{
    //    diff.x /= sum_diff;
    //    diff.y /= sum_diff;
    //}

    // limit the maximum displacement to less than one tile
    if ( ABS( diff.x ) + ABS( diff.y ) > 0.0f )
    {
        float fmax = std::max( ABS( diff.x ), ABS( diff.y ) );

        diff.x /= fmax;
        diff.y /= fmax;
    }

    return diff;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD ego_mesh_hit_wall( const ego_mesh_t * pmesh, const float pos[], const float radius, const BIT_FIELD bits, float nrm[], float * pressure, mesh_wall_data_t * pdata )
{
    /// @author BB
    /// @details an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    BIT_FIELD loc_pass;
    Uint32 itile, pass;
    int ix, iy;
    bool invalid;

    float  loc_pressure;
    fvec3_base_t loc_nrm;

    bool needs_pressure = ( NULL != pressure );
    bool needs_nrm      = ( NULL != nrm );

    mesh_wall_data_t loc_data;

    // deal with the optional parameters
    if ( NULL == pressure ) pressure = &loc_pressure;
    *pressure = 0.0f;

    if ( NULL == nrm ) nrm = loc_nrm;
    nrm[kX] = nrm[kY] = 0.0f;

    // if pdata is not NULL, someone has already run a version of mesh_test_wall
    if ( NULL == pdata )
    {
        pdata = &loc_data;

        // Do the simplest test.
        // Initializes the shared mesh_wall_data_t struct, so no need to do it again
        // Eliminates all cases of bad source data, so no need to test them again.
        if ( 0 == ego_mesh_test_wall( pmesh, pos, radius, bits, pdata ) ) return 0;
    }

    // ego_mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

    pass = loc_pass = 0;
    nrm[kX] = nrm[kY] = 0.0f;
    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        float ty_min, ty_max;

        invalid = false;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pdata->pinfo->tiles_y )
        {
            loc_pass |= ( MAPFX_IMPASS | MAPFX_WALL );

            if ( needs_nrm )
            {
                nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
            }

            invalid = true;
            mesh_bound_tests++;
        }

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            float tx_min, tx_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pdata->pinfo->tiles_x )
            {
                loc_pass |=  MAPFX_IMPASS | MAPFX_WALL;

                if ( needs_nrm )
                {
                    nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                }

                invalid = true;
                mesh_bound_tests++;
            }

            if ( !invalid )
            {
                itile = ego_mesh_get_tile_int( pmesh, ix, iy );
                if ( ego_mesh_grid_is_valid( pmesh, itile ) )
                {
                    BIT_FIELD mpdfx   = ego_grid_info_get_all_fx( pdata->glist + itile );
                    bool is_blocked = HAS_SOME_BITS( mpdfx, bits );

                    if ( is_blocked )
                    {
                        SET_BIT( loc_pass,  mpdfx );

                        if ( needs_nrm )
                        {
                            nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                            nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
                        }
                    }
                }
            }
        }
    }

    pass = loc_pass & bits;

    if ( 0 == pass )
    {
        // if there is no impact at all, there is no normal and no pressure
        nrm[kX] = nrm[kY] = 0.0f;
        *pressure = 0.0f;
    }
    else
    {
        if ( needs_nrm )
        {
            // special cases happen a lot. try to avoid computing the square root
            if ( 0.0f == nrm[kX] && 0.0f == nrm[kY] )
            {
                // no normal does not mean no net pressure,
                // just that all the simplistic normal calculations balance
            }
            else if ( 0.0f == nrm[kX] )
            {
                nrm[kY] = SGN( nrm[kY] );
            }
            else if ( 0.0f == nrm[kY] )
            {
                nrm[kX] = SGN( nrm[kX] );
            }
            else
            {
                float dist = std::sqrt( nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY] );

                //*pressure = dist;
                nrm[kX] /= dist;
                nrm[kY] /= dist;
            }
        }

        if ( needs_pressure )
        {
            *pressure = ego_mesh_get_pressure( pmesh, pos, radius, bits );
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float ego_mesh_get_max_vertex_0( const ego_mesh_t * pmesh, int grid_x, int grid_y )
{
    Uint32 itile;
    ego_tile_info_t * ptile;
    int type;
    Uint32 cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    if ( NULL == pmesh ) return 0.0f;

    itile = ego_mesh_get_tile_int( pmesh, grid_x, grid_y );
    if ( INVALID_TILE == itile ) return 0.0f;

    // get a pointer to the tile
    ptile = pmesh->tmem.tile_list + itile;

    type   = ptile->type;
    vstart = ptile->vrtstart;
    vcount = std::min( (size_t)4, pmesh->tmem.vert_count );

    ivrt = vstart;
    zmax = pmesh->tmem.plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = std::max( zmax, pmesh->tmem.plst[ivrt][ZZ] );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_get_max_vertex_1( const ego_mesh_t * pmesh, int grid_x, int grid_y, float xmin, float ymin, float xmax, float ymax )
{
    Uint32 itile, cnt;
    int type;
    float zmax;
    size_t vcount, vstart, ivrt;

    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    if ( NULL == pmesh ) return 0.0f;

    itile = ego_mesh_get_tile_int( pmesh, grid_x, grid_y );

    if ( INVALID_TILE == itile ) return 0.0f;

    type   = pmesh->tmem.tile_list[itile].type;
    vstart = pmesh->tmem.tile_list[itile].vrtstart;
    vcount = std::min( (size_t)4, pmesh->tmem.vert_count );

    zmax = -1e6;
    for ( ivrt = vstart, cnt = 0; cnt < vcount; ivrt++, cnt++ )
    {
        float fx, fy;
        GLXvector3f * pvert = pmesh->tmem.plst + ivrt;

        // we are evaluating the height based on the grid, not the actual vertex positions
        fx = ( grid_x + ix_off[cnt] ) * GRID_FSIZE;
        fy = ( grid_y + iy_off[cnt] ) * GRID_FSIZE;

        if ( fx >= xmin && fx <= xmax && fy >= ymin && fy <= ymax )
        {
            zmax = std::max( zmax, ( *pvert )[ZZ] );
        }
    }

    if ( -1e6 == zmax ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
// ego_tile_info_t
//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_ctor( ego_tile_info_t * ptr, int index )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr )

    // set the non-zero, non-NULL, non-false values
    ptr->fanoff                = true;
    ptr->inrenderlist_frame    = -1;

    ptr->request_lcache_update   = true;
    ptr->lcache_frame         = -1;

    ptr->request_clst_update     = false;
    ptr->clst_frame            = -1;

    BSP_leaf_ctor( &( ptr->bsp_leaf ), ptr, BSP_LEAF_TILE, index );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_dtor( ego_tile_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr = ego_tile_info_free( ptr );

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_free( ego_tile_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    // delete any dynamically allocaed data

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_create( int index )
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW( ego_tile_info_t );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_ctor( retval, index );
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_destroy( ego_tile_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr = ego_tile_info_dtor( ptr );

    EGOBOO_DELETE( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_ctor_ary( ego_tile_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_tile_info_ctor( ptr + cnt, cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_dtor_ary( ego_tile_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_tile_info_dtor( ptr + cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_create_ary( size_t count )
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW_ARY( ego_tile_info_t, count );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_ctor_ary( retval, count );
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_destroy_ary( ego_tile_info_t * ary, size_t count )
{
    if ( NULL == ary ) return ary;

    ego_tile_info_ctor_ary( ary, count );

    EGOBOO_DELETE_ARY( ary );

    return ary;
}

//--------------------------------------------------------------------------------------------
// ego_grid_info_t
//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_ctor( ego_grid_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr )

    // set the non-zero, non-NULL, non-false values
    ptr->cache_frame = -1;
    ptr->twist       = TWIST_FLAT;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_dtor( ego_grid_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_grid_info_free( ptr );

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_free( ego_grid_info_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    // deallocate any dynamically allocated data
    // nothing to do yet

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_create()
{
    ego_grid_info_t * retval = NULL;

    retval = EGOBOO_NEW( ego_grid_info_t );
    if ( NULL == retval ) return NULL;

    return ego_grid_info_ctor( retval );
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_destroy( ego_grid_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr = ego_grid_info_dtor( ptr );

    EGOBOO_DELETE( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_ctor_ary( ego_grid_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_grid_info_ctor( ptr + cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_dtor_ary( ego_grid_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_grid_info_dtor( ptr + cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_create_ary( size_t count )
{
    ego_grid_info_t * retval = NULL;

    retval = EGOBOO_NEW_ARY( ego_grid_info_t, count );
    if ( NULL == retval ) return NULL;

    return ego_grid_info_ctor_ary( retval, count );
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t * ego_grid_info_destroy_ary( ego_grid_info_t * ary, size_t count )
{
    if ( NULL == ary ) return ary;

    ary = ego_grid_info_dtor_ary( ary, count );

    EGOBOO_DELETE_ARY( ary );

    return ary;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_update_water_level( ego_mesh_t * pmesh )
{
    // BB>
    // TODO: WHEN we begin using the map_BSP for frustum culling, we need to
    // update the bounding box height for every single water tile and then re-insert them in the mpd
    // AT THE MOMENT, this is not done and the increased bounding height due to water is handled elsewhere

    if ( NULL == pmesh ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t * mpdfx_list_ary_ctor( mpdfx_list_ary_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t * mpdfx_list_ary_dtor( mpdfx_list_ary_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    mpdfx_list_ary_dealloc( ptr );

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t * mpdfx_list_ary_alloc( mpdfx_list_ary_t * ptr, size_t count )
{
    if ( NULL == ptr ) return ptr;

    mpdfx_list_ary_dealloc( ptr );

    if ( 0 == count ) return ptr;

    ptr->lst = EGOBOO_NEW_ARY( size_t, count );
    ptr->cnt = ( NULL == ptr->lst ) ? 0 : count;
    ptr->idx = 0;

    return ptr;
}

//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t * mpdfx_list_ary_dealloc( mpdfx_list_ary_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    if ( 0 == ptr->cnt ) return ptr;

    EGOBOO_DELETE_ARY( ptr->lst );
    ptr->cnt = 0;
    ptr->idx = 0;

    return ptr;
}

//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t * mpdfx_list_ary_reset( mpdfx_list_ary_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->idx = 0;

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_list_ary_push( mpdfx_list_ary_t * ptr, size_t value )
{
    bool retval = false;

    if ( NULL == ptr ) return false;

    retval = false;
    if ( ptr->idx < ptr->cnt )
    {
        ptr->lst[ptr->idx] = value;
        ptr->idx++;
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpdfx_lists_t * mpdfx_lists_ctor( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return plst;

    BLANK_STRUCT_PTR( plst );

    mpdfx_list_ary_ctor( &( plst->sha ) );
    mpdfx_list_ary_ctor( &( plst->drf ) );
    mpdfx_list_ary_ctor( &( plst->anm ) );
    mpdfx_list_ary_ctor( &( plst->wat ) );
    mpdfx_list_ary_ctor( &( plst->wal ) );
    mpdfx_list_ary_ctor( &( plst->imp ) );
    mpdfx_list_ary_ctor( &( plst->dam ) );
    mpdfx_list_ary_ctor( &( plst->slp ) );

    return plst;
}

//--------------------------------------------------------------------------------------------
mpdfx_lists_t * mpdfx_lists_dtor( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return NULL;

    mpdfx_lists_dealloc( plst );

    mpdfx_list_ary_dtor( &( plst->sha ) );
    mpdfx_list_ary_dtor( &( plst->drf ) );
    mpdfx_list_ary_dtor( &( plst->anm ) );
    mpdfx_list_ary_dtor( &( plst->wat ) );
    mpdfx_list_ary_dtor( &( plst->wal ) );
    mpdfx_list_ary_dtor( &( plst->imp ) );
    mpdfx_list_ary_dtor( &( plst->dam ) );
    mpdfx_list_ary_dtor( &( plst->slp ) );

    BLANK_STRUCT_PTR( plst )

    return plst;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_alloc( mpdfx_lists_t * plst, const ego_mesh_info_t * pinfo )
{
    if ( NULL == plst || NULL == pinfo ) return false;

    // free any memory already allocated
    if ( !mpdfx_lists_dealloc( plst ) ) return false;

    if ( 0 == pinfo->tiles_count ) return true;

    mpdfx_list_ary_alloc( &( plst->sha ), pinfo->tiles_count );
    if ( NULL == plst->sha.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->drf ), pinfo->tiles_count );
    if ( NULL == plst->drf.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->anm ), pinfo->tiles_count );
    if ( NULL == plst->anm.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->wat ), pinfo->tiles_count );
    if ( NULL == plst->wat.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->wal ), pinfo->tiles_count );
    if ( NULL == plst->wal.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->imp ), pinfo->tiles_count );
    if ( NULL == plst->imp.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->dam ), pinfo->tiles_count );
    if ( NULL == plst->dam.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_alloc( &( plst->slp ), pinfo->tiles_count );
    if ( NULL == plst->slp.lst ) goto mesh_mem_alloc_fail;

    // the list needs to be resynched
    plst->dirty = true;

    return true;

mesh_mem_alloc_fail:

    mpdfx_lists_dealloc( plst );

    log_error( "%s - cannot allocate mpdfx_lists_t for this mesh!\n", __FUNCTION__ );

    return false;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_dealloc( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return false;

    // free the memory
    mpdfx_list_ary_dealloc( &( plst->sha ) );
    mpdfx_list_ary_dealloc( &( plst->drf ) );
    mpdfx_list_ary_dealloc( &( plst->anm ) );
    mpdfx_list_ary_dealloc( &( plst->wat ) );
    mpdfx_list_ary_dealloc( &( plst->wal ) );
    mpdfx_list_ary_dealloc( &( plst->imp ) );
    mpdfx_list_ary_dealloc( &( plst->dam ) );
    mpdfx_list_ary_dealloc( &( plst->slp ) );

    // no memory, so nothing can be stored and nothing can be dirty
    plst->dirty = false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_reset( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return false;

    // free the memory
    mpdfx_list_ary_reset( &( plst->sha ) );
    mpdfx_list_ary_reset( &( plst->drf ) );
    mpdfx_list_ary_reset( &( plst->anm ) );
    mpdfx_list_ary_reset( &( plst->wat ) );
    mpdfx_list_ary_reset( &( plst->wal ) );
    mpdfx_list_ary_reset( &( plst->imp ) );
    mpdfx_list_ary_reset( &( plst->dam ) );
    mpdfx_list_ary_reset( &( plst->slp ) );

    // everything has been reset. force it to recalculate
    plst->dirty = true;

    return true;
}

//--------------------------------------------------------------------------------------------
int mpdfx_lists_push( mpdfx_lists_t * plst, GRID_FX_BITS fx_bits, size_t value )
{
    int retval = 0;

    if ( NULL == plst ) return false;

    if ( 0 == fx_bits ) return true;

    if ( HAS_NO_BITS( fx_bits, MAPFX_SHA ) )
    {
        if ( mpdfx_list_ary_push( &( plst->sha ), value ) )
        {
            retval++;
        }

    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_DRAWREF ) )
    {
        if ( mpdfx_list_ary_push( &( plst->drf ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_ANIM ) )
    {
        if ( mpdfx_list_ary_push( &( plst->anm ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WATER ) )
    {
        if ( mpdfx_list_ary_push( &( plst->wat ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WALL ) )
    {
        if ( mpdfx_list_ary_push( &( plst->wal ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_IMPASS ) )
    {
        if ( mpdfx_list_ary_push( &( plst->imp ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_DAMAGE ) )
    {
        if ( mpdfx_list_ary_push( &( plst->dam ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_SLIPPY ) )
    {
        if ( mpdfx_list_ary_push( &( plst->slp ), value ) )
        {
            retval++;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_synch( mpdfx_lists_t * plst, const grid_mem_t * pgmem, bool force )
{
    size_t count, i;
    GRID_FX_BITS fx;

    if ( NULL == plst || NULL == pgmem ) return false;

    count =  pgmem->grid_count;
    if ( 0 == count ) return true;

    // don't re-calculate unless it is necessary
    if ( !force && !plst->dirty ) return true;

    // !!reset the counts!!
    mpdfx_lists_reset( plst );

    for ( i = 0; i < count; i++ )
    {
        fx = ego_grid_info_get_all_fx( pgmem->grid_list + i );

        mpdfx_lists_push( plst, fx, i );
    }

    // we're done calculating
    plst->dirty = false;

    return true;
}
