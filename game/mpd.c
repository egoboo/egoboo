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

// mesh initialization - not accessible by scripts
static void   mesh_make_vrtstart( mesh_t * pmesh );
static void   mesh_make_fanstart( mesh_t * pmesh );
static void   make_twist();

static mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem );
static mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem );
static bool_t       mesh_mem_free( mesh_mem_t * pmem );
static bool_t       mesh_mem_allocate( mesh_mem_t * pmem, mesh_info_t * pinfo );

static mesh_info_t * mesh_info_new( mesh_info_t * pinfo );
static mesh_info_t * mesh_info_delete( mesh_info_t * pinfo );
static void          mesh_info_init( mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static mesh_t _mesh[2];

mesh_t * PMesh = _mesh + 0;

Uint32 maplrtwist[256];            // For surface normal of mesh
Uint32 mapudtwist[256];
float  vellrtwist[256];            // For sliding down steep hills
float  veludtwist[256];
Uint8  flattwist[256];

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
    pinfo->edge_x = pinfo->tiles_x << 7;
    pinfo->edge_y = pinfo->tiles_y << 7;
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

        mesh_mem_new( &(pmesh->mem) );
        mesh_info_new( &(pmesh->info) );
    }

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
mesh_t * mesh_load( const char *modname, mesh_t * pmesh )
{
    // ZZ> This function loads the level.mpd file
    FILE* fileread;
    char newloadname[256];
    int itmp, tiles_x, tiles_y;
    float ftmp;
    Uint32 fan, cnt;
    int numvert;
    Uint8 btemp;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh)
    {
//        pmesh = calloc(1, sizeof(mesh_t));
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
        pmem->vrt_a[cnt] = btemp;
        pmem->vrt_l[cnt] = 0;
    }

    fclose( fileread );

    mesh_make_fanstart( pmesh );
    mesh_make_vrtstart( pmesh );
    make_twist();

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff_u[cnt] = ( cnt & 7 ) / 8.0f;
        pmesh->tileoff_v[cnt] = ( cnt >> 3 ) / 8.0f;
    }

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
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_y = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_y == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_z = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_z == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_a = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_a == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_l = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_l == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
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
            if ( INVALID_TILE != fan )
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
void make_twist()
{
    // ZZ> This function precomputes surface normals and steep hill acceleration for
    //     the mesh
    Uint16 cnt;
    int x, y;
    float xslide, yslide;

    cnt = 0;

    while ( cnt < 256 )
    {
        y = (cnt >> 4) & 0x0F;
        x = (cnt >> 0) & 0x0F;
        y -= 7;  // -7 to 8
        x -= 7;  // -7 to 8

        mapudtwist[cnt] = 32768 + y * SLOPE;
        maplrtwist[cnt] = 32768 + x * SLOPE;

        if ( ABS( y ) >= 7 ) y = y << 1;
        if ( ABS( x ) >= 7 ) x = x << 1;

        xslide = x * SLIDE;
        yslide = y * SLIDE;
        if ( xslide < 0 )
        {
            xslide += SLIDEFIX;
            if ( xslide > 0 )
                xslide = 0;
        }
        else
        {
            xslide -= SLIDEFIX;
            if ( xslide < 0 )
                xslide = 0;
        }
        if ( yslide < 0 )
        {
            yslide += SLIDEFIX;
            if ( yslide > 0 )
                yslide = 0;
        }
        else
        {
            yslide -= SLIDEFIX;
            if ( yslide < 0 )
                yslide = 0;
        }

        veludtwist[cnt] = -yslide * hillslide;
        vellrtwist[cnt] =  xslide * hillslide;
        flattwist[cnt] = bfalse;
        if ( ABS( veludtwist[cnt] ) + ABS( vellrtwist[cnt] ) < SLIDEFIX*4 )
        {
            flattwist[cnt] = btrue;
        }

        cnt++;
    }
}


//---------------------------------------------------------------------------------------------
float get_level( mesh_t * pmesh, float x, float y, bool_t waterwalk )
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
    //     level of the water.

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_tile( pmesh, x, y );
    if ( INVALID_TILE == tile ) return 0;

    ix = x;
    iy = y;

    ix &= 127;
    iy &= 127;

    z0 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 0 ];
    z1 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 1 ];
    z2 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 2 ];
    z3 = pmesh->mem.vrt_z[ pmesh->mem.tile_list[tile].vrtstart + 3 ];

    zleft = ( z0 * ( 128 - iy ) + z3 * iy ) / (float)(1 << 7);
    zright = ( z1 * ( 128 - iy ) + z2 * iy ) / (float)(1 << 7);
    zdone = ( zleft * ( 128 - ix ) + zright * ix ) / (float)(1 << 7);

    if ( waterwalk )
    {
        if ( watersurfacelevel > zdone && 0 != ( pmesh->mem.tile_list[tile].fx & MESHFX_WATER ) && wateriswater )
        {
            return watersurfacelevel;
        }
    }

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

