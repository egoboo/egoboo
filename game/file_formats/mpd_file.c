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

/// @file file_formats/mpd_file.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "mpd_file.h"
#include "log.h"

#include "egoboo_math.inl"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static mpd_info_t * mpd_info_ctor( mpd_info_t * pinfo );
static mpd_info_t * mpd_info_dtor( mpd_info_t * pinfo );

static mpd_mem_t *  mpd_mem_ctor( mpd_mem_t * pmem );
static mpd_mem_t *  mpd_mem_dtor( mpd_mem_t * pmem );
static bool_t       mpd_mem_free( mpd_mem_t * pmem );
static bool_t       mpd_mem_alloc( mpd_mem_t * pmem, mpd_info_t * pinfo );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

fvec3_t   map_twist_nrm[256];
Uint32    map_twist_y[256];            // For surface normal of mesh
Uint32    map_twist_x[256];
float     map_twistvel_x[256];            // For sliding down steep hills
float     map_twistvel_y[256];
float     map_twistvel_z[256];
Uint8     map_twist_flat[256];

tile_definition_t tile_dict[MAXMESHTYPE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tile_dictionary_load_vfs( const char * filename, tile_definition_t dict[], size_t dict_size )
{
    /// @details ZZ@> This function loads fan types for the terrain

    Uint32 cnt, entry, vertices, commandsize;
    int numfantype, fantype, bigfantype;
    int numcommand, command;
    int itmp;
    float ftmp;
    vfs_FILE* fileread;

    if ( !VALID_CSTR( filename ) || NULL == dict || dict_size < 2 ) return;

    // Initialize all mesh types to 0
    for ( entry = 0; entry < dict_size; entry++ )
    {
        dict[entry].numvertices = 0;
        dict[entry].command_count = 0;
    }

    // Open the file and go to it
    fileread = vfs_openRead( filename );
    if ( NULL == fileread )
    {
        log_error( "Cannot load the tile definitions \"%s\".\n", filename );
        return;
    }

    numfantype = fget_next_int( fileread );

    for ( fantype = 0; fantype < numfantype; fantype++ )
    {
        bigfantype = fantype + dict_size / 2;  // Duplicate for 64x64 tiles

        vertices = fget_next_int( fileread );
        dict[fantype].numvertices = vertices;
        dict[bigfantype].numvertices = vertices;  // Dupe

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            itmp = fget_next_int( fileread );

            ftmp = fget_next_float( fileread );
            dict[fantype].u[cnt] = ftmp;
            dict[bigfantype].u[cnt] = ftmp;  // Dupe

            ftmp = fget_next_float( fileread );
            dict[fantype].v[cnt] = ftmp;
            dict[bigfantype].v[cnt] = ftmp;  // Dupe
        }

        numcommand = fget_next_int( fileread );
        dict[fantype].command_count = numcommand;
        dict[bigfantype].command_count = numcommand;  // Dupe

        for ( entry = 0, command = 0; command < numcommand; command++ )
        {
            commandsize = fget_next_int( fileread );
            dict[fantype].command_entries[command] = commandsize;
            dict[bigfantype].command_entries[command] = commandsize;  // Dupe

            for ( cnt = 0; cnt < commandsize; cnt++ )
            {
                itmp = fget_next_int( fileread );
                dict[fantype].command_verts[entry] = itmp;
                dict[bigfantype].command_verts[entry] = itmp;  // Dupe

                entry++;
            }
        }
    }

    vfs_close( fileread );

    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < dict_size / 2; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = (( 0.6f / 32 ) + ( dict[entry].u[cnt] * 30.8f / 32 ) ) / 8;
            dict[entry].v[cnt] = (( 0.6f / 32 ) + ( dict[entry].v[cnt] * 30.8f / 32 ) ) / 8;
        }
    }

    // Do for big tiles too
    for ( /* nothing */; entry < dict_size; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = (( 0.6f / 64 ) + ( dict[entry].u[cnt] * 62.8f / 64 ) ) / 4;
            dict[entry].v[cnt] = (( 0.6f / 64 ) + ( dict[entry].v[cnt] * 62.8f / 64 ) ) / 4;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_t * mpd_ctor( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    memset( pmesh, 0, sizeof( *pmesh ) );

    if ( NULL == mpd_mem_ctor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == mpd_info_ctor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_dtor( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == mpd_mem_dtor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == mpd_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_free( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return bfalse;

    mpd_mem_free( &( pmesh->mem ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_load( const char *loadname, mpd_t * pmesh )
{
    ///// @details ZZ@> This function loads the level.mpd file
    FILE* fileread;
    int itmp;
    float ftmp;
    Uint32 fan, cnt;
    Uint32 tiles_count;
    Uint8 btemp;

    mpd_info_t * pinfo;
    mpd_mem_t  * pmem;

    if ( NULL == pmesh || INVALID_CSTR( loadname ) ) return pmesh;

    printf( "---- mpd_load(\"%s\",%p)\n", loadname, pmesh );

    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    fileread = fopen( loadname, "rb" );
    if ( NULL == fileread )
    {
        log_warning( "mpd_load() - cannot find \"%s\"!!\n", loadname );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );
    if ( MAPID != ( Uint32 )ENDIAN_INT32( itmp ) )
    {
        log_warning( "mpd_load() - this is not a valid level.mpd!!\n" );
        fclose( fileread );
        return NULL;
    }

    // Read the number of vertices
    fread( &itmp, 4, 1, fileread );  pinfo->vertcount   = ( int )ENDIAN_INT32( itmp );

    // grab the tiles in x and y
    fread( &itmp, 4, 1, fileread );  pinfo->tiles_x = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_x >= MAXMESHTILEY )
    {
        mpd_dtor( pmesh );
        log_warning( "mpd_load() - invalid mpd size. Mesh too large in x direction.\n" );
        fclose( fileread );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );  pinfo->tiles_y = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_y >= MAXMESHTILEY )
    {
        mpd_dtor( pmesh );
        log_warning( "mpd_load() - invalid mpd size. Mesh too large in y direction.\n" );
        fclose( fileread );
        return NULL;
    }

    // allocate the mesh memory
    if ( !mpd_mem_alloc( pmem, pinfo ) )
    {
        mpd_dtor( pmesh );
        fclose( fileread );
        log_warning( "mpd_load() - could not allocate memory for the mesh!!\n" );
        return NULL;
    }

    tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // Load fan data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        fread( &itmp, 4, 1, fileread );
        pmem->tile_list[fan].type = CLIP_TO_08BITS( ENDIAN_INT32( itmp ) >> 24 );
        pmem->tile_list[fan].fx   = CLIP_TO_08BITS( ENDIAN_INT32( itmp ) >> 16 );
        pmem->tile_list[fan].img  = CLIP_TO_16BITS( ENDIAN_INT32( itmp ) >>  0 );
    }

    // Load twist data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        fread( &itmp, 1, 1, fileread );
        pmem->tile_list[fan].twist = ENDIAN_INT32( itmp );
    }

    // Load vertex x data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vlst[cnt].pos.x = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex y data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vlst[cnt].pos.y = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex z data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vlst[cnt].pos.z = ENDIAN_FLOAT( ftmp ) / 16.0f;  // Cartman uses 4 bit fixed point for Z
    }

    // Load vertex a data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        fread( &btemp, 1, 1, fileread );
        pmem->vlst[cnt].a = 0; // btemp;
    }

    fclose( fileread );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_ctor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_dtor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_ctor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_dtor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    mpd_mem_free( pmem );
    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_mem_alloc( mpd_mem_t * pmem, mpd_info_t * pinfo )
{
    int tile_count;

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mpd_mem_free( pmem ) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "mpd_mem_alloc() - mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate new memory
    pmem->vlst = EGOBOO_NEW_ARY( mpd_vertex_t, pinfo->vertcount );
    if ( NULL == pmem->vlst )
    {
        mpd_mem_free( pmem );
        log_error( "mpd_mem_alloc() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
        return bfalse;
    }
    pmem->vcount = pinfo->vertcount;

    tile_count = pinfo->tiles_x * pinfo->tiles_y;
    pmem->tile_list  = EGOBOO_NEW_ARY( tile_info_t, tile_count );
    if ( NULL == pmem->tile_list )
    {
        mpd_mem_free( pmem );
        log_error( "mpd_mem_alloc() - not enough memory to allocate the tile info\n" );
        return bfalse;
    }
    pmem->tile_count = tile_count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_mem_free( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->vlst );
    pmem->vcount = 0;

    EGOBOO_DELETE_ARY( pmem->tile_list );
    pmem->tile_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 cartman_get_twist( int x, int y )
{
    Uint8 twist;

    // x and y should be from -7 to 8
    if ( x < -7 ) x = -7;
    if ( x > 8 ) x = 8;
    if ( y < -7 ) y = -7;
    if ( y > 8 ) y = 8;

    // Now between 0 and 15
    x = x + 7;
    y = y + 7;
    twist = ( y << 4 ) + x;

    return twist;
}

//--------------------------------------------------------------------------------------------
bool_t twist_to_normal( Uint8 twist, float v[], float slide )
{
    int ix, iy;
    float dx, dy;
    float nx, ny, nz, nz2;
    float diff_xy;

    if ( NULL == v ) return bfalse;

    diff_xy = 128.0f / slide;

    ix = ( twist >> 0 ) & 0x0f;
    iy = ( twist >> 4 ) & 0x0f;
    ix -= 7;
    iy -= 7;

    dx = -ix / ( float )CARTMAN_FIXNUM * ( float )CARTMAN_SLOPE;
    dy = iy / ( float )CARTMAN_FIXNUM * ( float )CARTMAN_SLOPE;

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
