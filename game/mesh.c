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

/// @file mesh.c
/// @brief Functions for creating, reading, and writing Egoboo's .mpd mesh file
/// @details

#include "mesh.inl"

#include "log.h"
#include "graphic.h"

#include "egoboo_math.inl"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"
#include "egoboo.h"
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh initialization - not accessible by scripts
static void   mesh_init_tile_offset( ego_mpd_t * pmesh );
static void   mesh_make_vrtstart( ego_mpd_t * pmesh );

static grid_mem_t *  grid_mem_ctor( grid_mem_t * pmem );
static grid_mem_t *  grid_mem_dtor( grid_mem_t * pmem );
static bool_t        grid_mem_alloc( grid_mem_t * pmem, ego_mpd_info_t * pinfo );
static bool_t        grid_mem_free( grid_mem_t * pmem );
static void          grid_make_fanstart( grid_mem_t * pmesh, ego_mpd_info_t * pinfo );

static tile_mem_t *    tile_mem_ctor( tile_mem_t * pmem );
static tile_mem_t *    tile_mem_dtor( tile_mem_t * pmem );
static bool_t          tile_mem_free( tile_mem_t * pmem );
static bool_t          tile_mem_alloc( tile_mem_t * pmem, ego_mpd_info_t * pinfo );

static ego_mpd_info_t * mesh_info_ctor( ego_mpd_info_t * pinfo );
static ego_mpd_info_t * mesh_info_dtor( ego_mpd_info_t * pinfo );
static void             mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y );

// some twist/normal functions
static bool_t mesh_make_normals( ego_mpd_t * pmesh );

static bool_t mesh_convert( ego_mpd_t * pmesh_dst, mpd_t * pmesh_src );
static bool_t mesh_make_bbox( ego_mpd_t * pmesh );

static float grid_get_mix( float u0, float u, float v0, float v );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t   mesh;

int mesh_mpdfx_tests = 0;
int mesh_bound_tests = 0;
int mesh_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_ctor( ego_mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
void mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y )
{
    // set the desired number of tiles
    pinfo->tiles_x = tiles_x;
    pinfo->tiles_y = tiles_y;
    pinfo->tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // set the desired number of fertices
    if ( numvert < 0 )
    {
        numvert = MAXMESHVERTICES * pinfo->tiles_count;
    };
    pinfo->vertcount = numvert;
}

//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_dtor( ego_mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
tile_mem_t * tile_mem_ctor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
tile_mem_t * tile_mem_dtor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    tile_mem_free( pmem );

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_ctor( ego_mpd_t * pmesh )
{
    /// @details BB@> initialize the ego_mpd_t structure

    if ( NULL != pmesh )
    {
        memset( pmesh, 0, sizeof( *pmesh ) );

        mesh_init_tile_offset( pmesh );

        tile_mem_ctor( &( pmesh->tmem ) );
        grid_mem_ctor( &( pmesh->gmem ) );
        mesh_info_ctor( &( pmesh->info ) );
    }

    // global initialization
    mesh_make_twist();

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_dtor( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == tile_mem_dtor( &( pmesh->tmem ) ) ) return NULL;
    if ( NULL == grid_mem_dtor( &( pmesh->gmem ) ) ) return NULL;
    if ( NULL == mesh_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_renew( ego_mpd_t * pmesh )
{
    pmesh = mesh_dtor( pmesh );

    return mesh_ctor( pmesh );
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_ctor_1( ego_mpd_t * pmesh, int tiles_x, int tiles_y )
{

    if ( NULL == pmesh ) return pmesh;

    memset( pmesh, 0, sizeof( *pmesh ) );

    // intitalize the mesh info using the max number of vertices for each tile
    mesh_info_init( &( pmesh->info ), -1, tiles_x, tiles_y );

    // allocate the mesh memory
    tile_mem_alloc( &( pmesh->tmem ), &( pmesh->info ) );
    grid_mem_alloc( &( pmesh->gmem ), &( pmesh->info ) );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_create( ego_mpd_t * pmesh, int tiles_x, int tiles_y )
{
    if ( NULL == pmesh )
    {
        pmesh = EGOBOO_NEW( ego_mpd_t );
        pmesh = mesh_ctor( pmesh );
    }

    return mesh_ctor_1( pmesh, tiles_x, tiles_y );
}

//--------------------------------------------------------------------------------------------
bool_t mesh_destroy( ego_mpd_t ** ppmesh )
{
    if ( NULL == ppmesh || NULL == *ppmesh ) return bfalse;

    mesh_dtor( *ppmesh );

    *ppmesh = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void mesh_init_tile_offset( ego_mpd_t * pmesh )
{
    int cnt;

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff[cnt].x = (( cnt >> 0 ) & 7 ) / 8.0f;
        pmesh->tileoff[cnt].y = (( cnt >> 3 ) & 7 ) / 8.0f;
    }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_remove_ambient( ego_mpd_t * pmesh )
{
    /// @details BB@> remove extra ambient light in the lightmap

    Uint32 cnt;
    UFP8_T min_vrt_a = 255;

    if ( NULL == pmesh ) return bfalse;

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        min_vrt_a = MIN( min_vrt_a, pmesh->gmem.grid_list[cnt].a );
    }

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        pmesh->gmem.grid_list[cnt].a -= min_vrt_a;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_recalc_twist( ego_mpd_t * pmesh )
{
    Uint32 fan;
    ego_mpd_info_t * pinfo;
    tile_mem_t  * ptmem;
    grid_mem_t  * pgmem;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &( pmesh->info );
    ptmem  = &( pmesh->tmem );
    pgmem  = &( pmesh->gmem );

    // recalculate the twist
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        Uint8 twist = cartman_get_fan_twist( pmesh, fan );
        pgmem->grid_list[fan].twist = twist;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_set_texture( ego_mpd_t * pmesh, Uint16 tile, Uint16 image )
{
    ego_tile_info_t * ptile;
    Uint16 tile_value, tile_upper, tile_lower;

    if ( !mesh_grid_is_valid( pmesh, tile ) ) return bfalse;
    ptile = pmesh->tmem.tile_list + tile;

    // get the upper and lower bits for this tile image
    tile_value = pmesh->tmem.tile_list[tile].img;
    tile_lower = image      & TILE_LOWER_MASK;
    tile_upper = tile_value & TILE_UPPER_MASK;

    // set the actual image
    pmesh->tmem.tile_list[tile].img = tile_upper | tile_lower;

    // update the pre-computed texture info
    return mesh_update_texture( pmesh, tile );
}

//--------------------------------------------------------------------------------------------
bool_t mesh_update_texture( ego_mpd_t * pmesh, Uint32 tile )
{
    size_t mesh_vrt;
    int    tile_vrt;
    Uint16 vertices;
    float  offu, offv;
    Uint16 image;
    Uint8  type;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;
    ego_tile_info_t * ptile;

    ptmem  = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );
    pinfo = &( pmesh->info );

    if ( !mesh_grid_is_valid( pmesh, tile ) ) return bfalse;
    ptile = ptmem->tile_list + tile;

    image = TILE_GET_LOWER_BITS( ptile->img );
    type  = ptile->type & 0x3F;

    offu  = pmesh->tileoff[image].x;          // Texture offsets
    offv  = pmesh->tileoff[image].y;

    mesh_vrt = ptmem->tile_list[tile].vrtstart;    // Number of vertices
    vertices = tile_dict[type].numvertices;      // Number of vertices
    for ( tile_vrt = 0; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
    {
        ptmem->tlst[mesh_vrt][SS] = tile_dict[type].u[tile_vrt] + offu;
        ptmem->tlst[mesh_vrt][TT] = tile_dict[type].v[tile_vrt] + offv;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_make_texture( ego_mpd_t * pmesh )
{
    Uint32 cnt;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &( pmesh->info );

    // set the texture coordinate for every vertex
    for ( cnt = 0; cnt < pinfo->tiles_count; cnt++ )
    {
        mesh_update_texture( pmesh, cnt );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_finalize( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    mesh_make_vrtstart( pmesh );
    mesh_remove_ambient( pmesh );
    mesh_recalc_twist( pmesh );
    mesh_make_normals( pmesh );
    mesh_make_bbox( pmesh );
    mesh_make_texture( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_convert( ego_mpd_t * pmesh_dst, mpd_t * pmesh_src )
{
    Uint32 cnt;

    mpd_mem_t  * pmem_src;
    mpd_info_t * pinfo_src;

    tile_mem_t * ptmem_dst;
    grid_mem_t * pgmem_dst;
    ego_mpd_info_t * pinfo_dst;
    bool_t allocated_dst;

    if ( NULL == pmesh_src ) return bfalse;
    pmem_src = &( pmesh_src->mem );
    pinfo_src = &( pmesh_src->info );

    // clear out all data in the destination mesh
    if ( NULL == mesh_renew( pmesh_dst ) ) return bfalse;
    ptmem_dst = &( pmesh_dst->tmem );
    pgmem_dst = &( pmesh_dst->gmem );
    pinfo_dst = &( pmesh_dst->info );

    // set up the destination mesh from the source mesh
    mesh_info_init( pinfo_dst, pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y );

    allocated_dst = tile_mem_alloc( ptmem_dst, pinfo_dst );
    if ( !allocated_dst ) return bfalse;

    allocated_dst = grid_mem_alloc( pgmem_dst, pinfo_dst );
    if ( !allocated_dst ) return bfalse;

    // copy all the per-tile info
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        tile_info_t     * ptile_src = pmem_src->tile_list  + cnt;
        ego_tile_info_t * ptile_dst = ptmem_dst->tile_list + cnt;
        ego_grid_info_t * pgrid_dst = pgmem_dst->grid_list + cnt;

        memset( ptile_dst, 0, sizeof( *ptile_dst ) );
        ptile_dst->type         = ptile_src->type;
        ptile_dst->img          = ptile_src->img;

        memset( pgrid_dst, 0, sizeof( *pgrid_dst ) );
        pgrid_dst->fx    = ptile_src->fx;
        pgrid_dst->twist = ptile_src->twist;

        // lcache is set in a hepler function
        // nlst is set in a hepler function
    }

    // copy all the per-vertex info
    for ( cnt = 0; cnt < pinfo_src->vertcount; cnt++ )
    {
        // copy all info from mpd_mem_t
        ptmem_dst->plst[cnt][XX] = pmem_src->vlst[cnt].pos.x;
        ptmem_dst->plst[cnt][YY] = pmem_src->vlst[cnt].pos.y;
        ptmem_dst->plst[cnt][ZZ] = pmem_src->vlst[cnt].pos.z;

        // default color
        ptmem_dst->clst[cnt][XX] = ptmem_dst->clst[cnt][GG] = ptmem_dst->clst[cnt][BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        size_t vertex = ptmem_dst->tile_list[cnt].vrtstart;
        pgmem_dst->grid_list[cnt].a = pmem_src->vlst[vertex].a;
        pgmem_dst->grid_list[cnt].l = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_load( const char *modname, ego_mpd_t * pmesh )
{
    // trap bad module names
    if ( !VALID_CSTR( modname ) ) return pmesh;

    // initialize the mesh
    {
        // create a new mesh if we are passed a NULL pointer
        if ( NULL == pmesh )
        {
            pmesh = mesh_ctor( pmesh );
        }

        if ( NULL == pmesh ) return pmesh;

        // free any memory that has been allocated
        pmesh = mesh_renew( pmesh );
    }

    // actually do the loading
    {
        mpd_t  local_mpd, * pmpd;

        // load a raw mpd
        mpd_ctor( &local_mpd );
        tile_dictionary_load_vfs( "mp_data/fans.txt" , tile_dict, MAXMESHTYPE );
        pmpd = mpd_load( vfs_resolveReadFilename( "mp_data/level.mpd" ), &local_mpd );

        // convert it into a convenient version for Egoboo
        if ( !mesh_convert( pmesh, pmpd ) )
        {
            pmesh = NULL;
        }

        // delete the now useless mpd data
        mpd_dtor( &local_mpd );
    }

    // do some calculation to set up the mpd as a game mesh
    pmesh = mesh_finalize( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_ctor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_dtor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    grid_mem_free( pmem );

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_alloc( grid_mem_t * pgmem, ego_mpd_info_t * pinfo )
{

    if ( NULL == pgmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !grid_mem_free( pgmem ) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // set the desired blocknumber of grids
    pgmem->grids_x = pinfo->tiles_x;
    pgmem->grids_y = pinfo->tiles_y;
    pgmem->grid_count = pgmem->grids_x * pgmem->grids_y;

    // set the mesh edge info
    pgmem->edge_x = ( pgmem->grids_x + 1 ) << GRID_BITS;
    pgmem->edge_y = ( pgmem->grids_y + 1 ) << GRID_BITS;

    // set the desired blocknumber of blocks
    pgmem->blocks_x = ( pinfo->tiles_x >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_x, 0x03 ) ) pgmem->blocks_x++;

    pgmem->blocks_y = ( pinfo->tiles_y >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_y, 0x03 ) ) pgmem->blocks_y++;

    pgmem->blocks_count = pgmem->blocks_x * pgmem->blocks_y;

    // allocate per-grid memory
    pgmem->grid_list = EGOBOO_NEW_ARY( ego_grid_info_t, pgmem->grid_count );
    if ( NULL == pgmem->grid_list ) goto grid_mem_alloc_fail;

    // helper info
    pgmem->blockstart = EGOBOO_NEW_ARY( Uint32, pgmem->blocks_y );
    if ( NULL == pgmem->blockstart ) goto grid_mem_alloc_fail;

    pgmem->tilestart  = EGOBOO_NEW_ARY( Uint32, pinfo->tiles_y );
    if ( NULL == pgmem->tilestart ) goto grid_mem_alloc_fail;

    // initialize the fanstart/blockstart data
    grid_make_fanstart( pgmem, pinfo );

    return btrue;

grid_mem_alloc_fail:

    grid_mem_free( pgmem );
    log_error( "grid_mem_alloc() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_free( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->grid_list );
    EGOBOO_DELETE_ARY( pmem->blockstart );
    EGOBOO_DELETE_ARY( pmem->tilestart );

    // reset some values to safe values
    memset( pmem, 0, sizeof( *pmem ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t tile_mem_alloc( tile_mem_t * pmem, ego_mpd_info_t * pinfo )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !tile_mem_free( pmem ) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
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
    pmem->tile_list  = ego_tile_info_alloc_ary( pinfo->tiles_count );
    if ( NULL == pmem->tile_list ) goto mesh_mem_alloc_fail;

    pmem->vert_count = pinfo->vertcount;
    pmem->tile_count = pinfo->tiles_count;

    return btrue;

mesh_mem_alloc_fail:

    tile_mem_free( pmem );
    log_error( "tile_mem_alloc() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t tile_mem_free( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->plst );
    EGOBOO_DELETE_ARY( pmem->nlst );
    EGOBOO_DELETE_ARY( pmem->clst );
    EGOBOO_DELETE_ARY( pmem->tlst );

    // per-tile values
    EGOBOO_DELETE_ARY( pmem->tile_list );

    // reset some values to safe values
    pmem->vert_count  = 0;
    pmem->tile_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void grid_make_fanstart( grid_mem_t * pgmem, ego_mpd_info_t * pinfo )
{
    /// @details ZZ@> This function builds a look up table to ease calculating the
    ///    fan number given an x,y pair

    int cnt;

    if ( NULL == pgmem || NULL == pinfo ) return;

    // do the tilestart
    for ( cnt = 0; cnt < pinfo->tiles_y; cnt++ )
    {
        pgmem->tilestart[cnt] = pinfo->tiles_x * cnt;
    }

    // calculate some of the block info
    if ( pgmem->blocks_x >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", pgmem->blocks_x, MAXMESHBLOCKY );
    }

    if ( pgmem->blocks_y >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", pgmem->blocks_y, MAXMESHBLOCKY );
    }

    // do the blockstart
    for ( cnt = 0; cnt < pgmem->blocks_y; cnt++ )
    {
        pgmem->blockstart[cnt] = pgmem->blocks_x * cnt;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_make_vrtstart( ego_mpd_t * pmesh )
{
    size_t vert;
    Uint32 tile;

    ego_mpd_info_t * pinfo;
    tile_mem_t  * ptmem;

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

        vert += tile_dict[ttype].numvertices;
    }

    if ( vert != pinfo->vertcount )
    {
        log_warning( "mesh_make_vrtstart() - unexpected number of vertices %d of %d\n", vert, pinfo->vertcount );
    }
}

//--------------------------------------------------------------------------------------------
void mesh_make_twist()
{
    /// @details ZZ@> This function precomputes surface normals and steep hill acceleration for
    ///    the mesh

    Uint16 cnt;

    float     gdot;
    fvec3_t   grav = VECT3( 0, 0, gravity );

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)
        fvec3_t   nrm;

        twist_to_normal( cnt, nrm.v, 1.0f );

        map_twist_nrm[cnt] = nrm;

        map_twist_x[cnt] = ( Uint16 )( - vec_to_facing( nrm.z, nrm.y ) );
        map_twist_y[cnt] = vec_to_facing( nrm.z, nrm.x );

        // this is about 5 degrees off of vertical
        map_twist_flat[cnt] = bfalse;
        if ( nrm.z > 0.9945f )
        {
            map_twist_flat[cnt] = btrue;
        }

        // projection of the gravity parallel to the surface
        gdot = grav.z * nrm.z;

        gperp.x = gdot * nrm.x;
        gperp.y = gdot * nrm.y;
        gperp.z = gdot * nrm.z;

        gpara.x = grav.x - gperp.x;
        gpara.y = grav.y - gperp.y;
        gpara.z = grav.z - gperp.z;

        map_twistvel_x[cnt] = gpara.x;
        map_twistvel_y[cnt] = gpara.y;
        map_twistvel_z[cnt] = gpara.z;
    }
}

//--------------------------------------------------------------------------------------------
Uint8 cartman_get_fan_twist( const ego_mpd_t * pmesh, Uint32 tile )
{
    size_t vrtstart;
    float z0, z1, z2, z3;
    float zx, zy;

    // check for a valid tile
    if ( INVALID_TILE == tile  || tile > pmesh->info.tiles_count ) return TWIST_FLAT;

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->tmem.tile_list[tile].img ) return TWIST_FLAT;

    vrtstart = pmesh->tmem.tile_list[tile].vrtstart;

    z0 = pmesh->tmem.plst[vrtstart + 0][ZZ];
    z1 = pmesh->tmem.plst[vrtstart + 1][ZZ];
    z2 = pmesh->tmem.plst[vrtstart + 2][ZZ];
    z3 = pmesh->tmem.plst[vrtstart + 3][ZZ];

    zx = CARTMAN_FIXNUM * ( z0 + z3 - z1 - z2 ) / CARTMAN_SLOPE;
    zy = CARTMAN_FIXNUM * ( z2 + z3 - z0 - z1 ) / CARTMAN_SLOPE;

    return cartman_get_twist( zx, zy );
}

//--------------------------------------------------------------------------------------------
bool_t mesh_make_bbox( ego_mpd_t * pmesh )
{
    /// @details BB@> set the bounding box for each tile, and for the entire mesh

    size_t mesh_vrt;
    int tile_vrt;
    Uint32 cnt;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
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

        mesh_vrt = ptmem->tile_list[cnt].vrtstart;    // Number of vertices
        vertices = tile_dict[type].numvertices;          // Number of vertices

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
        ptmem->bbox.mins[XX] = MIN( ptmem->bbox.mins[XX], poct->mins[XX] );
        ptmem->bbox.mins[YY] = MIN( ptmem->bbox.mins[YY], poct->mins[YY] );
        ptmem->bbox.mins[ZZ] = MIN( ptmem->bbox.mins[ZZ], poct->mins[ZZ] );

        ptmem->bbox.maxs[XX] = MAX( ptmem->bbox.maxs[XX], poct->maxs[XX] );
        ptmem->bbox.maxs[YY] = MAX( ptmem->bbox.maxs[YY], poct->maxs[YY] );
        ptmem->bbox.maxs[ZZ] = MAX( ptmem->bbox.maxs[ZZ], poct->maxs[ZZ] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_make_normals( ego_mpd_t * pmesh )
{
    /// @details BB@> this function calculates a set of normals for the 4 corners
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
    if ( NULL == pmesh ) return bfalse;
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

            fan0 = mesh_get_tile_int( pmesh, ix, iy );
            if ( !mesh_grid_is_valid( pmesh, fan0 ) ) continue;

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

                    fan1 = mesh_get_tile_int( pmesh, jx, jy );

                    if ( mesh_grid_is_valid( pmesh, fan1 ) )
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
                    int k = ( j + 1 ) % 4;

                    vdot = fvec3_dot_product( nrm_lst[j].v, nrm_lst[k].v );

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

                vec_sum = fvec3_normalize( vec_sum.v );

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

    //                    fan1 = mesh_get_tile_int(pmesh, jx, jy);
    //                    if ( mesh_grid_is_valid(pmesh, fan1) )
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
    //                vec_sum = fvec3_normalize( vec_sum.v );

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

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t grid_light_one_corner( ego_mpd_t * pmesh, int fan, float height, float nrm[], float * plight )
{
    bool_t             reflective;
    lighting_cache_t * lighting;

    if ( NULL == pmesh || NULL == plight || !mesh_grid_is_valid( pmesh, fan ) ) return bfalse;

    // get the grid lighting
    lighting = &( pmesh->gmem.grid_list[fan].cache );

    reflective = ( 0 != mesh_test_fx( pmesh, fan, MPDFX_DRAWREF ) );

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
    ( *plight ) = CLIP(( *plight ), 0, 255 );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_test_one_corner( ego_mpd_t * pmesh, GLXvector3f pos, float * pdelta )
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

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_light_one_corner( ego_mpd_t * pmesh, int itile, GLXvector3f pos, GLXvector3f nrm, float * plight )
{
    lighting_cache_t grid_light;
    bool_t reflective;

    if ( !mesh_grid_is_valid( pmesh, itile ) ) return bfalse;

    // add in the effect of this lighting cache node
    reflective = ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) );

    // interpolate the lighting for the given corner of the mesh
    grid_lighting_interpolate( pmesh, &grid_light, pos[XX], pos[YY] );

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

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_test_corners( ego_mpd_t * pmesh, int itile, float threshold )
{
    bool_t retval;
    int corner;

    tile_mem_t    * ptmem;
    light_cache_t * lcache;
    light_cache_t * d1_cache;

    if ( NULL == pmesh || !mesh_grid_is_valid( pmesh, itile ) ) return bfalse;
    ptmem = &( pmesh->tmem );

    // get the normal and lighting cache for this tile
    lcache   = &( ptmem->tile_list[itile].lcache );
    d1_cache = &( ptmem->tile_list[itile].d1_cache );

    retval = bfalse;
    for ( corner = 0; corner < 4; corner++ )
    {
        float            delta;
        float          * pdelta;
        float          * plight;
        GLXvector3f    * ppos;

        pdelta = ( *d1_cache ) + corner;
        plight = ( *lcache ) + corner;
        ppos   = ptmem->plst + ptmem->tile_list[itile].vrtstart + corner;

        mesh_test_one_corner( pmesh, *ppos, &delta );

        if ( 0.0f == *plight )
        {
            delta = 10.0f;
        }
        else
        {
            delta /= *plight;
            delta = CLIP( delta, 0, 10.0f );
        }

        *pdelta += delta;

        if ( *pdelta > threshold ) retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
float mesh_light_corners( ego_mpd_t * pmesh, int itile, float mesh_lighting_keep )
{
    int corner;
    float max_delta;

    ego_mpd_info_t * pinfo;
    tile_mem_t     * ptmem;
    grid_mem_t     * pgmem;
    normal_cache_t * ncache;
    light_cache_t  * lcache;
    light_cache_t  * d1_cache, * d2_cache;

    if ( NULL == pmesh || !mesh_grid_is_valid( pmesh, itile ) ) return 0.0f;
    pinfo = &( pmesh->info );
    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

    // get the normal and lighting cache for this tile
    ncache   = &( ptmem->tile_list[itile].ncache );
    lcache   = &( ptmem->tile_list[itile].lcache );
    d1_cache = &( ptmem->tile_list[itile].d1_cache );
    d2_cache = &( ptmem->tile_list[itile].d2_cache );

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
        ppos    = ptmem->plst + ptmem->tile_list[itile].vrtstart + corner;

        light_new = 0.0f;
        mesh_light_one_corner( pmesh, itile, *ppos, *pnrm, &light_new );

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

        max_delta = MAX( max_delta, *pdelta1 );
    }

    return max_delta;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_interpolate_vertex( tile_mem_t * pmem, int fan, float pos[], float * plight )
{
    int cnt;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    float u, v, wt, weight_sum;

    oct_bb_t       * poct;
    light_cache_t  * lc;

    if ( NULL == plight ) return bfalse;
    ( *plight ) = 0.0f;

    if ( NULL == pmem ) return bfalse;

    poct = &( pmem->tile_list[fan].oct );
    lc   = &( pmem->tile_list[fan].lcache );

    // determine a u,v coordinate for the vertex
    u = ( pos[XX] - poct->mins[XX] ) / ( poct->maxs[XX] - poct->mins[XX] );
    v = ( pos[YY] - poct->mins[YY] ) / ( poct->maxs[YY] - poct->mins[YY] );

    // average the cached data on the 4 corners of the mesh
    weight_sum = 0.0f;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        wt = grid_get_mix( ix_off[cnt], u, iy_off[cnt], v );

        weight_sum += wt;
        ( *plight )  += wt * ( *lc )[cnt];
    }

    if (( *plight ) > 0 && weight_sum > 0.0f )
    {
        ( *plight ) /= weight_sum;
    }
    else
    {
        ( *plight ) = 0;
    }

    ( *plight ) = CLIP(( *plight ), 0, 255 );

    return btrue;
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
BIT_FIELD mesh_test_wall( const ego_mpd_t * pmesh, const float pos[], const float radius, const BIT_FIELD bits, mesh_wall_data_t * pdata )
{
    /// @details BB@> an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    mesh_wall_data_t loc_data;

    BIT_FIELD pass;
    int   ix, iy;
    float loc_radius;

    ego_irect_t bound;

    // deal with the optional parameters
    if ( NULL == pdata ) pdata = &loc_data;

    // if there is no interaction with the mesh, return 0
    if ( 0 == bits ) return 0;

    // require a valid position
    if ( NULL == pos ) return 0;

    // if the mesh is empty, return 0
    if ( NULL == pmesh || 0 == pmesh->info.tiles_count || 0 == pmesh->tmem.tile_count ) return 0;
    pdata->pinfo = ( ego_mpd_info_t* ) & ( pmesh->info );
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
    pdata->fx_min = MAX( pdata->fx_min, -9.0f * pmesh->gmem.edge_x );
    pdata->fx_max = MIN( pdata->fx_max, 10.0f * pmesh->gmem.edge_x );

    pdata->fy_min = MAX( pdata->fy_min, -9.0f * pmesh->gmem.edge_y );
    pdata->fy_max = MIN( pdata->fy_max, 10.0f * pmesh->gmem.edge_y );

    // find an integer bound.
    // we need to know about out of range values below clamp these to valid values
    bound.xmin = FLOOR( pdata->fx_min / GRID_FSIZE );
    bound.xmax = FLOOR( pdata->fx_max / GRID_FSIZE );
    bound.ymin = FLOOR( pdata->fy_min / GRID_FSIZE );
    bound.ymax = FLOOR( pdata->fy_max / GRID_FSIZE );

    // limit the test values to be in-bounds
    pdata->fx_min = MAX( pdata->fx_min, 0.0f );
    pdata->fx_max = MIN( pdata->fx_max, pmesh->gmem.edge_x );
    pdata->fy_min = MAX( pdata->fy_min, 0.0f );
    pdata->fy_max = MIN( pdata->fy_max, pmesh->gmem.edge_y );

    pdata->ix_min = MAX( bound.xmin, 0 );
    pdata->ix_max = MIN( bound.xmax, pmesh->info.tiles_x - 1 );
    pdata->iy_min = MAX( bound.ymin, 0 );
    pdata->iy_max = MIN( bound.ymax, pmesh->info.tiles_y - 1 );

    // clear the bit accumulator
    pass = 0;

    // detect out of bounds in the y-direction
    if ( bound.ymin < 0 || bound.ymax >= pdata->pinfo->tiles_y )
    {
        pass = ( MPDFX_IMPASS | MPDFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( 0 != pass ) return pass;

    // detect out of bounds in the x-direction
    if ( bound.xmin < 0 || bound.xmax >= pdata->pinfo->tiles_x )
    {
        pass = ( MPDFX_IMPASS | MPDFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( 0 != pass ) return pass;

    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        // since we KNOW that this is in range, allow raw access to the data strucutre
        int irow = pmesh->gmem.tilestart[iy];

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            int itile = ix + irow;

            // since we KNOW that this is in range, allow raw access to the data strucutre
            pass = pdata->glist[itile].fx & bits;
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
float mesh_get_pressure( const ego_mpd_t * pmesh, const float pos[], float radius, BIT_FIELD bits )
{
    const float tile_area = GRID_FSIZE * GRID_FSIZE;

    Uint32 itile;
    int   ix_min, ix_max, iy_min, iy_max;
    float fx_min, fx_max, fy_min, fy_max, obj_area;
    int ix, iy;

    float  loc_pressure, loc_radius;

    const ego_mpd_info_t  * pinfo;
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

        bool_t tile_valid = btrue;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pinfo->tiles_y )
        {
            tile_valid = bfalse;
        }

        for ( ix = ix_min; ix <= ix_max; ix++ )
        {
            bool_t is_blocked = bfalse;
            float tx_min, tx_max;

            float area_ratio;
            float ovl_x_min, ovl_x_max;
            float ovl_y_min, ovl_y_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pinfo->tiles_x )
            {
                tile_valid = bfalse;
            }

            if ( tile_valid )
            {
                itile = mesh_get_tile_int( pmesh, ix, iy );
                tile_valid = mesh_grid_is_valid( pmesh, itile );
                if ( !tile_valid )
                {
                    is_blocked = btrue;
                }
                else
                {
                    is_blocked = ( 0 != mesh_has_some_mpdfx( glist[itile].fx, bits ) );
                }
            }

            if ( !tile_valid )
            {
                is_blocked = btrue;
            }

            if ( is_blocked )
            {
                // hiting the mesh
                float min_area;

                // determine the area overlap of the tile with the
                // object's bounding box
                ovl_x_min = MAX( fx_min, tx_min );
                ovl_x_max = MIN( fx_max, tx_max );

                ovl_y_min = MAX( fy_min, ty_min );
                ovl_y_max = MIN( fy_max, ty_max );

                min_area = MIN( tile_area, obj_area );

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
fvec2_t mesh_get_diff( const ego_mpd_t * pmesh, const float pos[], float radius, float center_pressure, Uint32 bits )
{
    /// @details BB@> determine the shortest "way out", but creating an array of "pressures"
    /// with each element representing the pressure when the object is moved in different directions
    /// by 1/2 a tile.

    const float jitter_size = GRID_FSIZE * 0.5f;
    float pressure_ary[9];
    float fx, fy;
    fvec2_t diff = ZERO_VECT2;
    float   sum_diff = 0.0f;
    float   dpressure;

    int cnt;

    // find the pressure for the 9 points of jittering around the current position
    pressure_ary[4] = center_pressure;
    for ( cnt = 0, fy = pos[kY] - jitter_size; fy <= pos[kY] + jitter_size; fy += jitter_size )
    {
        for ( fx = pos[kX] - jitter_size; fx <= pos[kX] + jitter_size; fx += jitter_size, cnt++ )
        {
            fvec2_t jitter_pos = VECT2( fx, fy );

            if ( 4 == cnt ) continue;

            pressure_ary[cnt] = mesh_get_pressure( pmesh, jitter_pos.v, radius, bits );
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
                float weight;
                float   dist = pressure_ary[4] / dpressure;
                fvec2_t tmp = VECT2( dist * fx, dist * fy );

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
        float fmax = MAX( ABS( diff.x ), ABS( diff.y ) );

        diff.x /= fmax;
        diff.y /= fmax;
    }

    return diff;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mesh_hit_wall( const ego_mpd_t * pmesh, const float pos[], const float radius, const BIT_FIELD bits, float nrm[], float * pressure, mesh_wall_data_t * pdata )
{
    /// @details BB@> an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    BIT_FIELD loc_pass;
    Uint32 itile, pass;
    int ix, iy;
    bool_t invalid;

    float  loc_pressure;
    fvec3_base_t loc_nrm;

    bool_t needs_pressure = ( NULL != pressure );
    bool_t needs_nrm      = ( NULL != nrm );

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
        if ( 0 == mesh_test_wall( pmesh, pos, radius, bits, pdata ) ) return 0;
    }

    // mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

    pass = loc_pass = 0;
    nrm[kX] = nrm[kY] = 0.0f;
    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        float ty_min, ty_max;

        invalid = bfalse;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pdata->pinfo->tiles_y )
        {
            loc_pass |= ( MPDFX_IMPASS | MPDFX_WALL );

            if ( needs_nrm )
            {
                nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
            }

            invalid = btrue;
            mesh_bound_tests++;
        }

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            float tx_min, tx_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pdata->pinfo->tiles_x )
            {
                loc_pass |=  MPDFX_IMPASS | MPDFX_WALL;

                if ( needs_nrm )
                {
                    nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                }

                invalid = btrue;
                mesh_bound_tests++;
            }

            if ( !invalid )
            {
                itile = mesh_get_tile_int( pmesh, ix, iy );
                if ( mesh_grid_is_valid( pmesh, itile ) )
                {
                    BIT_FIELD mpdfx   = pdata->glist[itile].fx;
                    bool_t is_blocked = ( 0 != mesh_has_some_mpdfx( mpdfx, bits ) );

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
                float dist = SQRT( nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY] );

                //*pressure = dist;
                nrm[kX] /= dist;
                nrm[kY] /= dist;
            }
        }

        if ( needs_pressure )
        {
            *pressure = mesh_get_pressure( pmesh, pos, radius, bits );
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float mesh_get_max_vertex_0( const ego_mpd_t * pmesh, int grid_x, int grid_y )
{
    Uint32 itile;
    int type;
    Uint32 cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    if ( NULL == pmesh ) return 0.0f;

    itile = mesh_get_tile_int( pmesh, grid_x, grid_y );

    if ( INVALID_TILE == itile ) return 0.0f;

    type   = pmesh->tmem.tile_list[itile].type;
    vstart = pmesh->tmem.tile_list[itile].vrtstart;
    vcount = MIN( 4, pmesh->tmem.vert_count );

    ivrt = vstart;
    zmax = pmesh->tmem.plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = MAX( zmax, pmesh->tmem.plst[ivrt][ZZ] );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float mesh_get_max_vertex_1( const ego_mpd_t * pmesh, int grid_x, int grid_y, float xmin, float ymin, float xmax, float ymax )
{
    Uint32 itile, cnt;
    int type;
    float zmax;
    size_t vcount, vstart, ivrt;

    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    if ( NULL == pmesh ) return 0.0f;

    itile = mesh_get_tile_int( pmesh, grid_x, grid_y );

    if ( INVALID_TILE == itile ) return 0.0f;

    type   = pmesh->tmem.tile_list[itile].type;
    vstart = pmesh->tmem.tile_list[itile].vrtstart;
    vcount = MIN( 4, pmesh->tmem.vert_count );

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
            zmax = MAX( zmax, ( *pvert )[ZZ] );
        }
    }

    if ( -1e6 == zmax ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_init( ego_tile_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    // set the non-zero, non-NULL, non-bfalse values
    ptr->fanoff             = btrue;
    ptr->inrenderlist_frame = -1;
    ptr->needs_lighting_update = btrue;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_alloc()
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW( ego_tile_info_t );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_init( retval );
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_init_ary( ego_tile_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_tile_info_init( ptr + cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_alloc_ary( size_t count )
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW_ARY( ego_tile_info_t, count );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_init_ary( retval, count );
}
