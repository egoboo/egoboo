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

#include "mesh.h"
#include "log.h"

#include "egoboo_math.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh initialization - not accessible by scripts
static void   mesh_init_tile_offset( ego_mpd_t * pmesh );
static void   mesh_make_vrtstart( ego_mpd_t * pmesh );

static bool_t           mesh_free( ego_mpd_t * pmesh );

static mesh_mem_t *  mesh_mem_new( mesh_mem_t * pmem );
static mesh_mem_t *  mesh_mem_delete( mesh_mem_t * pmem );
static bool_t        mesh_mem_free( mesh_mem_t * pmem );
static bool_t        mesh_mem_allocate( mesh_mem_t * pmem, ego_mpd_info_t * pinfo );

static grid_mem_t *  grid_mem_new( grid_mem_t * pmem );
static grid_mem_t *  grid_mem_delete( grid_mem_t * pmem );
static bool_t        grid_mem_allocate( grid_mem_t * pmem, ego_mpd_info_t * pinfo );
static bool_t        grid_mem_free( grid_mem_t * pmem );
static void          grid_make_fanstart( grid_mem_t * pmesh, ego_mpd_info_t * pinfo );

static ego_mpd_info_t * mesh_info_new( ego_mpd_info_t * pinfo );
static ego_mpd_info_t * mesh_info_delete( ego_mpd_info_t * pinfo );
static void             mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  );

// some twist/normal functions
static bool_t mesh_make_normals( ego_mpd_t * pmesh );
static Uint8 cartman_get_fan_twist( ego_mpd_t * pmesh, Uint32 tile );

static bool_t mesh_convert( ego_mpd_t * pmesh_dst, mpd_t * pmesh_src );
static bool_t mesh_make_bbox( ego_mpd_t * pmesh );

static float grid_get_mix(float u0, float u, float v0, float v);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t            mesh;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_new( ego_mpd_info_t * pinfo )
{
    if (NULL == pinfo) return pinfo;

    memset( pinfo, 0, sizeof(ego_mpd_info_t) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
void mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  )
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

    // set the desired blocknumber of blocks
    pinfo->blocks_x = (pinfo->tiles_x >> 2);
    if ( HAS_SOME_BITS(pinfo->tiles_x, 0x03) ) pinfo->blocks_x++;

    pinfo->blocks_y = (pinfo->tiles_y >> 2);
    if ( HAS_SOME_BITS(pinfo->tiles_y, 0x03) ) pinfo->blocks_y++;

    pinfo->blocks_count = pinfo->blocks_x * pinfo->blocks_y;

    // set the mesh edge info
    pinfo->edge_x = pinfo->tiles_x << TILE_BITS;
    pinfo->edge_y = pinfo->tiles_y << TILE_BITS;
}

//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_delete( ego_mpd_info_t * pinfo )
{
    if ( NULL != pinfo )
    {
        memset( pinfo, 0, sizeof(ego_mpd_info_t) );
    }

    return pinfo;
}

//--------------------------------------------------------------------------------------------
mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem )
{
    if (NULL == pmem) return pmem;

    memset( pmem, 0, sizeof(mpd_mem_t) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem )
{
    if ( NULL != pmem )
    {
        mesh_mem_free( pmem );
        memset( pmem, 0, sizeof(mesh_mem_t) );
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_new( ego_mpd_t * pmesh )
{
    /// @details BB@> initialize the ego_mpd_t structure

    if ( NULL != pmesh )
    {
        memset( pmesh, 0, sizeof(ego_mpd_t) );

        mesh_init_tile_offset( pmesh );

        mesh_mem_new( &(pmesh->mmem) );
        grid_mem_new( &(pmesh->gmem) );
        mesh_info_new( &(pmesh->info) );
    }

    // global initialization
    mesh_make_twist();

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_delete( ego_mpd_t * pmesh )
{
    if ( NULL != pmesh )
    {
        mesh_mem_delete( &(pmesh->mmem) );
        grid_mem_delete( &(pmesh->gmem) );
        mesh_info_delete( &(pmesh->info)  );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_free( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return bfalse;

    mesh_mem_free( &(pmesh->mmem) );
    grid_mem_free( &(pmesh->gmem) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_renew( ego_mpd_t * pmesh )
{
    pmesh = mesh_delete( pmesh );

    return mesh_new( pmesh );
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_create( ego_mpd_t * pmesh, int tiles_x, int tiles_y )
{
    if (NULL == pmesh)
    {
        pmesh = mesh_new(pmesh);
    }

    if (NULL != pmesh)
    {
        // intitalize the mesh info using the max number of vertices for each tile
        mesh_info_init( &(pmesh->info), -1, tiles_x, tiles_y );

        // allocate the mesh memory
        mesh_mem_allocate( &(pmesh->mmem), &(pmesh->info) );
        grid_mem_allocate( &(pmesh->gmem), &(pmesh->info) );

        return pmesh;
    };

    return pmesh;
}

//--------------------------------------------------------------------------------------------
void mesh_init_tile_offset(ego_mpd_t * pmesh)
{
    int cnt;

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff[cnt].x = (( cnt >> 0 ) & 7) / 8.0f;
        pmesh->tileoff[cnt].y = (( cnt >> 3 ) & 7) / 8.0f;
    }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_remove_ambient( ego_mpd_t * pmesh )
{
    /// @details BB@> remove extra ambient light in the lightmap

    Uint32 cnt;
    Uint16 min_vrt_a = 255;

    if ( NULL == pmesh ) return bfalse;

    for (cnt = 0; cnt < pmesh->info.tiles_count; cnt++)
    {
        min_vrt_a = MIN(min_vrt_a, pmesh->gmem.light[cnt].a);
    }

    for (cnt = 0; cnt < pmesh->info.tiles_count; cnt++)
    {
        pmesh->gmem.light[cnt].a -= min_vrt_a;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_recalc_twist( ego_mpd_t * pmesh )
{
    Uint32 fan;
    ego_mpd_info_t * pinfo;
    mesh_mem_t  * pmem;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mmem);

    // recalculate the twist
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        Uint8 twist = cartman_get_fan_twist(pmesh, fan);
        pmem->tile_list[fan].twist = twist;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_set_texture( ego_mpd_t * pmesh, Uint16 tile, Uint16 image )
{
    tile_info_t * ptile;
    Uint16 tile_value, tile_upper, tile_lower;

    if ( !VALID_TILE(pmesh, tile) ) return bfalse;
    ptile = pmesh->mmem.tile_list + tile;

    // get the upper and lower bits for this tile image
    tile_value = pmesh->mmem.tile_list[tile].img;
    tile_lower = image      & TILE_LOWER_MASK;
    tile_upper = tile_value & TILE_UPPER_MASK;

    // set the actual image
    pmesh->mmem.tile_list[tile].img = tile_upper | tile_lower;

    // update the pre-computed texture info
    return mesh_update_texture( pmesh, tile );
}

//--------------------------------------------------------------------------------------------
bool_t mesh_update_texture( ego_mpd_t * pmesh, Uint16 tile )
{
    int    mesh_vrt, tile_vrt;
    Uint16 vertices;
    float  offu, offv;
    Uint16 image;
    Uint8  type;

    mesh_mem_t * pmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;
    tile_info_t * ptile;

    pmem  = &(pmesh->mmem);
    pgmem = &(pmesh->gmem);
    pinfo = &(pmesh->info);

    if ( !VALID_TILE(pmesh, tile) ) return bfalse;
    ptile = pmem->tile_list + tile;

    image = TILE_GET_LOWER_BITS( ptile->img );
    type  = ptile->type & 0x3F;

    offu  = pmesh->tileoff[image].x;          // Texture offsets
    offv  = pmesh->tileoff[image].y;

    mesh_vrt = pmem->tile_list[tile].vrtstart;    // Number of vertices
    vertices = tile_dict[type].numvertices;      // Number of vertices
    for ( tile_vrt = 0; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
    {
        pmem->tlst[mesh_vrt][SS] = tile_dict[type].u[tile_vrt] + offu;
        pmem->tlst[mesh_vrt][TT] = tile_dict[type].v[tile_vrt] + offv;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_make_texture( ego_mpd_t * pmesh )
{
    int cnt;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &(pmesh->info);

    // set the texture coordinate for every vertex
    for ( cnt = 0; cnt < pinfo->tiles_count; cnt++)
    {
        mesh_update_texture( pmesh, cnt );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_finalize( ego_mpd_t * pmesh )
{
    if (NULL == pmesh) return NULL;

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

    mesh_mem_t * pmem_dst;
    grid_mem_t * pgmem_dst;
    ego_mpd_info_t * pinfo_dst;
    bool_t allocated_dst;

    if ( NULL == pmesh_src ) return bfalse;
    pmem_src = &(pmesh_src->mem);
    pinfo_src = &(pmesh_src->info);

    // clear out all data in the destination mesh
    if ( NULL == mesh_renew( pmesh_dst ) ) return bfalse;
    pmem_dst  = &(pmesh_dst->mmem);
    pgmem_dst = &(pmesh_dst->gmem);
    pinfo_dst = &(pmesh_dst->info);

    // set up the destination mesh from the source mesh
    mesh_info_init( pinfo_dst, pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y );

    allocated_dst = mesh_mem_allocate( pmem_dst, pinfo_dst );
    if ( !allocated_dst) return bfalse;

    allocated_dst = grid_mem_allocate( pgmem_dst, pinfo_dst );
    if ( !allocated_dst) return bfalse;

    // copy all the per-tile info
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        memcpy( pmem_dst->tile_list + cnt, pmem_src->tile_list + cnt, sizeof(tile_info_t) );

        // lcache is set in a hepler function
        // nlst is set in a hepler function
    }

    // copy all the per-vertex info
    for ( cnt = 0; cnt < pinfo_src->vertcount; cnt++ )
    {
        // copy all info from mpd_mem_t
        pmem_dst->plst[cnt][XX] = pmem_src->vlst[cnt].pos.x;
        pmem_dst->plst[cnt][YY] = pmem_src->vlst[cnt].pos.y;
        pmem_dst->plst[cnt][ZZ] = pmem_src->vlst[cnt].pos.z;

        // default color
        pmem_dst->clst[cnt][XX] = pmem_dst->clst[cnt][GG] = pmem_dst->clst[cnt][BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        int vertex = pmem_dst->tile_list[cnt].vrtstart;
        pgmem_dst->light[cnt].a = pmem_src->vlst[vertex].a;
        pgmem_dst->light[cnt].l = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_load( const char *modname, ego_mpd_t * pmesh )
{
    // trap bad module names
    if ( !VALID_CSTR(modname) ) return pmesh;

    // initialize the mesh
    {
        // create a new mesh if we are passed a NULL pointer
        if (NULL == pmesh)
        {
            pmesh = mesh_new(pmesh);
        }
        if (NULL == pmesh) return pmesh;

        // free any memory that has been allocated
        pmesh = mesh_renew( pmesh );
    }

    // actually do the loading
    {
        STRING loadname;
        mpd_t  local_mpd, * pmpd;

        // try to load a mpd file
        make_newloadname( modname, "gamedat" SLASH_STR "level.mpd", loadname );

        // load a raw mpd
        mpd_new( &local_mpd );
        pmpd = mpd_load( vfs_resolveReadFilename(loadname), &local_mpd);

        // convert it into a convenient version for egoboo
        if ( !mesh_convert( pmesh, pmpd ) )
        {
            pmesh = NULL;
        }

        // delete the now useless mpd data
        mpd_delete( &local_mpd );
    }

    // do some calculation to set up the mpd as a game mesh
    pmesh = mesh_finalize( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block_int( ego_mpd_t * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->info.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->info.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->gmem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile_int( ego_mpd_t * pmesh, int tile_x,  int tile_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( tile_x < 0 || tile_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( tile_y < 0 || tile_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return tile_x + pmesh->gmem.tilestart[tile_y];
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_new( grid_mem_t * pmem )
{
    if (NULL == pmem) return pmem;

    memset( pmem, 0, sizeof(mpd_mem_t) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_delete( grid_mem_t * pmem )
{
    if ( NULL != pmem )
    {
        grid_mem_free( pmem );
        memset( pmem, 0, sizeof(grid_mem_t) );
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_allocate( grid_mem_t * pmem, ego_mpd_info_t * pinfo  )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !grid_mem_free(pmem) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate per-tile memory
    pmem->light  = (grid_lighting_t *) calloc( pinfo->tiles_count, sizeof(grid_lighting_t) );
    if ( NULL == pmem->light ) goto grid_mem_allocate_fail;

    // helper info
    pmem->blockstart = (Uint32*) calloc( pinfo->blocks_y, sizeof(Uint32) );
    if ( NULL == pmem->blockstart ) goto grid_mem_allocate_fail;

    pmem->tilestart  = (Uint32*) calloc( pinfo->tiles_y, sizeof(Uint32) );
    if ( NULL == pmem->tilestart ) goto grid_mem_allocate_fail;

    pmem->grid_count = pinfo->tiles_count;

    // initialize the fanstart/blockstart data
    grid_make_fanstart( pmem, pinfo );

    return btrue;

grid_mem_allocate_fail:

    grid_mem_free( pmem );
    log_error( "grid_mem_allocate() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_free( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->light != NULL )
    {
        free( pmem->light );
        pmem->light = NULL;
    }

    if ( pmem->blockstart != NULL )
    {
        free( pmem->blockstart );
        pmem->blockstart = NULL;
    }

    if ( pmem->tilestart != NULL )
    {
        free( pmem->tilestart );
        pmem->tilestart = NULL;
    }

    // reset some values to safe values
    pmem->grid_count  = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_allocate( mesh_mem_t * pmem, ego_mpd_info_t * pinfo  )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mesh_mem_free(pmem) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate per-vertex memory
    pmem->plst = ( GLXvector3f * ) calloc( pinfo->vertcount, sizeof(GLXvector3f) );
    if ( NULL == pmem->plst ) goto mesh_mem_allocate_fail;

    pmem->tlst = ( GLXvector2f * ) calloc( pinfo->vertcount, sizeof(GLXvector2f) );
    if ( NULL == pmem->tlst ) goto mesh_mem_allocate_fail;

    pmem->clst = ( GLXvector3f * ) calloc( pinfo->vertcount, sizeof(GLXvector3f) );
    if ( NULL == pmem->clst ) goto mesh_mem_allocate_fail;

    pmem->nlst = ( GLXvector3f * ) calloc( pinfo->vertcount, sizeof(GLXvector3f) );
    if ( NULL == pmem->nlst ) goto mesh_mem_allocate_fail;

    // allocate per-tile memory
    pmem->tile_list  = (tile_info_t *) calloc( pinfo->tiles_count, sizeof(tile_info_t) );
    if ( NULL == pmem->tile_list ) goto mesh_mem_allocate_fail;

    pmem->ncache = ( normal_cache_t * ) calloc( pinfo->tiles_count, sizeof(normal_cache_t) );
    if ( NULL == pmem->ncache ) goto mesh_mem_allocate_fail;

    pmem->lcache = ( light_cache_t * ) calloc( pinfo->tiles_count, sizeof(light_cache_t) );
    if ( NULL == pmem->lcache ) goto mesh_mem_allocate_fail;

    pmem->bb_list = ( aabb_t * ) calloc( pinfo->tiles_count, sizeof(aabb_t) );
    if ( NULL == pmem->bb_list ) goto mesh_mem_allocate_fail;

    pmem->vert_count = pinfo->vertcount;
    pmem->tile_count = pinfo->tiles_count;

    return btrue;

mesh_mem_allocate_fail:

    mesh_mem_free( pmem );
    log_error( "mesh_mem_allocate() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_free( mesh_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->plst != NULL )
    {
        free( pmem->plst );
        pmem->plst = NULL;
    }

    if ( pmem->nlst != NULL )
    {
        free( pmem->nlst );
        pmem->nlst = NULL;
    }

    if ( pmem->clst != NULL )
    {
        free( pmem->clst );
        pmem->clst = NULL;
    }

    if ( pmem->tlst != NULL )
    {
        free( pmem->tlst );
        pmem->tlst = NULL;
    }

    // per-tile values
    if ( NULL != pmem->tile_list )
    {
        free(pmem->tile_list);
        pmem->tile_list = NULL;
    }

    if ( NULL != pmem->bb_list )
    {
        free( pmem->bb_list );
        pmem->bb_list = NULL;
    }

    if ( NULL != pmem->ncache )
    {
        free( pmem->ncache );
        pmem->ncache = NULL;
    }

    if ( NULL != pmem->lcache )
    {
        free( pmem->lcache );
        pmem->lcache = NULL;
    }

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
    if ( pinfo->blocks_x >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", pinfo->blocks_x, MAXMESHBLOCKY );
    }

    if ( pinfo->blocks_y >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", pinfo->blocks_y, MAXMESHBLOCKY );
    }

    // do the blockstart
    for ( cnt = 0; cnt < pinfo->blocks_y; cnt++ )
    {
        pgmem->blockstart[cnt] = pinfo->blocks_x * cnt;
    }

}

//--------------------------------------------------------------------------------------------
void mesh_make_vrtstart( ego_mpd_t * pmesh )
{
    int vert;
    Uint32 tile;

    ego_mpd_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh) return;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mmem);

    vert = 0;
    for ( tile = 0; tile < pinfo->tiles_count; tile++ )
    {
        Uint8 ttype;

        pmem->tile_list[tile].vrtstart = vert;

        ttype = pmem->tile_list[tile].type;

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

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        GLfloat nrm[3];

        twist_to_normal(cnt, nrm, 1.0f);

        map_twist_nrm[cnt].x = nrm[XX];
        map_twist_nrm[cnt].y = nrm[YY];
        map_twist_nrm[cnt].z = nrm[ZZ];

        map_twist_x[cnt] = (Uint16) (- vec_to_facing( nrm[2], nrm[1] ));
        map_twist_y[cnt] = vec_to_facing( nrm[2], nrm[0] );

        // this is about 5 degrees off of vertical
        map_twist_flat[cnt] = bfalse;
        if ( nrm[2] > 0.9945f )
        {
            map_twist_flat[cnt] = btrue;
        }

        // projection of the gravity perpendicular to the surface
        map_twistvel_x[cnt] = nrm[0] * ABS(nrm[2]) * gravity;
        map_twistvel_y[cnt] = nrm[1] * ABS(nrm[2]) * gravity;
        map_twistvel_z[cnt] = nrm[2] * nrm[2] * gravity;
    }
}

//---------------------------------------------------------------------------------------------
float mesh_get_level( ego_mpd_t * pmesh, float x, float y )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precisely

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_tile( pmesh, x, y );
    if ( !VALID_TILE(pmesh, tile) ) return 0;

    ix = x;
    iy = y;

    ix &= TILE_MASK;
    iy &= TILE_MASK;

    z0 = pmesh->mmem.plst[ pmesh->mmem.tile_list[tile].vrtstart + 0 ][ZZ];
    z1 = pmesh->mmem.plst[ pmesh->mmem.tile_list[tile].vrtstart + 1 ][ZZ];
    z2 = pmesh->mmem.plst[ pmesh->mmem.tile_list[tile].vrtstart + 2 ][ZZ];
    z3 = pmesh->mmem.plst[ pmesh->mmem.tile_list[tile].vrtstart + 3 ][ZZ];

    zleft  = ( z0 * ( TILE_SIZE - iy ) + z3 * iy ) / TILE_SIZE;
    zright = ( z1 * ( TILE_SIZE - iy ) + z2 * iy ) / TILE_SIZE;
    zdone  = ( zleft * ( TILE_SIZE - ix ) + zright * ix ) / TILE_SIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x < pmesh->info.edge_x && pos_y >= 0.0f && pos_y < pmesh->info.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= BLOCK_BITS;
        iy >>= BLOCK_BITS;

        block = mesh_get_block_int(pmesh, ix, iy);
    }

    return block;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < pmesh->info.edge_x && pos_y >= 0.0f && pos_y < pmesh->info.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= TILE_BITS;
        iy >>= TILE_BITS;

        tile = mesh_get_tile_int( pmesh, ix, iy );
    }

    return tile;
}

//------------------------------------------------------------------------------
Uint8 cartman_get_fan_twist( ego_mpd_t * pmesh, Uint32 tile )
{
    int vrtstart;
    float z0, z1, z2, z3;
    float zx, zy;

    // check for a valid tile
    if ( INVALID_TILE == tile  || tile > pmesh->info.tiles_count ) return TWIST_FLAT;

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->mmem.tile_list[tile].img ) return TWIST_FLAT;

    vrtstart = pmesh->mmem.tile_list[tile].vrtstart;

    z0 = pmesh->mmem.plst[vrtstart + 0][ZZ];
    z1 = pmesh->mmem.plst[vrtstart + 1][ZZ];
    z2 = pmesh->mmem.plst[vrtstart + 2][ZZ];
    z3 = pmesh->mmem.plst[vrtstart + 3][ZZ];

    zx = CARTMAN_FIXNUM * (z0 + z3 - z1 - z2) / CARTMAN_SLOPE;
    zy = CARTMAN_FIXNUM * (z2 + z3 - z0 - z1) / CARTMAN_SLOPE;

    return cartman_get_twist( zx, zy );

}

//------------------------------------------------------------------------------
Uint32 mesh_test_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags )
{
    // test for mesh
    if ( NULL == pmesh ) return 0;

    // test for invalid tile
    if ( itile > pmesh->info.tiles_count )
    {
        return flags & (MPDFX_WALL | MPDFX_IMPASS);
    }

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->mmem.tile_list[itile].img )
    {
        return flags & (MPDFX_WALL | MPDFX_IMPASS);
    }

    return pmesh->mmem.tile_list[itile].fx & flags;
}

//------------------------------------------------------------------------------
bool_t mesh_make_bbox( ego_mpd_t * pmesh )
{
    /// @details BB@> set the bounding box for each tile, and for the entire mesh

    int mesh_vrt, tile_vrt;
    Uint32 cnt;

    mesh_mem_t * pmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
    pmem  = &(pmesh->mmem);
    pgmem = &(pmesh->gmem);
    pinfo = &(pmesh->info);

    pmem->bbox.mins[XX] = pmem->bbox.maxs[XX] = pmem->plst[0][XX];
    pmem->bbox.mins[YY] = pmem->bbox.maxs[YY] = pmem->plst[0][YY];
    pmem->bbox.mins[ZZ] = pmem->bbox.maxs[ZZ] = pmem->plst[0][ZZ];

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++)
    {
        aabb_t      * pbb;
        tile_info_t * ptile;
        Uint16 vertices;
        Uint8 type;

        pbb   = pmem->bb_list   + cnt;
        ptile = pmem->tile_list + cnt;

        type = ptile->type;
        type &= 0x3F;

        mesh_vrt = pmem->tile_list[cnt].vrtstart;    // Number of vertices
        vertices = tile_dict[type].numvertices;          // Number of vertices

        // set the bounding box for this tile
        pbb->mins[XX] = pbb->maxs[XX] = pmem->plst[mesh_vrt][XX];
        pbb->mins[YY] = pbb->maxs[YY] = pmem->plst[mesh_vrt][YY];
        pbb->mins[ZZ] = pbb->maxs[ZZ] = pmem->plst[mesh_vrt][ZZ];
        for ( tile_vrt = 1; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
        {
            pbb->mins[XX] = MIN(pbb->mins[XX], pmem->plst[mesh_vrt][XX]);
            pbb->mins[YY] = MIN(pbb->mins[YY], pmem->plst[mesh_vrt][YY]);
            pbb->mins[ZZ] = MIN(pbb->mins[ZZ], pmem->plst[mesh_vrt][ZZ]);

            pbb->maxs[XX] = MAX(pbb->maxs[XX], pmem->plst[mesh_vrt][XX]);
            pbb->maxs[YY] = MAX(pbb->maxs[YY], pmem->plst[mesh_vrt][YY]);
            pbb->maxs[ZZ] = MAX(pbb->maxs[ZZ], pmem->plst[mesh_vrt][ZZ]);
        }

        // extend the mesh bounding box
        pmem->bbox.mins[XX] = MIN(pmem->bbox.mins[XX], pbb->mins[XX]);
        pmem->bbox.mins[YY] = MIN(pmem->bbox.mins[YY], pbb->mins[YY]);
        pmem->bbox.mins[ZZ] = MIN(pmem->bbox.mins[ZZ], pbb->mins[ZZ]);

        pmem->bbox.maxs[XX] = MAX(pmem->bbox.maxs[XX], pbb->maxs[XX]);
        pmem->bbox.maxs[YY] = MAX(pmem->bbox.maxs[YY], pbb->maxs[YY]);
        pmem->bbox.maxs[ZZ] = MAX(pmem->bbox.maxs[ZZ], pbb->maxs[ZZ]);
    }

    return btrue;
}

//------------------------------------------------------------------------------
bool_t mesh_make_normals( ego_mpd_t * pmesh )
{
    int i, ix, iy, jx, jy;
    int dx, dy;
    Uint32 fan0, fan1;
    int wt_cnt;
    fvec3_t   vec0, vec1, vec_sum;
    mesh_mem_t * pmem;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;
    pmem = &(pmesh->mmem);

    // set the default normal for each fan, based on the calculated twist value
    for ( fan0 = 0; fan0 < pmem->tile_count; fan0++)
    {
        Uint8 twist = pmem->tile_list[fan0].twist;

        pmem->nlst[fan0][XX] = map_twist_nrm[twist].x;
        pmem->nlst[fan0][YY] = map_twist_nrm[twist].y;
        pmem->nlst[fan0][ZZ] = map_twist_nrm[twist].z;
    }

    // find an "average" normal of each corner of the tile
    for (iy = 0; iy < pmesh->info.tiles_y; iy++)
    {
        for (ix = 0; ix < pmesh->info.tiles_x; ix++)
        {
            int ivrt;

            fan0 = mesh_get_tile_int(pmesh, ix, iy);
            if ( !VALID_TILE(pmesh, fan0) ) continue;

            // the start of the 4 corner vertices for this tile
            ivrt = pmem->tile_list[fan0].vrtstart;

            vec0.x = pmem->nlst[fan0][XX];
            vec0.y = pmem->nlst[fan0][YY];
            vec0.z = pmem->nlst[fan0][ZZ];
            if ( vec0.z < 0 )
            {
                vec0.x *= -1.0f;
                vec0.y *= -1.0f;
                vec0.z *= -1.0f;
            }

            for ( i = 0; i < 4; i++)
            {
                int dx_min, dx_max, dy_min, dy_max;
                int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};

                dx_min = ix_off[i] - 1;
                dx_max = ix_off[i];

                dy_min = iy_off[i] - 1;
                dy_max = iy_off[i];

                wt_cnt = 0;
                vec_sum.x = vec_sum.y = vec_sum.z = 0.0f;
                for (dy = dy_min; dy <= dy_max; dy++)
                {
                    jy = iy + dy;
                    for (dx = dx_min; dx <= dx_max; dx++)
                    {
                        jx = ix + dx;

                        fan1 = mesh_get_tile_int(pmesh, jx, jy);
                        if ( VALID_TILE(pmesh, fan1) )
                        {
                            float wt;

                            vec1.x = pmem->nlst[fan1][XX];
                            vec1.y = pmem->nlst[fan1][YY];
                            vec1.z = pmem->nlst[fan1][ZZ];
                            if ( vec1.z < 0 )
                            {
                                vec1.x *= -1.0f;
                                vec1.y *= -1.0f;
                                vec1.z *= -1.0f;
                            }

                            wt = fvec3_dot_product( vec0.v, vec1.v );
                            if ( wt > 0 )
                            {
                                vec_sum.x += wt * vec1.x;
                                vec_sum.y += wt * vec1.y;
                                vec_sum.z += wt * vec1.z;

                                wt_cnt += 1;
                            }
                        }
                    }
                }

                if ( wt_cnt > 1 )
                {
                    vec_sum = fvec3_normalize( vec_sum.v );

                    pmem->ncache[fan0][i][XX] = vec_sum.x;
                    pmem->ncache[fan0][i][YY] = vec_sum.y;
                    pmem->ncache[fan0][i][ZZ] = vec_sum.z;
                }
                else
                {
                    pmem->ncache[fan0][i][XX] = vec0.x;
                    pmem->ncache[fan0][i][YY] = vec0.y;
                    pmem->ncache[fan0][i][ZZ] = vec0.z;
                }
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t grid_light_one_corner( ego_mpd_t * pmesh, int fan, float height, float nrm[], float * plight )
{
    bool_t             reflective;
    lighting_cache_t * lighting;

    if ( NULL == pmesh || NULL == plight || !VALID_TILE(pmesh, fan) ) return bfalse;

    // get the grid lighting
    lighting = &(pmesh->gmem.light[fan].cache);

    reflective = HAS_SOME_BITS( pmesh->mmem.tile_list[fan].fx, MPDFX_DRAWREF );

    // evaluate the grid lighting at this node
    if( reflective )
    {
        float light_dir, light_amb;

        evaluate_lighting_cache(lighting, nrm, height, pmesh->mmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        (*plight) = light_amb + 0.5f * light_dir;
    }
    else
    {
        (*plight) = evaluate_lighting_cache(lighting, nrm, height, pmesh->mmem.bbox, NULL, NULL );
    }

    // clip the light to a reasonable value
    (*plight) = CLIP((*plight), 0, 255);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_light_one_corner( ego_mpd_t * pmesh, GLXvector3f pos, GLXvector3f nrm, float * plight )
{
    int   ix, iy, tnc, fan1;
    float u, v;
    float wt, wt_sum;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};

    ego_mpd_info_t * pinfo;
    mesh_mem_t     * pmem;
    grid_mem_t     * pgmem;

    if (NULL == pmesh || NULL == plight) return bfalse;
    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mmem);
    pgmem = &(pmesh->gmem);

    if ( pos[XX] < 0 || pos[XX] > pinfo->edge_x ) return bfalse;
    if ( pos[YY] < 0 || pos[YY] > pinfo->edge_y ) return bfalse;

    ix = ((int)pos[XX]) >> TILE_BITS;
    iy = ((int)pos[YY]) >> TILE_BITS;

    u = ( pos[XX] - (ix << TILE_BITS) ) / TILE_SIZE;
    v = ( pos[YY] - (iy << TILE_BITS) ) / TILE_SIZE;

    wt_sum = 0;
    for ( tnc = 0; tnc < 4; tnc++ )
    {
        int   jx, jy;
        lighting_cache_t * lighting;

        // what are the new vertices?
        jx = ix + ix_off[tnc];
        jy = iy + iy_off[tnc];

        // which fan does this corner belong to?
        fan1 = mesh_get_tile_int( pmesh, jx, jy );
        if ( !VALID_TILE(pmesh, fan1) ) continue;

        // get the lighting cache for this corner
        lighting = &(pgmem->light[fan1].cache);

        // get the weight for this corner
        wt = grid_get_mix(ix_off[tnc], u, iy_off[tnc], v);

        // add in the effect of this lighting cache node
        if ( 0.0f != wt )
        {
            bool_t reflective = HAS_SOME_BITS( pmesh->mmem.tile_list[fan1].fx, MPDFX_DRAWREF );

            if( reflective )
            {
                float light_dir, light_amb;

                evaluate_lighting_cache(lighting, nrm, pos[ZZ], pmesh->mmem.bbox, &light_amb, &light_dir );

                // make ambient light only illuminate 1/2
                (*plight) += wt * (light_amb + 0.5f * light_dir);
            }
            else
            {
                (*plight) += wt * evaluate_lighting_cache(lighting, nrm, pos[ZZ], pmesh->mmem.bbox, NULL, NULL );
            }

            wt_sum    += wt;
        }
    }



    // normalize the lighting value, if needed
    if ( (*plight) > 0.0f && wt_sum > 0.0f )
    {
        (*plight) /= wt_sum;
    }
    else
    {
        (*plight) = 0.0f;
    }

    // clip the lighting value to a reasonable size
    (*plight) = CLIP((*plight), 0, 255);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_light_corners( ego_mpd_t * pmesh, int fan1 )
{
    int corner;

    ego_mpd_info_t * pinfo;
    mesh_mem_t     * pmem;
    grid_mem_t     * pgmem;
    normal_cache_t * ncache;
    light_cache_t  * lcache;

    if ( NULL == pmesh || !VALID_TILE(pmesh, fan1) ) return bfalse;
    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mmem);
    pgmem = &(pmesh->gmem);

    // get the normal and lighting cache for this tile
    ncache = pmem->ncache + fan1;
    lcache = pmem->lcache + fan1;

    for ( corner = 0; corner < 4; corner++ )
    {
        float light;

        GLXvector3f    * pnrm;
        Uint8          * plight;
        GLXvector3f    * ppos;

        pnrm   = (*ncache) + corner;
        plight = (*lcache) + corner;
        ppos   = pmem->plst + pmem->tile_list[fan1].vrtstart + corner;

        light = 0.0f;
        mesh_light_one_corner( pmesh, *ppos, *pnrm, &light );

        (*plight) = (*plight) * 0.9f + light * 0.1f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_interpolate_vertex( mesh_mem_t * pmem, int fan, float pos[], float * plight )
{
    int cnt;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    float u, v, wt, weight_sum;

    aabb_t         * bb;
    light_cache_t  * lc;

    if ( NULL == plight ) return bfalse;
    (*plight) = 0.0f;

    if ( NULL == pmem ) return bfalse;

    bb = pmem->bb_list + fan;
    lc = pmem->lcache  + fan;

    // determine a u,v coordinate for the vertex
    u = (pos[XX] - bb->mins[XX]) / (bb->maxs[XX] - bb->mins[XX]);
    v = (pos[YY] - bb->mins[YY]) / (bb->maxs[YY] - bb->mins[YY]);

    // average the cached data on the 4 corners of the mesh
    weight_sum = 0.0f;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        wt = grid_get_mix(ix_off[cnt], u, iy_off[cnt], v);

        weight_sum += wt;
        (*plight)  += wt * (*lc)[cnt];
    }

    if ( (*plight) > 0 && weight_sum > 0.0 )
    {
        (*plight) /= weight_sum;
    }
    else
    {
        (*plight) = 0;
    }

    (*plight) = CLIP((*plight), 0, 255);

    return btrue;
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float grid_get_mix(float u0, float u, float v0, float v)
{
    float wt_u, wt_v;
    float du = u - u0;
    float dv = v - v0;

    // du *= 1.0f;
    if ( ABS(du) > 1.0 ) return 0;
    wt_u = (1.0f - du) * (1.0f + du);

    // dv *= 1.0f;
    if ( ABS(dv) > 1.0 ) return 0;
    wt_v = (1.0f - dv) * (1.0f + dv);

    return wt_u * wt_v;
}

//--------------------------------------------------------------------------------------------
float evaluate_lighting_cache( lighting_cache_t * src, GLfloat nrm[], float z, aabb_t bbox, float * light_amb, float * light_dir )
{
    float lighting;
    float hgh_wt, low_wt;

    if ( NULL == src || NULL == nrm ) return 0.0f;

    if ( src->max_light <= 0.0f ) return 0.0f;

    hgh_wt = (z - bbox.mins[ZZ]) / (bbox.maxs[ZZ] - bbox.mins[ZZ]);
    hgh_wt = CLIP( hgh_wt, 0.0f, 1.0f);

    low_wt = 1.0f - hgh_wt;


    lighting = 0.0f;
    if( low_wt > 0.0f )
    {
        lighting += low_wt * evaluate_lighting_vector( src->lighting_low, nrm );
    }
    if( hgh_wt > 0.0f )
    {
        lighting += hgh_wt * evaluate_lighting_vector( src->lighting_hgh, nrm );
    }

    if( NULL != light_amb || NULL != light_dir )
    {
        if( lighting <= 0.0f )
        {
            if( NULL != light_amb )
            {
                *light_amb = 0.0f;
            }

            if( NULL != light_dir )
            {
                *light_dir = 0.0f;
            }
        }
        else
        {
            // we are interested in splitting the light up into ambient and
            // directional components... do a bit more calculation

            int cnt;
            float ftmp, lighting_amb;

            lighting_amb = low_wt * src->lighting_low[0] + hgh_wt *src->lighting_hgh[0];
            for( cnt = 1; cnt < 6; cnt++ )
            {
                ftmp = low_wt * src->lighting_low[cnt] + hgh_wt *src->lighting_hgh[cnt];
                lighting_amb = MIN(lighting_amb, ftmp);
            }

            if( NULL != light_amb )
            {
                *light_amb = lighting_amb;
            }

            if( NULL != light_dir )
            {
                *light_dir = lighting - lighting_amb;
            }
        }
    }

    return lighting;
}

//--------------------------------------------------------------------------------------------
float evaluate_lighting_vector( lighting_vector_t lvec, GLfloat nrm[] )
{
    float lighting;

    if ( NULL == nrm ) return 0.0f;

    lighting = 0.0f;

    if ( nrm[XX] > 0.0f )
    {
        lighting += ABS(nrm[XX]) * lvec[0];
    }
    else if ( nrm[XX] < 0.0f )
    {
        lighting += ABS(nrm[XX]) * lvec[1];
    }

    if ( nrm[YY] > 0.0f )
    {
        lighting += ABS(nrm[YY]) * lvec[2];
    }
    else if ( nrm[YY] < 0.0f )
    {
        lighting += ABS(nrm[YY]) * lvec[3];
    }

    if ( nrm[ZZ] > 0.0f )
    {
        lighting += ABS(nrm[ZZ]) * lvec[4];
    }
    else if ( nrm[ZZ] < 0.0f )
    {
        lighting += ABS(nrm[ZZ]) * lvec[5];
    }

    return lighting;
}

