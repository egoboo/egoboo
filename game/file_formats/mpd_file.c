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
#include "../log.h"

#include "../egoboo_endian.h"
#include "../egoboo_fileutil.h"
#include "../egoboo_strutil.h"
#include "../egoboo.h"

#include "../egoboo_math.inl"

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
fvec3_t   map_twist_vel[256];            // For sliding down steep hills
Uint8     map_twist_flat[256];

tile_definition_t tile_dict[MPD_FAN_TYPE_MAX];

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

    BLANK_STRUCT_PTR( pmesh )

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
mpd_t * mpd_renew( mpd_t * pmesh )
{
    pmesh = mpd_dtor( pmesh );
    pmesh = mpd_ctor( pmesh );

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
bool_t mpd_init( mpd_t * pmesh, mpd_info_t * pinfo )
{
    mpd_info_t loc_info;

    if ( NULL == pmesh || NULL == pinfo ) return bfalse;

    // make a copy of the mesh info, in case "pinfo == &(pmesh->info)"
    memcpy( &loc_info, pinfo, sizeof( loc_info ) );

    // renew the mesh
    mpd_renew( pmesh );

    // check the tiles in the x direction
    if ( loc_info.tiles_x >= MPD_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in x direction, %d.\n", __FUNCTION__, loc_info.tiles_x );
        goto mpd_init_fail;
    }

    // check the tiles in the y direction
    if ( loc_info.tiles_y >= MPD_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in y direction, %d.\n", __FUNCTION__, loc_info.tiles_y );
        goto mpd_init_fail;
    }

    // check the total number of vertices
    if ( loc_info.vertcount >= MPD_VERTICES_MAX )
    {
        log_warning( "%s - invalid mpd size. too many vertices, %d.\n", __FUNCTION__ , loc_info.vertcount );
    }

    // allocate the mesh memory
    if ( !mpd_mem_alloc( &( pmesh->mem ), pinfo ) )
    {
        log_warning( "%s - could not allocate memory for the mesh!!\n", __FUNCTION__ );
        goto mpd_init_fail;
    }

    // copy the desired mesh info into the actual mesh info
    memmove( &( pmesh->info ), &loc_info, sizeof( pmesh->info ) );

    return btrue;

mpd_init_fail:

    mpd_dtor( pmesh );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_ctor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_dtor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_ctor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_dtor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    mpd_mem_free( pmem );
    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_mem_alloc( mpd_mem_t * pmem, mpd_info_t * pinfo )
{
    int tile_count;

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mpd_mem_free( pmem ) ) return bfalse;

    if ( pinfo->vertcount > MPD_VERTICES_MAX )
    {
        log_warning( "mpd_mem_alloc() - mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MPD_VERTICES_MAX );
        return bfalse;
    }

    // allocate new memory
    pmem->vlst = EGOBOO_NEW_ARY( mpd_vertex_t, pinfo->vertcount );
    if ( NULL == pmem->vlst )
    {
        mpd_mem_free( pmem );
        log_error( "mpd_mem_alloc() - reduce the maximum number of vertices! (Check MPD_VERTICES_MAX)\n" );
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
mpd_t * mpd_load( const char *loadname, mpd_t * pmesh )
{
    ///// @details ZZ@> This function loads the level.mpd file
    FILE* fileread = NULL;

    Uint32 fan, cnt;
    Uint32 tiles_count;

    Uint32 ui32_tmp;
    Uint8 ui08_tmp;
    float ieee32_tmp;
    mpd_info_t loc_info;

    mpd_info_t * pinfo = NULL;
    mpd_mem_t  * pmem = NULL;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    // if there is no filename, fail and wipe the mesh struct
    if ( INVALID_CSTR( loadname ) )
    {
        goto mpd_load_fail;
    }

    fileread = fopen( loadname, "rb" );
    if ( NULL == fileread )
    {
        log_warning( "%s - cannot find \"%s\"!!\n", __FUNCTION__, loadname );
        goto mpd_load_fail;
    }

    // read the file type
    endian_fread_uint32( fileread, &ui32_tmp );
    if ( MPD_ID != ui32_tmp )
    {
        log_warning( "%s - this is not a valid level.mpd!!\n", __FUNCTION__ );
        goto mpd_load_fail;
    }

    // read the rest of the "header"
    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.vertcount = ui32_tmp;

    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.tiles_x = ui32_tmp;

    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.tiles_y = ui32_tmp;

    // allocate the mesh memory
    if ( !mpd_init( pmesh, &loc_info ) )
    {
        goto mpd_load_fail;
    }

    // calculate the total number of tiles in the file
    tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // Load fan data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        endian_fread_uint32( fileread, &ui32_tmp );

        pmem->tile_list[fan].type = CLIP_TO_08BITS( ui32_tmp >> 24 );
        pmem->tile_list[fan].fx   = CLIP_TO_08BITS( ui32_tmp >> 16 );
        pmem->tile_list[fan].img  = CLIP_TO_16BITS( ui32_tmp >>  0 );
    }

    // Load twist data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        endian_fread_uint08( fileread, &ui08_tmp );

        pmem->tile_list[fan].twist = ui08_tmp;
    }

    // Load vertex x data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        pmem->vlst[cnt].pos.x = ieee32_tmp;
    }

    // Load vertex y data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        pmem->vlst[cnt].pos.y = ieee32_tmp;
    }

    // Load vertex z data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        pmem->vlst[cnt].pos.z = ieee32_tmp / 16.0f;
    }

    // Load vertex a data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_uint08( fileread, &ui08_tmp );

        pmem->vlst[cnt].a = ui08_tmp;
    }

    fclose( fileread );

    return pmesh;

mpd_load_fail:

    mpd_renew( pmesh );

    if ( NULL != fileread )
    {
        fclose( fileread );
        fileread = NULL;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_save( const char * savename, mpd_t * pmesh )
{
    FILE* filewrite;
    size_t cnt;

    mpd_info_t * pinfo;
    mpd_mem_t  * pmem;

    if ( NULL == pmesh || INVALID_CSTR( savename ) ) return NULL;

    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return NULL;

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return NULL;

    filewrite = fopen( savename, "wb" );
    if ( NULL == filewrite ) return NULL;

    // write the file identifier
    endian_fwrite_uint32( filewrite, MPD_ID );

    // write the file vertex count
    endian_fwrite_uint32( filewrite, pinfo->vertcount );

    // write the tiles in the x direction
    endian_fwrite_uint32( filewrite, pinfo->tiles_x );

    // write the tiles in the y direction
    endian_fwrite_uint32( filewrite, pinfo->tiles_y );

    // write the fx data for each tile
    for ( cnt = 0; cnt < pmem->tile_count; cnt++ )
    {
        tile_info_t * ptile = pmem->tile_list + cnt;

        endian_fwrite_uint32( filewrite, ( ptile->type << 24 ) + ( ptile->fx << 16 ) + ptile->img );
    }

    // write the twist data for each tile
    for ( cnt = 0; cnt < pmem->tile_count; cnt++ )
    {
        tile_info_t * ptile = pmem->tile_list + cnt;

        endian_fwrite_uint08( filewrite, ptile->twist );
    }

    // write the x-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        endian_fwrite_ieee32( filewrite, pvert->pos.x );
    }

    // write the y-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        endian_fwrite_ieee32( filewrite, pvert->pos.y );
    }

    // write the y-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        endian_fwrite_ieee32( filewrite, pvert->pos.z * 16.0f );
    }

    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        endian_fwrite_uint08( filewrite, pvert->a );
    }

    fclose( filewrite );

    return pmesh;
}
