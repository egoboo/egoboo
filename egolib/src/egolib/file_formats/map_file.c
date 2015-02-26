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

/// @file file_formats/map_file.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/file_formats/map_file.h"
#include "egolib/file_formats/map_file-v1.h"
#include "egolib/file_formats/map_file-v2.h"
#include "egolib/file_formats/map_file-v3.h"
#include "egolib/file_formats/map_file-v4.h"

#include "egolib/file_formats/map_tile_dictionary.h"

#include "egolib/map_functions.h"

#include "egolib/log.h"

#include "egolib/endian.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CURRENT_MAP_VERSION_NUMBER (( CURRENT_MAP_VERSION_LETTER - 'A' ) + 1 )
#define CURRENT_MAP_ID             ( MAP_ID_BASE + (CURRENT_MAP_VERSION_LETTER - 'A') )
#define GET_MAP_VERSION_NUMBER(VAL) LAMBDA( (static_cast<uint32_t>(VAL)) >= (static_cast<uint32_t>(MAP_ID_BASE)), (static_cast<uint32_t>(VAL)) - (static_cast<uint32_t>(MAP_ID_BASE)) + 1, -1 )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static map_info_t *map_info_ctor( map_info_t * pinfo );
static map_info_t *map_info_dtor( map_info_t * pinfo );

static map_mem_t *map_mem_ctor( map_mem_t * pmem );
static map_mem_t *map_mem_dtor( map_mem_t * pmem );
static bool map_mem_free( map_mem_t * pmem );
static bool map_mem_alloc( map_mem_t * pmem, map_info_t * pinfo );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t *map_ctor( map_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    BLANK_STRUCT_PTR( pmesh )

    if ( NULL == map_mem_ctor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == map_info_ctor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t *map_dtor( map_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == map_mem_dtor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == map_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_renew( map_t * pmesh )
{
    pmesh = map_dtor( pmesh );
    pmesh = map_ctor( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool map_free( map_t * pmesh )
{
    if ( NULL == pmesh ) return false;

    map_mem_free( &( pmesh->mem ) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool map_init( map_t * pmesh, map_info_t * pinfo )
{
    map_info_t loc_info;

    if ( NULL == pmesh || NULL == pinfo ) return false;

    // make a copy of the mesh info, in case "pinfo == &(pmesh->info)"
    memcpy( &loc_info, pinfo, sizeof( loc_info ) );

    // renew the mesh
    map_renew( pmesh );

    // check the tiles in the x direction
    if ( loc_info.tiles_x >= MAP_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in x direction, %d.\n", __FUNCTION__, loc_info.tiles_x );
        goto map_init_fail;
    }

    // check the tiles in the y direction
    if ( loc_info.tiles_y >= MAP_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in y direction, %d.\n", __FUNCTION__, loc_info.tiles_y );
        goto map_init_fail;
    }

    // check the total number of vertices
    if ( loc_info.vertcount > MAP_VERTICES_MAX )
    {
        log_warning( "%s - invalid mpd size. too many vertices, %" PRIuZ ".\n", __FUNCTION__ , loc_info.vertcount );
    }

    // allocate the mesh memory
    if ( !map_mem_alloc( &( pmesh->mem ), pinfo ) )
    {
        log_warning( "%s - could not allocate memory for the mesh!!\n", __FUNCTION__ );
        goto map_init_fail;
    }

    // copy the desired mesh info into the actual mesh info
    memmove( &( pmesh->info ), &loc_info, sizeof( pmesh->info ) );

    return true;

map_init_fail:

    map_dtor( pmesh );

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_info_t * map_info_ctor( map_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
map_info_t * map_info_dtor( map_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_mem_t * map_mem_ctor( map_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
map_mem_t * map_mem_dtor( map_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    map_mem_free( pmem );
    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool map_mem_alloc( map_mem_t * pmem, map_info_t * pinfo )
{
    int tile_count;

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return false;

    // free any memory already allocated
    if ( !map_mem_free( pmem ) ) return false;

    if ( pinfo->vertcount > MAP_VERTICES_MAX )
    {
        log_warning( "map_mem_alloc() - mesh requires too much memory ( %" PRIuZ " requested, but max is %d ). \n", pinfo->vertcount, MAP_VERTICES_MAX );
        return false;
    }

    // allocate new memory
    pmem->vlst = EGOBOO_NEW_ARY( map_vertex_t, pinfo->vertcount );
    if ( NULL == pmem->vlst )
    {
        map_mem_free( pmem );
        log_error( "map_mem_alloc() - reduce the maximum number of vertices! (Check MAP_VERTICES_MAX)\n" );

        return false;
    }
    pmem->vcount = pinfo->vertcount;

    tile_count = pinfo->tiles_x * pinfo->tiles_y;
    pmem->tile_list  = EGOBOO_NEW_ARY( tile_info_t, tile_count );
    if ( NULL == pmem->tile_list )
    {
        map_mem_free( pmem );
        log_error( "map_mem_alloc() - not enough memory to allocate the tile info\n" );

        return false;
    }
    pmem->tile_count = tile_count;

    return true;
}

//--------------------------------------------------------------------------------------------
bool map_mem_free( map_mem_t * pmem )
{
    if ( NULL == pmem ) return false;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->vlst );
    pmem->vcount = 0;

    EGOBOO_DELETE_ARY( pmem->tile_list );
    pmem->tile_count = 0;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t * map_load( const char *loadname, map_t * pmesh )
{
    /// @author ZZ
    /// @details This function loads the level.mpd file

    vfs_FILE* fileread = NULL;

    int map_version;
    Uint32 tiles_count;
    bool test_limits = false;

    Uint32 ui32_tmp;
    map_info_t loc_info;

    Uint32     * tmp_bitmap = NULL;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;

    // if there is no filename, fail and wipe the mesh struct
    if ( INVALID_CSTR( loadname ) )
    {
        goto map_load_fail;
    }

    fileread = vfs_openReadB(loadname);
    if ( NULL == fileread )
    {
        log_warning( "%s - cannot find \"%s\"!!\n", __FUNCTION__, loadname );
        goto map_load_fail;
    }

    // read the file version
    vfs_read_Uint32( fileread, &ui32_tmp );

    // this number is backwards for our purpose
    ui32_tmp = SDL_Swap32( ui32_tmp );

    map_version = GET_MAP_VERSION_NUMBER( ui32_tmp );
    if ( map_version <= 0 )
    {
        log_warning( "%s - unknown map type!!\n", __FUNCTION__ );
        goto map_load_fail;
    }
    else if ( map_version > CURRENT_MAP_VERSION_NUMBER )
    {
        log_warning( "%s - file version is too recent or invalid. Not all features will be supported %d/%d.\n", __FUNCTION__, map_version, CURRENT_MAP_VERSION_NUMBER );
        test_limits = true;
    }

    // read the rest of the "header"
    vfs_read_Uint32( fileread, &ui32_tmp );
    if ( test_limits && ( ui32_tmp >= MAP_VERTICES_MAX ) )
    {
        log_warning( "%s - unknown version and out of range vertex count (%d/%d)!!\n", __FUNCTION__, ui32_tmp, MAP_VERTICES_MAX );
        goto map_load_fail;
    }
    loc_info.vertcount = ui32_tmp;

    vfs_read_Uint32( fileread, &ui32_tmp );
    if ( test_limits && ( ui32_tmp >= MAP_TILEY_MAX ) )
    {
        log_warning( "%s - unknown version and mesh too large in the x direction (%d/%d)!!\n", __FUNCTION__, ui32_tmp, MAP_TILEY_MAX );
        goto map_load_fail;
    }
    loc_info.tiles_x = ui32_tmp;

    vfs_read_Uint32( fileread, &ui32_tmp );
    if ( test_limits && ( ui32_tmp >= MAP_TILEY_MAX ) )
    {
        log_warning( "%s - unknown version and mesh too large in the y direction (%d/%d)!!\n", __FUNCTION__, ui32_tmp, MAP_TILEY_MAX );
        goto map_load_fail;
    }
    loc_info.tiles_y = ui32_tmp;

    // how many tiles are we asking for?
    tiles_count = loc_info.tiles_x * loc_info.tiles_y;
    if ( test_limits && ( tiles_count >= MAP_TILE_MAX ) )
    {
        log_warning( "%s - unknown version and mesh is too large (%d/%d)!!\n", __FUNCTION__, tiles_count, MAP_TILE_MAX );
        goto map_load_fail;
    }

    // allocate the mesh memory
    if ( !map_init( pmesh, &loc_info ) )
    {
        log_warning( "%s - could not initialize the map!!\n", __FUNCTION__ );
        goto map_load_fail;
    }

    // version 1 data is required
    if ( map_version > 0 )
    {
        pmesh = map_read_v1( fileread, pmesh );
    }

    // version 2 data is optional-ish
    if ( map_version > 1 )
    {
        pmesh = map_read_v2( fileread, pmesh );
    }
    else
    {
        pmesh = map_generate_tile_twist_data( pmesh );
    }

    // version 3 data is optional-ish
    if ( map_version > 2 )
    {
        pmesh = map_read_v3( fileread, pmesh );
    }
    else
    {
        pmesh = map_generate_fan_type_data( pmesh );
        pmesh = map_generate_vertex_data( pmesh );
    }

    // version 4 data is completely optional
    if ( map_version > 3 )
    {
        pmesh = map_read_v4( fileread, pmesh );
    }

    vfs_close( fileread );

    EGOBOO_DELETE_ARY( tmp_bitmap );

    return pmesh;

map_load_fail:

    map_renew( pmesh );

    if ( NULL != fileread )
    {
        vfs_close( fileread );
        fileread = NULL;
    }

    EGOBOO_DELETE_ARY( tmp_bitmap );

    return NULL;
}

//--------------------------------------------------------------------------------------------
map_t * map_save( const char * savename, map_t * pmesh )
{
    vfs_FILE *filewrite;
    int map_version = CURRENT_MAP_VERSION_NUMBER;

    map_info_t * pinfo;
    map_mem_t  * pmem;

    if ( NULL == pmesh || INVALID_CSTR( savename ) ) return NULL;

    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return NULL;

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return NULL;

    filewrite = vfs_openWriteB(savename);
    if ( NULL == filewrite ) return NULL;

    // write the file identifier
    vfs_write_Uint32( filewrite, SDL_Swap32(CURRENT_MAP_ID) );

    // write the file vertex count
    vfs_write_Uint32( filewrite, pinfo->vertcount );

    // write the tiles in the x direction
    vfs_write_Uint32( filewrite, pinfo->tiles_x );

    // write the tiles in the y direction
    vfs_write_Uint32( filewrite, pinfo->tiles_y );

    if ( map_version > 0 )
    {
        map_write_v1( filewrite, pmesh );
    }

    if ( map_version > 1 )
    {
        map_write_v2( filewrite, pmesh );
    }

    if ( map_version > 2 )
    {
        map_write_v3( filewrite, pmesh );
    }

    if ( map_version > 3 )
    {
        map_write_v4( filewrite, pmesh );
    }

    vfs_close( filewrite );

    return pmesh;
}
