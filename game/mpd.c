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
#include "log.h"

#include "egoboo_math.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define CARTMAN_FIXNUM  4.125 // 4.150      // Magic number

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// mesh initialization - not accessible by scripts
static void   mesh_init_tile_offset( mesh_t * pmesh );
static void   mesh_make_vrtstart( mesh_t * pmesh );
static void   mesh_make_fanstart( mesh_t * pmesh );

static mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem );
static mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem );
static bool_t       mesh_mem_free( mesh_mem_t * pmem );
static bool_t       mesh_mem_allocate( mesh_mem_t * pmem, mesh_info_t * pinfo );

static mesh_info_t * mesh_info_new( mesh_info_t * pinfo );
static mesh_info_t * mesh_info_delete( mesh_info_t * pinfo );
static void          mesh_info_init( mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  );

static Uint8 cartman_get_fan_twist( mesh_t * pmesh, Uint32 tile );
static Uint8 cartman_get_twist(int x, int y);
static bool_t twist_to_normal( Uint8 twist, GLfloat v[], float slide );

static bool_t mesh_make_normals( mesh_t * pmesh );
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

GLvector3 map_twist_nrm[256];
Uint32    map_twist_y[256];            // For surface normal of mesh
Uint32    map_twist_x[256];
float     map_twistvel_x[256];            // For sliding down steep hills
float     map_twistvel_y[256];
float     map_twistvel_z[256];
Uint8     map_twist_flat[256];

mesh_t            mesh;
tile_definition_t tile_dict[MAXMESHTYPE];



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mesh_info_t * mesh_info_new( mesh_info_t * pinfo )
{
    if (NULL == pinfo) return pinfo;

    memset( pinfo, 0, sizeof(mesh_info_t) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
void mesh_info_init( mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  )
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
    if ( 0 != (pinfo->tiles_x & 0x03) ) pinfo->blocks_x++;

    pinfo->blocks_y = (pinfo->tiles_y >> 2);
    if ( 0 != (pinfo->tiles_y & 0x03) ) pinfo->blocks_y++;

    pinfo->blocks_count = pinfo->blocks_x * pinfo->blocks_y;

    // set the mesh edge info
    pinfo->edge_x = pinfo->tiles_x << TILE_BITS;
    pinfo->edge_y = pinfo->tiles_y << TILE_BITS;
};

//--------------------------------------------------------------------------------------------
mesh_info_t * mesh_info_delete( mesh_info_t * pinfo )
{
    if ( NULL != pinfo )
    {
        memset( pinfo, 0, sizeof(mesh_info_t) );
    }

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem )
{
    if (NULL == pmem) return pmem;

    memset( pmem, 0, sizeof(mesh_mem_t) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem )
{
    if ( NULL != pmem )
    {
        mesh_mem_free( pmem );
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mesh_t * mesh_new( mesh_t * pmesh )
{
    if ( NULL != pmesh )
    {
        memset( pmesh, 0, sizeof(mesh_t) );

        mesh_init_tile_offset( pmesh );

        mesh_mem_new( &(pmesh->mem) );
        mesh_info_new( &(pmesh->info) );
    }

    // global initialization
    mesh_make_twist();
    tile_dictionary_load( tile_dict, MAXMESHTYPE );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_delete( mesh_t * pmesh )
{
    if ( NULL != pmesh )
    {
        mesh_mem_delete( &(pmesh->mem) );
        mesh_info_delete( &(pmesh->info)  );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_renew( mesh_t * pmesh )
{
    pmesh = mesh_delete( pmesh );

    return mesh_new( pmesh );
}


//--------------------------------------------------------------------------------------------
mesh_t * mesh_create( mesh_t * pmesh, int tiles_x, int tiles_y )
{
    if (NULL == pmesh)
    {
//        pmesh = calloc(1, sizeof(mesh_t));
        pmesh = mesh_new(pmesh);
    }

    if (NULL != pmesh)
    {
        // intitalize the mesh info using the max number of vertices for each tile
        mesh_info_init( &(pmesh->info), -1, tiles_x, tiles_y );

        // allocate the mesh memory
        mesh_mem_allocate( &(pmesh->mem), &(pmesh->info) );

        return pmesh;
    };

    return pmesh;
}

//--------------------------------------------------------------------------------------------
void mesh_init_tile_offset(mesh_t * pmesh)
{
    int cnt;

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff_u[cnt] = ( cnt & 7 ) / 8.0f;
        pmesh->tileoff_v[cnt] = ( cnt >> 3 ) / 8.0f;
    }
 }

//--------------------------------------------------------------------------------------------
mesh_t * mesh_load( const char *modname, mesh_t * pmesh )
{
    // ZZ> This function loads the level.mpd file
    FILE* fileread;
    STRING newloadname;
    int itmp, tiles_x, tiles_y;
    float ftmp;
    Uint32 fan, cnt;
    int numvert;
    Uint8 btemp;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh)
    {
        pmesh = mesh_new(pmesh);
    }
    if (NULL == pmesh) return pmesh;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    // free any memory that has been allocated
    mesh_renew(pmesh);

    make_newloadname( modname, "gamedat" SLASH_STR "level.mpd", newloadname );
    fileread = fopen( newloadname, "rb" );
    if ( NULL == fileread )
    {
        log_warning( "Cannot find level.mpd!!\n" );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );
    if ( MAPID != ( Uint32 )ENDIAN_INT32( itmp ) )
    {
        log_warning( "This is not a valid level.mpd!!\n" );
        fclose( fileread );
        return NULL;
    }

    // Read the number of vertices
    fread( &itmp, 4, 1, fileread );  numvert   = ( int )ENDIAN_INT32( itmp );

    // grab the tiles in x and y
    fread( &itmp, 4, 1, fileread );  tiles_x = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_x >= MAXMESHTILEY )
    {
        mesh_delete( pmesh );
        log_warning( "Invalid mesh size. Mesh too large in x direction.\n" );
        fclose( fileread );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );  tiles_y = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_y >= MAXMESHTILEY )
    {
        mesh_delete( pmesh );
        log_warning( "Invalid mesh size. Mesh too large in y direction.\n" );
        fclose( fileread );
        return NULL;
    }

    // intitalize the mesh info
    mesh_info_init( pinfo, numvert, tiles_x, tiles_y );

    // allocate the mesh memory
    if ( !mesh_mem_allocate( pmem, pinfo ) )
    {
        mesh_delete( pmesh );
        fclose( fileread );
        log_warning( "Could not allocate memory for the mesh!!\n" );
        return NULL;
    }

    // Load fan data
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        fread( &itmp, 4, 1, fileread );
        pmem->tile_list[fan].type = (ENDIAN_INT32( itmp ) >> 24) & 0xFF;
        pmem->tile_list[fan].fx   = (ENDIAN_INT32( itmp ) >> 16) & 0xFF;
        pmem->tile_list[fan].img  = (ENDIAN_INT32( itmp )      ) & 0xFFFF;
    }

    // Load twist data
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        fread( &itmp, 1, 1, fileread );
        pmem->tile_list[fan].twist = ENDIAN_INT32( itmp );
    }

    // Load vertex x data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_x[cnt] = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex y data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_y[cnt] = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex z data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_z[cnt] = ENDIAN_FLOAT( ftmp ) / 16.0f;  // Cartman uses 4 bit fixed point for Z
    }

    // Load vertex a data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &btemp, 1, 1, fileread );
        pmem->vrt_a[cnt] = 0; // btemp;
        pmem->vrt_l[cnt] = 0;
    }

    fclose( fileread );

    mesh_make_fanstart( pmesh );
    mesh_make_vrtstart( pmesh );

    // recalculate the twist
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        Uint8 twist = cartman_get_fan_twist(pmesh, fan);
        pmem->tile_list[fan].twist = twist;
    }

    mesh_make_normals( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block_int( mesh_t * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->info.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->info.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->mem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile_int( mesh_t * pmesh, int tile_x,  int tile_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( tile_x < 0 || tile_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( tile_y < 0 || tile_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return tile_x + pmesh->mem.tilestart[tile_y];
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_allocate( mesh_mem_t * pmem, mesh_info_t * pinfo  )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mesh_mem_free(pmem) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate new memory
    pmem->vrt_x = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_x == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_y = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_y == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_z = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_z == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_a = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_a == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_l = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_l == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->cache = ( light_cache_t * ) calloc( pinfo->vertcount, sizeof(light_cache_t) );
    if ( pmem->cache == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->nrm = ( GLvector3 * ) calloc( pinfo->vertcount, sizeof(GLvector3) );
    if ( pmem->nrm == NULL )
    {
        log_error( "Reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    // set the vertex count
    pmem->vertcount = pinfo->vertcount;

    pmem->tile_list  = (tile_info_t *) calloc( pinfo->tiles_count, sizeof(tile_info_t) );

    pmem->blockstart = (Uint32*) calloc( pinfo->blocks_y, sizeof(Uint32) );
    pmem->tilestart  = (Uint32*) calloc( pinfo->tiles_y, sizeof(Uint32) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_free( mesh_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->vrt_x != NULL )
    {
        free( pmem->vrt_x );
        pmem->vrt_x = NULL;
    }

    if ( pmem->vrt_y != NULL )
    {
        free( pmem->vrt_y );
        pmem->vrt_y = NULL;
    }

    if ( pmem->vrt_z != NULL )
    {
        free( pmem->vrt_z );
        pmem->vrt_z = NULL;
    }

    if ( pmem->vrt_a != NULL )
    {
        free( pmem->vrt_a );
        pmem->vrt_a = NULL;
    }

    if ( pmem->vrt_l != NULL )
    {
        free( pmem->vrt_l );
        pmem->vrt_l = NULL;
    }

    if ( NULL != pmem->tile_list )
    {
        free(pmem->tile_list);
        pmem->tile_list = NULL;
    }

    if ( NULL != pmem->blockstart )
    {
        free(pmem->blockstart);
        pmem->blockstart = NULL;
    }

    if ( NULL != pmem->tilestart )
    {
        free(pmem->tilestart);
        pmem->tilestart = NULL;
    }

    if ( NULL != pmem->cache )
    {
        free(pmem->cache);
        pmem->cache = NULL;
    }

    if ( NULL != pmem->nrm )
    {
        free(pmem->nrm);
        pmem->nrm = NULL;
    }

    // reset some values to safe values
    pmem->vertcount = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void mesh_make_fanstart( mesh_t * pmesh )
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh) return;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    // do the tilestart
    for ( cnt = 0; cnt < pinfo->tiles_y; cnt++ )
    {
        pmem->tilestart[cnt] = pinfo->tiles_x * cnt;
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
        pmem->blockstart[cnt] = pinfo->blocks_x * cnt;
    }

}

//--------------------------------------------------------------------------------------------
void mesh_make_vrtstart( mesh_t * pmesh )
{
    int x, y, vert;
    Uint32 fan;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh) return;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    vert = 0;
    for ( y = 0; y < pinfo->tiles_y; y++ )
    {
        for ( x = 0; x < pinfo->tiles_x; x++ )
        {
            // allow raw access because we are careful
            fan = mesh_get_tile_int( pmesh, x, y );
            if ( VALID_TILE(pmesh, fan) )
            {
                Uint8 ttype = pmem->tile_list[fan].type;

                pmem->tile_list[fan].vrtstart = vert;

                if ( ttype > MAXMESHTYPE )
                {
                    vert += 4;
                }
                else
                {
                    // throw away any remaining upper bits
                    ttype &= 0x3F;
                    vert += tile_dict[ttype].numvertices;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------
void mesh_make_twist()
{
    // ZZ> This function precomputes surface normals and steep hill acceleration for
    //     the mesh
    Uint16 cnt;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        GLfloat nrm[3];

        twist_to_normal(cnt, nrm, 1.0f);

        map_twist_nrm[cnt].x = nrm[0];
        map_twist_nrm[cnt].y = nrm[1];
        map_twist_nrm[cnt].z = nrm[2];

        map_twist_x[cnt] = 32768 - ATAN2( nrm[1], nrm[2] ) / TWO_PI * 0xFFFF;
        map_twist_y[cnt] = 32768 + ATAN2( nrm[0], nrm[2] ) / TWO_PI * 0xFFFF;

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
float mesh_get_level( mesh_t * pmesh, float x, float y )
{
    // ZZ> This function returns the height of a point within a mesh fan, precisely

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_tile( pmesh, x, y );
    if ( !VALID_TILE(pmesh, tile) ) return 0;

    ix = x;
    iy = y;

    ix &= 127;
    iy &= 127;

    z0 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 0 ];
    z1 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 1 ];
    z2 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 2 ];
    z3 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 3 ];

    zleft = ( z0 * ( 128 - iy ) + z3 * iy ) / TILE_SIZE;
    zright = ( z1 * ( 128 - iy ) + z2 * iy ) / TILE_SIZE;
    zdone = ( zleft * ( 128 - ix ) + zright * ix ) / TILE_SIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( mesh_t * pmesh, float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x < pmesh->info.edge_x && pos_y >= 0.0f && pos_y < pmesh->info.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 9;
        iy >>= 9;

        block = mesh_get_block_int(pmesh, ix, iy);
    }

    return block;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( mesh_t * pmesh, float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < pmesh->info.edge_x && pos_y >= 0.0f && pos_y < pmesh->info.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 7;
        iy >>= 7;

        tile = mesh_get_tile_int( pmesh, ix, iy );
    }

    return tile;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Uint8 cartman_get_fan_twist( mesh_t * pmesh, Uint32 tile )
{
    int vrtstart;
    float z0, z1, z2, z3;
    float zx, zy;

    // check for a valid tile
    if ( INVALID_TILE == tile  || tile > pmesh->info.tiles_count ) return TWIST_FLAT;

    // check for fanoff
    if ( 0 != (0xFF00 & pmesh->mem.tile_list[tile].img) ) return TWIST_FLAT;


    vrtstart = pmesh->mem.tile_list[tile].vrtstart;

    z0 = pmesh->mem.vrt_z[vrtstart + 0];
    z1 = pmesh->mem.vrt_z[vrtstart + 1];
    z2 = pmesh->mem.vrt_z[vrtstart + 2];
    z3 = pmesh->mem.vrt_z[vrtstart + 3];

    zx = CARTMAN_FIXNUM * (z0 + z3 - z1 - z2) / SLOPE;
    zy = CARTMAN_FIXNUM * (z2 + z3 - z0 - z1) / SLOPE;

    return cartman_get_twist( zx, zy );

}

//------------------------------------------------------------------------------
Uint8 cartman_get_twist(int x, int y)
{
    Uint8 twist;

    // x and y should be from -7 to 8
    if (x < -7) x = -7;
    if (x > 8) x = 8;
    if (y < -7) y = -7;
    if (y > 8) y = 8;

    // Now between 0 and 15
    x = x + 7;
    y = y + 7;
    twist = (y << 4) + x;

    return twist;
}

//------------------------------------------------------------------------------
bool_t twist_to_normal( Uint8 twist, GLfloat v[], float slide )
{
    int ix, iy;
    float dx, dy;
    float nx, ny, nz, nz2;
    float diff_xy;

    if (NULL == v) return bfalse;

    diff_xy = 128.0f / slide;

    ix = (twist >> 0) & 0x0f;
    iy = (twist >> 4) & 0x0f;
    ix -= 7;
    iy -= 7;

    dx = -ix / (float)CARTMAN_FIXNUM * (float)SLOPE;
    dy = iy / (float)CARTMAN_FIXNUM * (float)SLOPE;

    // determine the square of the z normal
    nz2 =  diff_xy * diff_xy / ( dx * dx + dy * dy + diff_xy * diff_xy );

    // determine the z normal
    nz = 0.0f;
    if ( nz2 > 0.0f )
    {
        nz = SQRT( nz2 );
    }

    nx = - dx * nz / diff_xy;
    ny = - dy * nz / diff_xy;

    v[0] = nx;
    v[1] = ny;
    v[2] = nz;

    return btrue;
}

//------------------------------------------------------------------------------
Uint32 mesh_test_fx( mesh_t * pmesh, Uint32 itile, Uint32 flags )
{
    // test for mesh
    if ( NULL == pmesh ) return 0;

    // test for invalid tile
    if ( itile > pmesh->info.tiles_count )
    {
        return flags & (MPDFX_WALL | MPDFX_IMPASS);
    }

    // test for FANOFF
    if ( 0xFF00 == (0xFF00 & pmesh->mem.tile_list[itile].img) )
    {
        return flags & (MPDFX_WALL | MPDFX_IMPASS);
    }

    return pmesh->mem.tile_list[itile].fx & flags;
}

//------------------------------------------------------------------------------
bool_t mesh_make_normals( mesh_t * pmesh )
{
    int ix, iy;
    int dx, dy;
    GLvector3 nrm_sum;
    Uint32 fan;
    int wt_sum;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    for (iy = 0; iy < pmesh->info.tiles_y; iy++)
    {
        for (ix = 0; ix < pmesh->info.tiles_x; ix++)
        {
            // find the average normal for the upper left map vertex
            wt_sum = 0;
            nrm_sum.x = nrm_sum.y = nrm_sum.z = 0;
            for (dy = -1; dy <= 0; dy++)
            {
                for (dx = -1; dx <= 0; dx++)
                {
                    fan = mesh_get_tile_int(pmesh, ix + dx, iy + dy);
                    if ( VALID_TILE(pmesh, fan) )
                    {
                        Uint8 twist = pmesh->mem.tile_list[fan].twist;

                        nrm_sum.x += map_twist_nrm[twist].x;
                        nrm_sum.y += map_twist_nrm[twist].y;
                        nrm_sum.z += map_twist_nrm[twist].z;

                        wt_sum++;
                    }
                }
            }

            fan = mesh_get_tile_int(pmesh, ix, iy);
            if ( VALID_TILE(pmesh, fan) )
            {
                if ( 0 == wt_sum )
                {
                    pmesh->mem.nrm[fan].x = pmesh->mem.nrm[fan].y = 0.0f;
                    pmesh->mem.nrm[fan].z = 1.0f;
                }
                else if ( 1 == wt_sum )
                {
                    pmesh->mem.nrm[fan] = nrm_sum;
                }
                else
                {
                    pmesh->mem.nrm[fan] = VNormalize( nrm_sum );
                }
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void tile_dictionary_load(tile_definition_t dict[], size_t dict_size)
{
    // ZZ> This function loads fan types for the terrain
    Uint32 cnt, entry, vertices, commandsize;
    int numfantype, fantype, bigfantype;
    int numcommand, command;
    int itmp;
    float ftmp;
    FILE* fileread;

    if ( NULL == dict || dict_size < 2 ) return;

    // Initialize all mesh types to 0
    for ( entry = 0; entry < dict_size; entry++ )
    {
        dict[entry].numvertices = 0;
        dict[entry].command_count = 0;
    }

    // Open the file and go to it
    fileread = fopen( "basicdat" SLASH_STR "fans.txt", "r" );
    if ( NULL == fileread )
    {
        log_error( "Cannot load the tile definitions \"basicdat" SLASH_STR "fans.txt\" \n" );
        return;
    }

    goto_colon( NULL, fileread, bfalse );
    fscanf( fileread, "%d", &numfantype );

    for ( fantype = 0; fantype < numfantype; fantype++ )
    {
        bigfantype = fantype + dict_size / 2;  // Duplicate for 64x64 tiles

        goto_colon( NULL, fileread, bfalse );
        fscanf( fileread, "%d", &vertices );
        dict[fantype].numvertices = vertices;
        dict[bigfantype].numvertices = vertices;  // Dupe

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%d", &itmp );

            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%f", &ftmp );
            dict[fantype].u[cnt] = ftmp;
            dict[bigfantype].u[cnt] = ftmp;  // Dupe

            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%f", &ftmp );
            dict[fantype].v[cnt] = ftmp;
            dict[bigfantype].v[cnt] = ftmp;  // Dupe
        }

        goto_colon( NULL, fileread, bfalse );
        fscanf( fileread, "%d", &numcommand );
        dict[fantype].command_count = numcommand;
        dict[bigfantype].command_count = numcommand;  // Dupe

        for ( entry = 0, command = 0; command < numcommand; command++ )
        {
            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%d", &commandsize );
            dict[fantype].command_entries[command] = commandsize;
            dict[bigfantype].command_entries[command] = commandsize;  // Dupe

            for ( cnt = 0; cnt < commandsize; cnt++ )
            {
                goto_colon( NULL, fileread, bfalse );
                fscanf( fileread, "%d", &itmp );
                dict[fantype].command_verts[entry] = itmp;
                dict[bigfantype].command_verts[entry] = itmp;  // Dupe

                entry++;
            }
        }
    }

    fclose( fileread );

    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < dict_size / 2; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = ( ( 0.6f / 32 ) + ( dict[entry].u[cnt] * 30.8f / 32 ) ) / 8;
            dict[entry].v[cnt] = ( ( 0.6f / 32 ) + ( dict[entry].v[cnt] * 30.8f / 32 ) ) / 8;
        }
    }

    // Do for big tiles too
    for ( /* nothing */; entry < dict_size; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = ( ( 0.6f / 64 ) + ( dict[entry].u[cnt] * 62.8f / 64 ) ) / 4;
            dict[entry].v[cnt] = ( ( 0.6f / 64 ) + ( dict[entry].v[cnt] * 62.8f / 64 ) ) / 4;
        }
    }

}

